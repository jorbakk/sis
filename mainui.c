#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
#endif
#if defined __EMSCRIPTEN__ || defined __ANDROID__
#include <GLES3/gl3.h>
#define NANOVG_GLES3_IMPLEMENTATION
#else
#include <GL/glew.h>
#define NANOVG_GL3_IMPLEMENTATION
#endif

#ifdef GLFW_BACKEND
#include <GLFW/glfw3.h>
#elif defined(SDL2_BACKEND)
#include <SDL.h>
#else
#include "sokol_app.h"
#define SOKOL_TIME_IMPL
#include "sokol_time.h"
#endif

#define FONTSTASH_IMPLEMENTATION
#include "fontstash_xc.h"
#include "nanovg_xc.h"
#ifndef SW_ONLY
#include "nanovg_xc_vtex.h"
#endif
#define NANOVG_SW_IMPLEMENTATION
#include "nanovg_xc_sw.h"
#define BLENDISH_IMPLEMENTATION
#include "blendish_xc.h"
#define OUI_IMPLEMENTATION
#include "oui.h"
#include "nfd.h"
#include "sis.h"


#ifdef GLFW_BACKEND
GLFWwindow *window;
#elif defined(SDL2_BACKEND)
SDL_Window *sdl_window = NULL;
int sdl_window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
SDL_Renderer *sdl_renderer = NULL;
SDL_Surface  *sdl_surface  = NULL;
SDL_Texture  *sdl_texture  = NULL;
SDL_Rect      display_bounds;
unsigned char *sw_img_buf = NULL;
bool running = true;
#ifndef SW_ONLY
SDL_GLContext sdlGLContext = NULL;
#endif
#endif

#define DEFAULT_WIN_WIDTH    (800)
#define DEFAULT_WIN_HEIGHT   (600)

const char *window_title = "SIS Stereogram Generator";
const char *image_read_extensions = "png,jpg,bmp,gif,hdr,tga,pic";
const char *image_write_extensions = "png";
static char dropped_file[PATH_MAX];
static int  dropped_file_len = 0;
static bool use_gl = true;
static int num_threads = 0;
static int nvg_flags = NVG_NO_FONTSTASH;

struct main_ctx {
	int argc;
	char **argv;
	NVGcontext *vg;
    UIcontext *ui_ctx;
	double mx, my;
	int depth_map_img, texture_img, sis_img;
} mctx;

typedef enum {
    ST_PANEL = 1,
    ST_BUTTON,
    ST_TEXT,
    ST_SLIDER,
    ST_SLIDER_INT,
    ST_CHECK,
    ST_IMAGE,
    ST_NUMFIELD,
} widget_sub_type;

typedef struct {
    int subtype;
    UIhandler handler;
} widget_head;

typedef struct {
    widget_head head;
    bool border;
} panel_head;

typedef struct {
    widget_head head;
    int iconid;
    const char *label;
} button_head;

typedef struct {
    widget_head head;
    char *text;
    int maxsize;
} text_head;

typedef struct {
    widget_head head;
    const char *label;
    float *value;
    float min, max;
    bool perc;
} slider_head;

typedef struct {
    widget_head head;
    const char *label;
    long int *value;
    long int min, max;
    bool perc;
} slider_int_head;

typedef struct {
    widget_head head;
    const char *label;
    bool *option;
} check_head;

typedef struct {
    widget_head head;
    int img;
} image_head;

typedef struct {
    widget_head head;
    const char *label;
    int *value;
    int min, max;
} numfield_head;


void
show_statistics(void)
{
}


void
event_handler(int item, UIevent event)
{
	widget_head *data = (widget_head *) uiGetHandle(item);
	if (data && data->handler) {
		data->handler(item, event);
	}
}


void
button_handler(int item, UIevent event)
{
	const button_head *data = (const button_head *)uiGetHandle(item);
	printf("clicked item pointer: %p, label: %s\n", uiGetHandle(item), data->label);
}


int
panel(bool border)
{
	int item = uiItem();
	panel_head *data = (panel_head *) uiAllocHandle(item, sizeof(panel_head));
	data->head.subtype = ST_PANEL;
	data->head.handler = NULL;
	data->border = border;
	return item;
}


static bool update_sis_view;


void
sis_button_handler(int item, UIevent event)
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_SaveDialog(image_write_extensions, NULL, &outPath);
    if (result == NFD_OKAY) {
		printf("saving sis image to '%s'\n", outPath);
		strncpy(SISFileName, outPath, PATH_MAX);
		WriteSISFile();
        free(outPath);
    }
}

void update_depth_map_and_sis();
void update_texture();
void update_sis();

void
depth_button_handler(int item, UIevent event)
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(image_read_extensions, DEPTHMAP_PREFIX, &outPath);
    if (result == NFD_OKAY) {
		printf("loading depth image '%s'\n", outPath);
		strncpy(DFileName, outPath, PATH_MAX);
		update_depth_map_and_sis();
        free(outPath);
		update_sis_view = true;
	}
}


void
texture_button_handler(int item, UIevent event)
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(image_read_extensions, TEXTURE_PREFIX, &outPath);
    if (result == NFD_OKAY) {
		printf("loading texture image '%s'\n", outPath);
		strncpy(TFileName, outPath, PATH_MAX);
		update_texture();
		update_sis();
        free(outPath);
		update_sis_view = true;
	}
}


int
button(int iconid, const char *label, UIhandler handler)
{
	int item = uiItem();
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetEvents(item, UI_BUTTON0_HOT_UP);
	button_head *data = (button_head *) uiAllocHandle(item, sizeof(button_head));
	data->head.subtype = ST_BUTTON;
	data->head.handler = handler;
	data->iconid = iconid;
	data->label = label;
	return item;
}


void
textboxhandler(int item, UIevent event)
{
	printf("textboxhandler\n");
	text_head *data = (text_head *) uiGetHandle(item);
	switch (event) {
	default:
		break;
	case UI_BUTTON0_DOWN:{
			uiFocus(item);
		}
		break;
	case UI_KEY_DOWN:{
			unsigned int key = uiGetKey();
			switch (key) {
			default:
				break;
				// case GLFW_KEY_BACKSPACE: {
				// int size = strlen(data->text);
				// if (!size) return;
				// data->text[size-1] = 0;
				// } break;
				// case GLFW_KEY_ENTER: {
				// uiFocus(-1);
				// } break;
			}
		}
		break;
	case UI_CHAR:{
			unsigned int key = uiGetKey();
			if ((key > 255) || (key < 32))
				return;
			int size = strlen(data->text);
			if (size >= (data->maxsize - 1))
				return;
			data->text[size] = (char)key;
		} break;
	}
}


int
textbox(char *text, int maxsize)
{
	int item = uiItem();
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	// uiSetEvents(item, UI_BUTTON0_DOWN | UI_KEY_DOWN | UI_CHAR);
	text_head *data = (text_head *) uiAllocHandle(item, sizeof(text_head));
	data->head.subtype = ST_TEXT;
	data->head.handler = textboxhandler;
	data->text = text;
	data->maxsize = maxsize;
	return item;
}


bool update_sis_image(void);
#if 0
bool update_texture_image(void);
#endif
bool load_texture_image(void);
bool load_depth_image(void);
bool load_sis_image(void);


void
update_sis()
{
	InitAlgorithm();
	render_sis();
	update_sis_image();
}


void
update_texture()
{
	CloseTFile(Theight);
	SIStype = SIS_TEXT_MAP;
	OpenTFile(TFileName, &Twidth, &Theight);
#if 0
	update_texture_image();
#endif
	load_texture_image();
}


void
update_depth_map_and_sis()
{
	CloseDFile();
	CloseSISFile();
	CloseTFile(Theight);
	FreeBuffers();
	/// The next three functions are usually called from FreeBuffers()
	// free(DBuffer);
	// free(IdentBuffer);
	// free(SISBuffer);

	free(SISred);
	free(SISgreen);
	free(SISblue);
	nvgDeleteImage(mctx.vg, mctx.depth_map_img);
	nvgDeleteImage(mctx.vg, mctx.sis_img);

	// init_base(0, NULL);
	init_all(0, NULL);
#if 0
	/// The next four functions are usually called in init_sis()
	OpenDFile(DFileName, &Dwidth, &Dheight);
	if (SIStype == SIS_TEXT_MAP) {
		OpenTFile(TFileName, &Twidth, &Theight);
	}
	InitAlgorithm();
	AllocBuffers();
	// /// The next functions are usually called in AllocBuffers()
	// if ((DBuffer = (col_t *)calloc(Dwidth * oversam, sizeof(col_t))) == NULL) {
		// fprintf(stderr, "Couldn't alloc memory for depth buffer.\n");
		// exit(1);
	// }
	// if ((IdentBuffer = (ind_t *)calloc(SISwidth * oversam, sizeof(ind_t))) == NULL) {
	// // if ((IdentBuffer = (col_t *) calloc(SISwidth * oversam, sizeof(col_t))) == NULL) {
		// fprintf(stderr, "Couldn't alloc memory for ident buffer\n");
		// free(DBuffer);
		// exit(1);
	// }
	// if ((SISBuffer = (col_t *)calloc(SISwidth * oversam, sizeof(col_t))) == NULL) {
		// fprintf(stderr, "Couldn't alloc memory for SIS buffer.\n");
		// free(DBuffer);
		// free(IdentBuffer);
		// exit(1);
	// }
#endif

	CreateSISBuffer(SISwidth, SISheight, SIStype);

	render_sis();
	// WriteSISFile();
	load_sis_image();
	load_depth_image();
}


void
slider_handler(int item, UIevent event)
{
	const slider_head *data = (const slider_head *)uiGetHandle(item);
	// printf("clicked slider pointer: %p, label: %s\n", uiGetHandle(item), data->label);
	UIrect r = uiGetRect(item);
	UIvec2 c = uiGetCursor();
	float v = (float)(c.x - r.x) / (float)r.w;
	if (v < 0.0f) v = 0.0f;
	if (v > 1.0f) v = 1.0f;
	// *(data->progress) = v;
	*(data->value) = v * (data->max - data->min) + data->min;
	update_sis_view = true;
}


int
slider(const char *label, float *value, UIhandler handler, float min, float max, bool perc)
{
	assert(min < max);
	int item = uiItem();
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetLayout(item, UI_HFILL);
	uiSetEvents(item, UI_BUTTON0_CAPTURE);
	slider_head *data = (slider_head *) uiAllocHandle(item, sizeof(slider_head));
	data->head.subtype = ST_SLIDER;
	data->head.handler = handler;
	data->label = label;
	data->value = value;
	// data->progress = *progress * (max - min) + min;
	data->min = min;
	data->max = max;
	data->perc = perc;
	return item;
}


void
slider_int_handler(int item, UIevent event)
{
	const slider_int_head *data = (const slider_int_head *)uiGetHandle(item);
	// printf("clicked slider pointer: %p, label: %s\n", uiGetHandle(item), data->label);
	UIrect r = uiGetRect(item);
	UIvec2 c = uiGetCursor();
	float v = (float)(c.x - r.x) / (float)r.w;
	if (v < 0.0f) v = 0.0f;
	if (v > 1.0f) v = 1.0f;
	// *(data->progress) = v;
	*(data->value) = v * (data->max - data->min) + data->min;
	update_sis_view = true;
}


int
slider_int(const char *label, long int *value, UIhandler handler, long int min, long int max, bool perc)
{
	assert(min < max);
	int item = uiItem();
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetLayout(item, UI_HFILL);
	uiSetEvents(item, UI_BUTTON0_CAPTURE);
	slider_int_head *data = (slider_int_head *) uiAllocHandle(item, sizeof(slider_int_head));
	data->head.subtype = ST_SLIDER_INT;
	data->head.handler = handler;
	data->label = label;
	data->value = value;
	// data->progress = *progress * (max - min) + min;
	data->min = min;
	data->max = max;
	data->perc = perc;
	return item;
}


void
checkhandler(int item, UIevent event)
{
    const check_head *data = (const check_head *)uiGetHandle(item);
    *data->option = !(*data->option);
	update_sis_view = true;
}


int
check(const char *label, bool *option)
{
    int item = uiItem();
    uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetLayout(item, UI_HFILL);
    uiSetEvents(item, UI_BUTTON0_DOWN);
    check_head *data = (check_head *)uiAllocHandle(item, sizeof(check_head));
    data->head.subtype = ST_CHECK;
    data->head.handler = checkhandler;
    data->label = label;
    data->option = option;
    return item;
}


#if 0
void
image_handler(int item, UIevent event)
{
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_SaveDialog(image_write_extensions, NULL, &outPath);
    if (result == NFD_OKAY) {
		printf("saving sis image to '%s'\n", outPath);
		strncpy(SISFileName, outPath, PATH_MAX);
		WriteSISFile();
        free(outPath);
    }
}
#endif


int
image(NVGcontext *vg, int img, int w, int h)
{
	int item = uiItem();
	int iw, ih;
	nvgImageSize(vg, img, &iw, &ih);
	float aspect_ratio = (float)w / (float)h;
	float aspect_ratio_i = (float)iw / (float)ih;
	if (iw < w && ih < h) {
		uiSetSize(item, iw, ih);
	}
	else if (aspect_ratio > aspect_ratio_i) {
		iw *= (float)h / (float)ih;
		uiSetSize(item, iw, h);
	} else {
		ih *= (float)w / (float)iw;
		uiSetSize(item, w, ih);
	}
#if 0
	uiSetEvents(item, UI_BUTTON0_HOT_UP);
	data->head.handler = image_handler;
#endif
	image_head *data = (image_head *) uiAllocHandle(item, sizeof(image_head));
	data->head.subtype = ST_IMAGE;
	data->img = img;
	return item;
}


#if 0
void
image_vert_handler(int item, UIevent event)
{
    const image_head *data = (const image_head *)uiGetHandle(item);
    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(image_read_extensions, NULL, &outPath);
    if (result == NFD_OKAY) {
        if (data->img == mctx.depth_map_img) {
			printf("loading depth image '%s'\n", outPath);
			strncpy(DFileName, outPath, PATH_MAX);
			update_depth_map_and_sis();
        }
        else if (data->img == mctx.texture_img) {
			printf("loading texture image '%s'\n", outPath);
			strncpy(TFileName, outPath, PATH_MAX);
			update_texture();
			update_sis();
        }
        free(outPath);
		update_sis_view = true;
    }

	// switch (event) {
	// default:
		// break;
	// case UI_DROP:{
			// uiFocus(item);
		// }
		// break;
	// }
	// UIvec2 c = uiGetCursor();
	// if (dropped_file_len && uiContains(item, c.x, c.y)) {
		// printf("dropped file '%s' on depth map view\n", dropped_file);
		// dropped_file_len = 0;
	// }
}
#endif


int
image_vert(NVGcontext *vg, int img, int w, int maxh)
{
	int item = uiItem();
	int iw, ih;
	nvgImageSize(vg, img, &iw, &ih);
	ih *= (float)w / (float)iw;
	if (maxh < 0) ih = 5;
	if (maxh > 0) ih = ih > maxh ? maxh : ih;
	uiSetSize(item, w, ih);
#if 0
	uiSetEvents(item, UI_BUTTON0_DOWN);
	data->head.handler = image_vert_handler;
#endif
	image_head *data = (image_head *) uiAllocHandle(item, sizeof(image_head));
	data->head.subtype = ST_IMAGE;
	data->img = img;
	return item;
}


void
numfield_handler(int item, UIevent event)
{
	const numfield_head *data = (const numfield_head *)uiGetHandle(item);
	UIrect r = uiGetRect(item);
	UIvec2 c = uiGetCursor();
	int up = (float)(c.x - r.x) > 0.5f * (float)r.w;
	if (up && *data->value < data->max) {
		(*data->value)++;
		update_sis_view = true;
	} else if (!up && *data->value > data->min) {
		(*data->value)--;
		update_sis_view = true;
	}
}


int
number_field(const char *label, int *value, int min, int max)
{
	int item = uiItem();
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetLayout(item, UI_HFILL);
	uiSetEvents(item, UI_BUTTON0_DOWN);
	numfield_head *data = (numfield_head *) uiAllocHandle(item, sizeof(numfield_head));
	data->head.subtype = ST_NUMFIELD;
	data->head.handler = numfield_handler;
	data->label = label;
	data->value = value;
	data->min = min;
	data->max = max;
	return item;
}


void render_ui(NVGcontext *vg, int item, int corners);


void
render_ui_items(NVGcontext *vg, int item, int corners)
{
	int kid = uiFirstChild(item);
	while (kid > 0) {
		render_ui(vg, kid, corners);
		kid = uiNextSibling(kid);
	}
}


void
render_ui(NVGcontext *vg, int item, int corners)
{
	const widget_head *head = (const widget_head *)uiGetHandle(item);
	UIrect rect = uiGetRect(item);
	if (head) {
		switch (head->subtype) {
		default:{
				render_ui_items(vg, item, corners);
			} break;
		case ST_PANEL: {
				const panel_head *data = (panel_head *) head;
				if (data->border) {
					bndBackground(vg, rect.x, rect.y, rect.w, rect.h);
					bndBevel(vg, rect.x, rect.y, rect.w, rect.h);
					float cr = 5.0f;
					// bndRoundedBox(vg, rect.x, rect.y, rect.w, rect.h, cr, cr, cr, cr);
					// // nvgFillColor(vg, bnd_theme.backgroundColor);
					// // nvgFill(vg);
					// nvgStrokeColor(vg, nvgRGB(1.0, 1.0, 1.0));
					// nvgStroke(vg);
				}
				render_ui_items(vg, item, corners);
			} break;
		case ST_BUTTON: {
				const button_head *data = (button_head *) head;
				bndToolButton(vg, rect.x, rect.y, rect.w, rect.h,
				              corners, (BNDwidgetState) uiGetState(item),
				              data->iconid, data->label);
			} break;
        case ST_TEXT: {
				const text_head *data = (text_head*)head;
				BNDwidgetState state = (BNDwidgetState)uiGetState(item);
				int idx = strlen(data->text);
				bndTextField(vg, rect.x, rect.y, rect.w, rect.h,
				             corners, state, -1, data->text, idx, idx);
            } break;
		case ST_SLIDER: {
				const slider_head *data = (slider_head *) head;
				BNDwidgetState state = (BNDwidgetState) uiGetState(item);
				static char value_str[32];
				float progress = (*data->value - data->min) / (data->max - data->min);
				float value = *data->value;
				if (data->perc) {
					value *= 100.0f;
					sprintf(value_str, "%.0f%%", value);
				} else {
					sprintf(value_str, "%.0f", value);
				}
				bndSlider(vg, rect.x, rect.y, rect.w, rect.h,
				          corners,
				          state,
				          progress,
				          data->label, value_str);
			} break;
		case ST_SLIDER_INT: {
				const slider_int_head *data = (slider_int_head *) head;
				BNDwidgetState state = (BNDwidgetState) uiGetState(item);
				static char value_str[32];
				float progress = (float)(*data->value - data->min) / (float)(data->max - data->min);
				long int value = *data->value;
				if (data->perc) {
					float value_float = value * 1e-2;
					sprintf(value_str, "%.0f%%", value_float);
				} else {
					sprintf(value_str, "%ld", value);
				}
				bndSlider(vg, rect.x, rect.y, rect.w, rect.h,
				          corners,
				          state,
				          progress,
				          data->label, value_str);
			} break;
		case ST_CHECK: {
				const check_head *data = (check_head*)head;
				BNDwidgetState state = (BNDwidgetState)uiGetState(item);
				if (*data->option)
					state = BND_ACTIVE;
				bndOptionButton(vg,rect.x,rect.y,rect.w,rect.h, state,
					data->label);
			} break;
		case ST_IMAGE: {
				const image_head *data = (image_head *) head;
				NVGpaint img_paint = nvgImagePattern(vg,
				  rect.x, rect.y, rect.w, rect.h, 0, data->img, 1.0f);
				nvgBeginPath(vg);
				nvgRect(vg, rect.x, rect.y, rect.w, rect.h);
				nvgFillPaint(vg, img_paint);
				nvgFill(vg);
			} break;
		case ST_NUMFIELD: {
				const numfield_head *data = (numfield_head *) head;
				static char value_str[32];
				sprintf(value_str, "%d", algorithm);
				bndNumberField(vg, rect.x, rect.y, rect.w, rect.h,
				              corners, (BNDwidgetState) uiGetState(item),
				              data->label, value_str);
			} break;
		}
	}
}


void
ui_frame(NVGcontext *vg, float w, float h)
{
	/// Layout user interface
	uiBeginLayout();

	int M = 7;
	int panel_margin_h = M;
	int panel_margin_v = 5;
	int panel_width = 200;
	// int panel_height = 400;
	int panel_height = h - 4 * panel_margin_v;
	int img_width = panel_width - 2 * panel_margin_h;
	int sis_view_width = w - panel_width - 4 * panel_margin_h;
	int sis_view_height = h - 4 * panel_margin_v;
	int pch = 0;

	int root = panel(false);
	uiSetSize(root, w, h);
	uiSetBox(root, UI_LAYOUT);
	uiSetLayout(root, UI_FILL);

	int ctl_panel = panel(true);
	uiSetSize(ctl_panel, panel_width, panel_height);
	uiSetLayout(ctl_panel, UI_TOP | UI_LEFT);
	uiSetMargins(ctl_panel, 10, 10, 5, 10);
	uiSetBox(ctl_panel, UI_COLUMN);
	uiInsert(root, ctl_panel);
	pch += 10;

	int sis_view_panel = panel(false);
	uiSetSize(sis_view_panel, sis_view_width, sis_view_height);
	uiSetLayout(sis_view_panel, UI_RIGHT | UI_TOP);
	uiSetMargins(sis_view_panel, 5, 10, 5, 10);
	uiSetBox(sis_view_panel, UI_COLUMN);
	uiInsert(root, sis_view_panel);

	if (update_sis_view) {
		update_sis();
		update_sis_view = false;
	}
	int sis_view = image(mctx.vg, mctx.sis_img, sis_view_width - 10, sis_view_height - 10);
	uiSetLayout(sis_view, UI_TOP);
	uiSetMargins(sis_view, panel_margin_h, panel_margin_v, panel_margin_h,
	             panel_margin_v);
	uiInsert(sis_view_panel, sis_view);

	int eye_dist_slider = slider_int("eye distance",
	  &eye_dist, slider_int_handler, 10, 0.5 * Dwidth, false);
	uiSetMargins(eye_dist_slider, M, 8, M, 3);
	uiInsert(ctl_panel, eye_dist_slider);
	pch += BND_WIDGET_HEIGHT + 8 + 3;

	int near_plane_slider = slider("scene depth", &u, slider_handler, 0.01f, 1.0f, true);
	uiSetMargins(near_plane_slider, M, 3, M, 3);
	uiInsert(ctl_panel, near_plane_slider);
	pch += BND_WIDGET_HEIGHT + 3 + 3;

	int far_plane_slider = slider("back distance", &t, slider_handler, 0.25f, 2.0f, true);
	uiSetMargins(far_plane_slider, M, 3, M, 3);
	uiInsert(ctl_panel, far_plane_slider);
	pch += BND_WIDGET_HEIGHT + 3 + 3;

	int origin_slider = slider_int("algo origin",
	  &origin, slider_int_handler, 0, Dwidth, false);
	uiSetMargins(origin_slider, M, 3, M, 3);
	uiInsert(ctl_panel, origin_slider);
	pch += BND_WIDGET_HEIGHT + 3 + 3;

	int algo_numfield;
	algo_numfield = number_field("algorithm", &algorithm, 1, 4);
	uiSetMargins(algo_numfield, M, 5, M, 5);
	uiInsert(ctl_panel, algo_numfield);
	pch += BND_WIDGET_HEIGHT + 3 + 3;

	int opt_show_marker = check("show markers", &mark);
	uiSetMargins(opt_show_marker, 2 * M, 5, 2 * M, 5);
	uiInsert(ctl_panel, opt_show_marker);
	pch += BND_WIDGET_HEIGHT + 2 * M + 5;

	int opt_invert_depth_map = check("invert depth", &invert);
	uiSetMargins(opt_invert_depth_map, 2 * M, 5, 2 * M, 5);
	uiInsert(ctl_panel, opt_invert_depth_map);
	pch += BND_WIDGET_HEIGHT + 2 * M + 5;

	// int file_buttons_panel = panel(false);
	// // uiSetLayout(file_buttons_panel, UI_HFILL);
	// uiSetSize(file_buttons_panel, 0, BND_WIDGET_HEIGHT);
	// uiSetMargins(file_buttons_panel, M, 5, M, 5);
	// uiSetBox(file_buttons_panel, UI_ROW);
	// uiInsert(ctl_panel, file_buttons_panel);

	int sis_button = button(BND_ICON_GHOST, "save sis image ...", sis_button_handler);
	uiSetLayout(sis_button, UI_HFILL | UI_TOP);
	uiSetMargins(sis_button, M, 5, M, 5);
	uiInsert(ctl_panel, sis_button);
	pch += BND_WIDGET_HEIGHT + 5 + 5;

	int depth_button = button(BND_ICON_GHOST, "load depth map ...", depth_button_handler);
	uiSetLayout(depth_button, UI_HFILL | UI_TOP);
	uiSetMargins(depth_button, M, 5, M, 5);
	uiInsert(ctl_panel, depth_button);
	pch += BND_WIDGET_HEIGHT + 5 + 5;

	int depth_map_view = image_vert(mctx.vg, mctx.depth_map_img,
	                                img_width, 0);
	uiSetMargins(depth_map_view, panel_margin_h, 0, panel_margin_h,
	             panel_margin_v);
	uiInsert(ctl_panel, depth_map_view);
	/// The depth map height is limited by panel_width, as the aspect ration
	/// of the depth image is kept
	pch += panel_width;

	int texture_button = button(BND_ICON_GHOST, "load texture image ...", texture_button_handler);
	uiSetLayout(texture_button, UI_HFILL | UI_TOP);
	uiSetMargins(texture_button, M, 5, M, 5);
	uiInsert(ctl_panel, texture_button);
	// pch += BND_WIDGET_HEIGHT + 5 + 5;

	int texture_view = image_vert(mctx.vg, mctx.texture_img, img_width, panel_height - pch);
	uiSetMargins(texture_view, panel_margin_h, 0, panel_margin_h,
	             panel_margin_v);
	uiInsert(ctl_panel, texture_view);

	int fill_panel = panel(false);
	uiSetLayout(fill_panel, UI_VFILL);
	uiInsert(ctl_panel, fill_panel);

	uiEndLayout();

	/// Need to handle dropping zones here, for now
	UIvec2 c = uiGetCursor();
	if (dropped_file_len && uiContains(depth_map_view, c.x, c.y) && (uiGetButton(0) == 0)) {
		// printf("dropped file '%s' on depth map view\n", dropped_file);
		strncpy(DFileName, dropped_file, PATH_MAX);
		update_depth_map_and_sis();
		dropped_file_len = 0;
	}
	if (dropped_file_len && uiContains(texture_view, c.x, c.y) && (uiGetButton(0) == 0)) {
		// printf("dropped file '%s' on texture view\n", dropped_file);
		strncpy(TFileName, dropped_file, PATH_MAX);
		update_texture();
		update_sis();
		dropped_file_len = 0;
	}

	/// Render user interface
	render_ui(vg, 0, BND_CORNER_NONE);
	/// Process user events
#ifdef GLFW_BACKEND
	uiProcess((int)(glfwGetTime() * 1e-3));
#elif SDL2_BACKEND
	uiProcess((int)(SDL_GetTicks()));
#else
	uiProcess((int)(stm_sec(stm_now()) * 1000.0));
#endif
}



void
frame(void)
{
	double t, dt;
	int winWidth, winHeight;
	int fbWidth, fbHeight;
	float pxRatio;

#ifdef GLFW_BACKEND
	glfwGetCursorPos(window, &mctx.mx, &mctx.my);
	glfwGetWindowSize(window, &winWidth, &winHeight);
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
#elif SDL2_BACKEND
    SDL_GetWindowSize(sdl_window, &winWidth, &winHeight);
    SDL_GL_GetDrawableSize(sdl_window, &fbWidth, &fbHeight);
    // SDL_GetRendererOutputSize(sdl_renderer, &fbWidth, &fbHeight);
#else
	winWidth = sapp_width();
	winHeight = sapp_height();
	fbWidth = sapp_width();
	fbHeight = sapp_height();
#endif
	/// Calculate pixel ration for hi-dpi devices.
	pxRatio = (float)fbWidth / (float)winWidth;

	/// Update and render
	glViewport(0.0f, 0.0f, fbWidth, fbHeight);
	glClearColor(0.20f, 0.20f, 0.20f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	nvgBeginFrame(mctx.vg, winWidth, winHeight, pxRatio);
	ui_frame(mctx.vg, winWidth, winHeight);
	nvgEndFrame(mctx.vg);

	glEnable(GL_DEPTH_TEST);
}


unsigned char *
rgb_to_rgba(unsigned char *img, int pixels)
{
	unsigned char *ret = malloc(4 * pixels);
	for (int i = 0; i < pixels; ++i) {
		ret[4 * i + 0] = img[3 * i + 0];
		ret[4 * i + 1] = img[3 * i + 1];
		ret[4 * i + 2] = img[3 * i + 2];
		ret[4 * i + 3] = 0xff;
	}
	return ret;
}


bool
load_depth_image(void)
{
	mctx.depth_map_img = nvgCreateImage(mctx.vg, DFileName, 0);

	return true;
}


#if 0
bool
update_depth_image(void)
{
	int imageFlags = 0;
	unsigned char *img = rgb_to_rgba(GetDFileBuffer(), Dwidth * Dheight);
	nvgUpdateImage(mctx.vg, mctx.depth_map_img, img);
	free(img);
	return true;
}
#endif


bool
load_texture_image(void)
{
	mctx.texture_img = nvgCreateImage(mctx.vg, TFileName, 0);
	// mctx.texture_img = nvgCreateImageRGBA(mctx.vg, Twidth, Theight, imageFlags, GetTFileBuffer());
	return true;
}


#if 0
bool
update_texture_image(void)
{
	int imageFlags = 0;
	unsigned char *img = rgb_to_rgba(GetTFileBuffer(), Twidth * Theight);
	nvgUpdateImage(mctx.vg, mctx.texture_img, img);
	free(img);
	return true;
}
#endif


bool
load_sis_image(void)
{
	update_sis_view = false;
	int imageFlags = 0;
	unsigned char *img = rgb_to_rgba(GetSISFileBuffer(), SISwidth * SISheight);
	mctx.sis_img = nvgCreateImageRGBA(mctx.vg, SISwidth, SISheight, imageFlags, img);
	free(img);
	return true;
}


bool
update_sis_image(void)
{
	int imageFlags = 0;
	unsigned char *img = rgb_to_rgba(GetSISFileBuffer(), SISwidth * SISheight);
	nvgUpdateImage(mctx.vg, mctx.sis_img, img);
	free(img);
	return true;
}


static float maxf(float a, float b) { return a < b ? b : a; }
static float minf(float a, float b) { return a < b ? a : b; }

typedef struct {
	NVGcontext *vg;
	float *fbuff;
	int fbuffw, fbuffh;
	float sdfScale, sdfOffset;
} SDFcontext;

static const float INITIAL_SDF_DIST = 1E6f;
float fontsize = 24.0f;
float fontblur = 0;


static void
sdfRender(void *uptr, void *fontimpl, unsigned char *output,
          int outWidth, int outHeight, int outStride, float scale, int padding,
          int glyph)
{
	SDFcontext *ctx = (SDFcontext *) uptr;

	nvgBeginFrame(ctx->vg, 0, 0, 1);
	nvgDrawSTBTTGlyph(ctx->vg, (stbtt_fontinfo *) fontimpl, scale, padding,
	                  glyph);
	nvgEndFrame(ctx->vg);

	for (int iy = 0; iy < outHeight; ++iy) {
		for (int ix = 0; ix < outWidth; ++ix) {
			float sd =
			    ctx->fbuff[ix + iy * ctx->fbuffw] * ctx->sdfScale +
			    ctx->sdfOffset;
			output[ix + iy*outStride] = (unsigned char)(0.5f + minf(maxf(sd, 0.f), 255.f));
			ctx->fbuff[ix + iy * ctx->fbuffw] = INITIAL_SDF_DIST;   // will get clamped to 255
		}
	}
}


static void
sdfDelete(void *uptr)
{
	SDFcontext *ctx = (SDFcontext *) uptr;
	free(ctx->fbuff);
	free(ctx);
}


FONScontext *
createFontstash(NVGcontext *vg, int nvgFlags, int maxAtlasFontPx)
{
	FONSparams params;
	memset(&params, 0, sizeof(FONSparams));
	params.flags = FONS_ZERO_TOPLEFT | FONS_DELAY_LOAD;
	params.flags |= (nvgFlags & NVG_SDF_TEXT) ? FONS_SDF : FONS_SUMMED;
	params.sdfPadding = 4;
	params.sdfPixelDist = 32.0f;

	SDFcontext *ctx = malloc(sizeof(SDFcontext));
	ctx->fbuffh = ctx->fbuffw = maxAtlasFontPx + 2 * params.sdfPadding + 16;
	// we use dist < 0.0f inside glyph; but for scale > 0, stbtt uses >on_edge_value for inside
	ctx->sdfScale = -params.sdfPixelDist;
	ctx->sdfOffset = 127;       // stbtt on_edge_value

	ctx->fbuff = malloc(ctx->fbuffw * ctx->fbuffh * sizeof(float));
	for (size_t ii = 0; ii < ctx->fbuffw * ctx->fbuffh; ++ii)
		ctx->fbuff[ii] = INITIAL_SDF_DIST;
	// ctx->vg = nvgswCreate(NVG_AUTOW_DEFAULT | NVG_NO_FONTSTASH | NVGSW_PATHS_XC | NVGSW_SDFGEN);
	ctx->vg = vg;
	// nvgswSetFramebuffer(ctx->vg, ctx->fbuff, ctx->fbuffw, ctx->fbuffh, params.sdfPadding, 0,0,0);

	params.userPtr = ctx;
	params.userSDFRender = sdfRender;
	params.userDelete = sdfDelete;
	return fonsCreateInternal(&params);
}


bool
init_ui(void)
{
	mctx = (struct main_ctx) {
		.mx = 0.0, .my = 0.0, .vg = NULL,
	};
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
	GLenum glew_ret = glewInit();
	if (glew_ret != GLEW_OK) {
		// SDL_Log("Could not init glew: %s\n", glewGetErrorString(glew_ret));
		return false;
	} else {
		// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
		glGetError();
	}
	// SDL_GL_SetSwapInterval(0);
	// SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));
#endif
	mctx.vg = nvglCreate(nvg_flags);
	if (mctx.vg == NULL) {
		printf("Could not init nanovg.\n");
		return false;
	}
	nvgSetFontStash(mctx.vg, createFontstash(mctx.vg, NVG_NO_FONTSTASH, 2 * 48));
	nvgAtlasTextThreshold(mctx.vg, 48.0f);
	bndSetFont(nvgCreateFont(mctx.vg, "system", ASSET_PREFIX "/DejaVuSans.ttf"));
	bndSetIconImage(nvgCreateImage(mctx.vg, ASSET_PREFIX "/blender_icons16.png", 0));
#if !defined(GLFW_BACKEND) && !defined(SDL2_BACKEND)
	stm_setup();
#endif
	mctx.ui_ctx = uiCreateContext(4096, 1<<20);
	uiMakeCurrent(mctx.ui_ctx);
	uiSetHandler(event_handler);

	load_depth_image();
	load_texture_image();
	load_sis_image();
	return true;
}


void
cleanup(void)
{
    uiDestroyContext(mctx.ui_ctx);
	// nvglDelete(mctx.vg);
}


void
init_app(void)
{
	init_all(mctx.argc, mctx.argv);
	render_sis();
	init_ui();
}


#ifdef GLFW_BACKEND

static void
mousebutton(GLFWwindow *window, int button, int action, int mods)
{
	NVG_NOTUSED(window);
	switch (button) {
	case 1:
		button = 2;
		break;
	case 2:
		button = 1;
		break;
	}
	uiSetButton(button, mods, (action == GLFW_PRESS) ? 1 : 0);
}

static void
cursorpos(GLFWwindow *window, double x, double y)
{
	NVG_NOTUSED(window);
	uiSetCursor((int)x, (int)y);
}

static void
scrollevent(GLFWwindow *window, double x, double y)
{
	NVG_NOTUSED(window);
	uiSetScroll((int)x, (int)y);
}

static void
charevent(GLFWwindow *window, unsigned int value)
{
	NVG_NOTUSED(window);
	uiSetChar(value);
}

static void
key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	uiSetKey(key, mods, action);
}

static void
drop_callback(GLFWwindow *window, int count, const char **paths)
{
	if (count > 0) {
		dropped_file_len = strlen(paths[0]);
		strncpy(dropped_file, paths[0], PATH_MAX);
#if 0
	    UIinputEvent event = { 0, 0, UI_DROP };
	    uiAddInputEvent(event);
	    uiSetChar('D');
#endif
	}
	// for (int i = 0; i < count; i++) {
		// printf("dropped: %s\n", paths[i]);
	// }
}

int
main(int argc, char **argv)
{
	if (!glfwInit()) {
		fprintf(stderr, "failed to init glfw\n");
		return EXIT_FAILURE;
	}
	/// If requesting an OpenGL version below 3.2, GLFW_OPENGL_ANY_PROFILE must be used
	/// which is already the default. On Linux, this results in a Compatibility Profile
	/// Version 4.6.
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	window = glfwCreateWindow(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, window_title, NULL, NULL);
	if (!window) {
		fprintf(stderr, "failed to create an OpenGL window\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key);
	glfwSetCharCallback(window, charevent);
	glfwSetCursorPosCallback(window, cursorpos);
	glfwSetMouseButtonCallback(window, mousebutton);
	glfwSetScrollCallback(window, scrollevent);
	glfwSetDropCallback(window, drop_callback);

	mctx.argc = argc;
	mctx.argv = argv;
	init_app();
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		frame();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	cleanup();
	finish_all();
	glfwTerminate();
	return 0;
}

#elif defined(SDL2_BACKEND)

/*
void
init_frame_buffer(void)
{
	if (!sw_img_buf || winPrevWidth != winWidth || winPrevHeight != winHeight) {
#ifndef __plan9__
		float ddpi, hdpi, vdpi;
		SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(sdlWindow), &ddpi, &hdpi, &vdpi);
		printf("init_frame_buffer() win: [%d,%d] -> [%d,%d], fb: [%d,%d], dpi: [%.1f,%.1f,%.1f]\n", winPrevWidth, winPrevHeight, winWidth, winHeight, fbWidth, fbHeight, ddpi, hdpi, vdpi);
#endif
		winPrevWidth = winWidth;
		winPrevHeight = winHeight;
		sw_img_buf = realloc(sw_img_buf, winWidth * winHeight * 4);
		if (sdlTexture != NULL) SDL_DestroyTexture(sdlTexture);
		sdlTexture = SDL_CreateTexture(
			sdlRenderer,
			SDL_PIXELFORMAT_ABGR8888,
			// SDL_TEXTUREACCESS_STREAMING,
			SDL_TEXTUREACCESS_TARGET,  // fast update w/o locking, can be used as a render target
			// set video size as the dimensions of the texture
			winWidth, winHeight
			);
	}
	nvgswSetFramebuffer(vg, sw_img_buf, winWidth, winHeight, 0, 8, 16, 24);
}
*/


bool
init_sdl_window(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	if (use_gl) {
#ifndef SW_ONLY
		sdl_window = SDL_CreateWindow("Nanovg SDL Renderer Demo",
		  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		  DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT,
		  sdl_window_flags | SDL_WINDOW_OPENGL);
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
		// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_Log("Using GL backend");
		SDL_GL_SetSwapInterval(0);
		// SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));
#else
		// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		// SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
	    sdlGLContext = SDL_GL_CreateContext(sdl_window);
	    if (!sdlGLContext) {
			SDL_Log("Could not set up GL context: %s\n", SDL_GetError());
			return false;
	    }
		SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));
#endif    /// SW_ONLY
	} else {
		sdl_window = SDL_CreateWindow("Nanovg SDL Renderer Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		  DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, sdl_window_flags);
		if (sdl_window == NULL) {
			SDL_Log("Error SDL_CreateWindow: %s\n", SDL_GetError());
			return false;
		}
		SDL_Log("%s\n", "Using SDL renderer with SW backend");
		sdl_renderer = SDL_CreateRenderer(
			sdl_window,
			-1,
			// SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
			// SDL_RENDERER_TARGETTEXTURE);
			0);
		if (sdl_renderer == NULL) {
			SDL_Log("SDL: could not create renderer: %s\n", SDL_GetError());
			return false;
		}
#ifndef __plan9__
		SDL_RendererInfo ri;
		if (SDL_GetRendererInfo(sdl_renderer, &ri) < 0) {
			SDL_Log("Failed to get renderer info: %s\n", SDL_GetError());
		} else {
			SDL_Log("Renderer name: %s, max texture width: %d, height: %d\n", ri.name, ri.max_texture_width, ri.max_texture_height);
		}
		/// At least on OS X the renderer will set up a GL 2.1 context
		SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));
#endif
		mctx.vg = nvgswCreate(nvg_flags);
		if (mctx.vg == NULL) {
			SDL_Log("%s\n", "Could not init nanovg");
			return false;
		}
#ifndef NO_THREADING
        int disp = SDL_GetWindowDisplayIndex(sdl_window);
        SDL_GetDisplayBounds(disp < 0 ? 0 : disp, &display_bounds);
	    if (num_threads == 0) num_threads = SDL_GetCPUCount();  // * (PLATFORM_MOBILE ? 1 : 2)
	    if (num_threads > 1) {
	      int xthreads = display_bounds.h > display_bounds.w ? 2 : num_threads/2;  // prefer square-like tiles
	      nvgswSetThreading(mctx.vg, xthreads, num_threads/xthreads);
	    }
#endif
	    // init_frame_buffer();
	}
	return true;
}


void
clear(void)
{
	if (use_gl) {
#ifndef SW_ONLY
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
#endif
	} else {
		// memset(sw_img_buf, 0.3f * 0xff, winWidth * winHeight * 4);
	}
}


void
cleanup_window(void)
{
	if (use_gl) {
#ifndef SW_ONLY
		nvglDelete(mctx.vg);
#endif
	} else {
		nvgswDelete(mctx.vg);
		// free(sw_img_buf);
	}
}


void
mainloop(void)
{
	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {
		if (evt.type == SDL_QUIT) {
			running = false;
			return;
		}
		else if (evt.type == SDL_MOUSEBUTTONDOWN) {
			uiSetButton(0, 0, 1);
		}
		else if (evt.type == SDL_MOUSEBUTTONUP) {
			uiSetButton(0, 0, 0);
		}
		else if (evt.type == SDL_MOUSEMOTION) {
			int mx, my;
		    SDL_GetMouseState(&mx, &my);
		    mctx.mx = mx; mctx.my = my;
		}
	}
    // SDL_GetMouseState(&mx, &my);
    // mx = mx * pxRatioX;
    // my = my * pxRatioY;
    // printf("mouse [%d,%d]\n", mx, my);
	uiSetCursor((int)mctx.mx, (int)mctx.my);
	clear();
	frame();
	if (use_gl) {
#ifndef SW_ONLY
		SDL_GL_SwapWindow(sdl_window);
#endif
	} else {
		SDL_RenderPresent(sdl_renderer);
	}
}


int
main(int argc, char **argv)
{
	if (!init_sdl_window()) exit(1);
	mctx.argc = argc;
	mctx.argv = argv;
	init_app();
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
	emscripten_set_main_loop(mainloop, 0, true);
#else
	while (running) mainloop();
#endif
	cleanup();
	finish_all();
 	cleanup_window();
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
	return 0;
}

#else

void
event_cb(const sapp_event* ev)
{
	// const float dpi_scale = sapp_dpi_scale();
	const float dpi_scale = 1.0f;
	switch (ev->type) {
		case SAPP_EVENTTYPE_MOUSE_DOWN:
		    uiSetButton((ev->mouse_button == SAPP_MOUSEBUTTON_LEFT ? 0 : 1), 0, 1);
			mctx.mx = ev->mouse_x / dpi_scale;
			mctx.my = ev->mouse_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
		    break;
		case SAPP_EVENTTYPE_MOUSE_UP:
		    uiSetButton((ev->mouse_button == SAPP_MOUSEBUTTON_LEFT ? 0 : 1), 0, 0);
			mctx.mx = ev->mouse_x / dpi_scale;
			mctx.my = ev->mouse_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
		    break;
		case SAPP_EVENTTYPE_MOUSE_MOVE:
			mctx.mx = ev->mouse_x / dpi_scale;
			mctx.my = ev->mouse_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
			break;
		case SAPP_EVENTTYPE_TOUCHES_BEGAN:
		    uiSetButton(0, 0, 1);
			mctx.mx = ev->touches[0].pos_x / dpi_scale;
			mctx.my = ev->touches[0].pos_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
			break;
		case SAPP_EVENTTYPE_TOUCHES_ENDED:
		    uiSetButton(0, 0, 0);
			mctx.mx = ev->touches[0].pos_x / dpi_scale;
			mctx.my = ev->touches[0].pos_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
			break;
		case SAPP_EVENTTYPE_TOUCHES_MOVED:
			mctx.mx = ev->touches[0].pos_x / dpi_scale;
			mctx.my = ev->touches[0].pos_y / dpi_scale;
		    uiSetCursor((int)mctx.mx,(int)mctx.my);
			break;
		case SAPP_EVENTTYPE_FILES_DROPPED:
			dropped_file_len = strlen(sapp_get_dropped_file_path(0));
			strncpy(dropped_file, sapp_get_dropped_file_path(0), PATH_MAX);
			break;
		default:
			break;
	}
}


sapp_desc
sokol_main(int argc, char *argv[])
{
	mctx.argc = argc;
	mctx.argv = argv;
	return (sapp_desc) {
		.init_cb = init_app,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.event_cb = event_cb,
		.width = DEFAULT_WIN_WIDTH,
		.height = DEFAULT_WIN_HEIGHT,
		.window_title = window_title,
		.enable_dragndrop = true,
		.max_dropped_files = 1,
		.max_dropped_file_path_length = PATH_MAX,
	};
}

#endif    /// GLFW_BACKEND
