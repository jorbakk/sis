#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define GLFW_BACKEND


#if defined __EMSCRIPTEN__
#include <GLES3/gl3.h>
#define SOKOL_GLES3
#else
#if defined NANOVG_USE_GLEW
#include <GL/glew.h>
#endif


#ifdef __APPLE__
/* Defined before OpenGL and GLUT includes to avoid deprecation messages */
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
// #include <GLUT/glut.h>
#else
#include <GL/gl.h>
// #include <GL/glut.h>
#endif

// #include <GL/gl.h>
// #include <GL/glext.h>

#define SOKOL_GLCORE33
#endif


#ifdef GLFW_BACKEND
#include <GLFW/glfw3.h>
#else
#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_time.h"
#endif


#include "nanovg.h"
#include "nanovg_gl.h"

#define BLENDISH_IMPLEMENTATION
#include "blendish.h"
#define OUI_IMPLEMENTATION
#include "oui.h"

// #include "stb_image.h"

#include "sis.h"


#ifdef GLFW_BACKEND
GLFWwindow *window;
#endif


const char *window_title = "SIS Stereogram Generator";
static char dropped_file[PATH_MAX];
static int  dropped_file_len = 0;

struct main_ctx {
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
algo_plus_button_handler(int item, UIevent event)
{
	if (algorithm < SIS_MAX_ALGO) {
		algorithm++;
		update_sis_view = true;
	}
}


void
algo_minus_button_handler(int item, UIevent event)
{
	if (algorithm > SIS_MIN_ALGO) {
		algorithm--;
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
	uiSetEvents(item, UI_BUTTON0_HOT_UP);
	image_head *data = (image_head *) uiAllocHandle(item, sizeof(image_head));
	data->head.subtype = ST_IMAGE;
	data->head.handler = NULL;
	data->img = img;
	return item;
}


#if 0
void
image_vert_handler(int item, UIevent event)
{
	printf("image_vert_handler\n");

	switch (event) {
	default:
		break;
	case UI_DROP:{
			uiFocus(item);
		}
		break;
	}

	UIvec2 c = uiGetCursor();
	if (dropped_file_len && uiContains(item, c.x, c.y)) {
		printf("dropped file '%s' on depth map view\n", dropped_file);
		dropped_file_len = 0;
	}
}
#endif


int
image_vert(NVGcontext *vg, int img, int w)
{
	int item = uiItem();
	int iw, ih;
	nvgImageSize(vg, img, &iw, &ih);
	ih *= (float)w / (float)iw;
	uiSetSize(item, w, ih);
	// uiSetEvents(item, UI_BUTTON0_HOT_UP);
	// uiSetEvents(item, UI_DROP);
	// uiSetEvents(item, UI_CHAR);
	// uiSetEvents(item, UI_DROP | UI_BUTTON0_DOWN | UI_KEY_DOWN | UI_CHAR);
	image_head *data = (image_head *) uiAllocHandle(item, sizeof(image_head));
	data->head.subtype = ST_IMAGE;
	// data->head.handler = image_vert_handler;
	data->img = img;
	return item;
}


void render_ui(NVGcontext * vg, int item, int corners);


void
render_ui_items(NVGcontext * vg, int item, int corners)
{
	int kid = uiFirstChild(item);
	while (kid > 0) {
		render_ui(vg, kid, corners);
		kid = uiNextSibling(kid);
	}
}


void
render_ui(NVGcontext * vg, int item, int corners)
{
	const widget_head *head = (const widget_head *)uiGetHandle(item);
	UIrect rect = uiGetRect(item);
	if (head) {
		switch (head->subtype) {
		default:{
				render_ui_items(vg, item, corners);
			}
			break;
		case ST_PANEL: {
				const panel_head *data = (panel_head *) head;
				if (data->border) {
					bndBevel(vg, rect.x, rect.y, rect.w, rect.h);
				}
				render_ui_items(vg, item, corners);
			}
			break;
		case ST_BUTTON: {
				const button_head *data = (button_head *) head;
				bndToolButton(vg, rect.x, rect.y, rect.w, rect.h,
				              corners, (BNDwidgetState) uiGetState(item),
				              data->iconid, data->label);
			}
			break;
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
			}
			break;
		}
	}
}


void
ui_frame(NVGcontext * vg, float w, float h)
{
	/// Layout user interface
	uiBeginLayout();

	int M = 6;
	int panel_margin_h = M;
	int panel_margin_v = 5;
	int panel_width = 200;
	// int panel_height = 400;
	int panel_height = h - 4 * panel_margin_v;
	int img_width = panel_width - 2 * panel_margin_h;
	int sis_view_width = w - panel_width - 4 * panel_margin_h;
	int sis_view_height = h - 4 * panel_margin_v;

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

	// int hello_button = button(BND_ICON_GHOST, "Hello SIS", button_handler);
	// uiSetLayout(hello_button, UI_HFILL | UI_TOP);
	// uiSetMargins(hello_button, panel_margin_h, panel_margin_v, panel_margin_h, panel_margin_v);
	// uiInsert(ctl_panel, hello_button);

	int eye_dist_slider = slider_int("eye distance",
	  &eye_dist, slider_int_handler, 25, 0.5 * Dwidth, false);
	uiSetMargins(eye_dist_slider, M, 10, M, 3);
	uiInsert(ctl_panel, eye_dist_slider);

	int origin_slider = slider_int("algo origin",
	  &origin, slider_int_handler, 0, Dwidth, false);
	uiSetMargins(origin_slider, M, 3, M, 3);
	uiInsert(ctl_panel, origin_slider);

	int near_plane_slider = slider("scene depth", &u, slider_handler, 0.01f, 1.0f, true);
	uiSetMargins(near_plane_slider, M, 3, M, 3);
	uiInsert(ctl_panel, near_plane_slider);

	int far_plane_slider = slider("back distance", &t, slider_handler, 0.25f, 2.0f, true);
	uiSetMargins(far_plane_slider, M, 3, M, 3);
	uiInsert(ctl_panel, far_plane_slider);

	int opt_show_marker = check("show markers", &mark);
	uiSetMargins(opt_show_marker, 2 * M, 5, 2 * M, 5);
	uiInsert(ctl_panel, opt_show_marker);

	int opt_invert_depth_map = check("invert depth", &invert);
	uiSetMargins(opt_invert_depth_map, 2 * M, 5, 2 * M, 5);
	uiInsert(ctl_panel, opt_invert_depth_map);

	int algo_panel = panel(false);
	uiSetLayout(algo_panel, UI_HFILL);
	uiSetSize(algo_panel, 0, BND_WIDGET_HEIGHT);
	uiSetMargins(algo_panel, M, 5, M, 5);
	uiSetBox(algo_panel, UI_ROW);
	uiInsert(ctl_panel, algo_panel);

	int algo_plus_button;
	algo_plus_button = button(-1, "+", algo_plus_button_handler);
	// uiSetLayout(algo_plus_button, UI_LEFT | UI_VCENTER);
	uiSetLayout(algo_plus_button, UI_LEFT);
	uiSetSize(algo_plus_button, 23, BND_WIDGET_HEIGHT);
	uiSetMargins(algo_plus_button, 2, 2, 2, 2);
	uiInsert(algo_panel, algo_plus_button);

	int algo_minus_button;
	algo_minus_button = button(-1, "-", algo_minus_button_handler);
	// uiSetLayout(algo_minus_button, UI_LEFT | UI_VCENTER);
	uiSetLayout(algo_minus_button, UI_LEFT);
	uiSetSize(algo_minus_button, 23, BND_WIDGET_HEIGHT);
	uiSetMargins(algo_minus_button, 2, 2, 2, 2);
	uiInsert(algo_panel, algo_minus_button);

	static char algo_value_str[8];
	sprintf(algo_value_str, "%d", algorithm);
	int algo_value = textbox(algo_value_str, 8);
	uiSetLayout(algo_value, UI_LEFT);
	uiSetSize(algo_value, 25, BND_WIDGET_HEIGHT);
	uiSetMargins(algo_value, 2, 2, 2, 2);
	uiInsert(algo_panel, algo_value);

	int depth_map_view = image_vert(mctx.vg, mctx.depth_map_img,
	                                img_width);
	uiSetMargins(depth_map_view, panel_margin_h, panel_margin_v, panel_margin_h,
	             panel_margin_v);
	uiInsert(ctl_panel, depth_map_view);

	int texture_view = image_vert(mctx.vg, mctx.texture_img, img_width);
	uiSetMargins(texture_view, panel_margin_h, panel_margin_v, panel_margin_h,
	             panel_margin_v);
	uiInsert(ctl_panel, texture_view);

	int fill_panel = panel(false);
	uiSetLayout(fill_panel, UI_VFILL);
	uiInsert(ctl_panel, fill_panel);

	uiEndLayout();

	/// Need to handle dropping zones here, for now
	UIvec2 c = uiGetCursor();
	if (dropped_file_len && uiContains(depth_map_view, c.x, c.y) && (uiGetButton(0) == 0)) {
		printf("dropped file '%s' on depth map view\n", dropped_file);
		strncpy(DFileName, dropped_file, PATH_MAX);
		dropped_file_len = 0;
	}
	if (dropped_file_len && uiContains(texture_view, c.x, c.y) && (uiGetButton(0) == 0)) {
		printf("dropped file '%s' on texture view\n", dropped_file);
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

	glfwGetCursorPos(window, &mctx.mx, &mctx.my);
#ifdef GLFW_BACKEND
	glfwGetWindowSize(window, &winWidth, &winHeight);
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
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


bool
update_depth_image(void)
{
	int imageFlags = 0;
	unsigned char *img = rgb_to_rgba(GetDFileBuffer(), Dwidth * Dheight);
	nvgUpdateImage(mctx.vg, mctx.depth_map_img, img);
	free(img);
	return true;
}


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


void
init_app(void)
{
	mctx = (struct main_ctx) {
		.mx = 0.0, .my = 0.0, .vg = NULL,
	};
#if defined __EMSCRIPTEN__
	mctx.vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	mctx.vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
	if (mctx.vg == NULL) {
		printf("Could not init nanovg.\n");
		return;
	}
	bndSetFont(nvgCreateFont(mctx.vg, "system", "assets/DejaVuSans.ttf"));
	bndSetIconImage(nvgCreateImage(mctx.vg, "assets/blender_icons16.png", 0));
#ifndef GLFW_BACKEND
	stm_setup();
#endif
	mctx.ui_ctx = uiCreateContext(4096, 1<<20);
	uiMakeCurrent(mctx.ui_ctx);
	uiSetHandler(event_handler);

	load_depth_image();
	load_texture_image();
	load_sis_image();
}


void
cleanup(void)
{
    uiDestroyContext(mctx.ui_ctx);

#if defined __EMSCRIPTEN__
	nvgDeleteGLES3(mctx.vg);
#else
	nvgDeleteGL3(mctx.vg);
#endif
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
	if (!glfwInit())
		return -1;
	window = glfwCreateWindow(640, 480, window_title, NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key);
	glfwSetCharCallback(window, charevent);
	glfwSetCursorPosCallback(window, cursorpos);
	glfwSetMouseButtonCallback(window, mousebutton);
	glfwSetScrollCallback(window, scrollevent);
	glfwSetDropCallback(window, drop_callback);

	init_all(argc, argv);
	render_sis();
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
		default:
			break;
	}
}


sapp_desc
sokol_main(int argc, char *argv[])
{
	return (sapp_desc) {
		.init_cb = init_app,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.event_cb = event_cb,
		.width = 640,
		.height = 480,
		.window_title = window_title,
	};
}

#endif    /// GLFW_BACKEND
