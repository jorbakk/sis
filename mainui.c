/*
 * Copyright 2025, 2026 Jörg Bakker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
#endif
#ifndef SW_ONLY
#include <GL/glew.h>
#define NANOVG_GL3_IMPLEMENTATION
#endif
#include <SDL.h>
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
#ifndef __plan9__
#include "nfd.h"
#include "nfd_sdl2.h"
#endif
#include "sis.h"


SDL_Window *sdl_window = NULL;
int sdl_window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
SDL_Renderer *sdl_renderer = NULL;
SDL_Surface  *sdl_surface  = NULL;
SDL_Texture  *sdl_texture  = NULL;
SDL_Rect      display_bounds;
int winPrevWidth = 0, winPrevHeight = 0;
int winWidth, winHeight;
int fbWidth, fbHeight;
float pxRatio;
unsigned char *sw_img_buf = NULL;
bool running = true;
#ifndef SW_ONLY
SDL_GLContext sdlGLContext = NULL;
#endif

#define DEFAULT_WIN_WIDTH    (800)
#define DEFAULT_WIN_HEIGHT   (600)

const char *window_title = "SIS Stereogram Generator";
const char *image_read_extensions = "png,jpg,bmp,gif,hdr,tga,pic";
const char *image_write_extensions = "png";
static char dropped_file[PATH_MAX];
static int  dropped_file_len = 0;
static bool use_gl = false;
static int num_threads = 0;
static int nvg_flags = NVG_NO_FONTSTASH;
bool gui = true;

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
#ifndef __plan9__
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filters[1] = {{"SIS files", image_write_extensions}};
    nfdsavedialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    if (!NFD_GetNativeWindowFromSDLWindow(sdl_window, &args.parentWindow)) {
        fprintf(stderr, "NFD_GetNativeWindowFromSDLWindow failed: %s\n", NFD_GetError());
    }
    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY) {
		fprintf(stderr, "saving sis image to '%s'\n", outPath);
		strncpy(SISFileName, outPath, PATH_MAX);
		WriteSISFile();
        free(outPath);
    }
#endif
}

void update_depth_map_and_sis();
void update_texture();
void update_sis();

void
depth_button_handler(int item, UIevent event)
{
#ifndef __plan9__
    nfdu8char_t *outPath = NULL;
    nfdu8filteritem_t filters[1] = {{"Depth files", image_read_extensions}};
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    args.defaultPath = DEPTHMAP_PREFIX;
    if (!NFD_GetNativeWindowFromSDLWindow(sdl_window, &args.parentWindow)) {
        fprintf(stderr, "NFD_GetNativeWindowFromSDLWindow failed: %s\n", NFD_GetError());
    }
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY) {
		fprintf(stderr, "loading depth image '%s'\n", outPath);
		strncpy(DFileName, outPath, PATH_MAX);
		update_depth_map_and_sis();
        free(outPath);
		update_sis_view = true;
	}
#endif
}


void
texture_button_handler(int item, UIevent event)
{
#ifndef __plan9__
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filters[1] = {{"Texture files", image_read_extensions}};
    nfdopendialogu8args_t args = {0};
    args.filterList = filters;
    args.filterCount = 1;
    args.defaultPath = TEXTURE_PREFIX;
    if (!NFD_GetNativeWindowFromSDLWindow(sdl_window, &args.parentWindow)) {
        fprintf(stderr, "NFD_GetNativeWindowFromSDLWindow failed: %s\n", NFD_GetError());
    }
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY) {
		fprintf(stderr, "loading texture image '%s'\n", outPath);
		strncpy(TFileName, outPath, PATH_MAX);
		update_texture();
		update_sis();
        free(outPath);
		update_sis_view = true;
	}
#endif
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


bool update_sis_image(void);
#if 0
bool update_texture_image(void);
#endif
bool load_texture_image(void);
bool load_depth_image(void);
bool load_sis_image(void);


void
update_sis(void)
{
	InitAlgorithm();
	render_sis();
	update_sis_image();
}


void
update_texture(void)
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
update_depth_map_and_sis(void)
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

	/// FIXME deleting images causes segfault with the software backend but not with GL backend
	// nvgDeleteImage(mctx.vg, mctx.depth_map_img);
	// nvgDeleteImage(mctx.vg, mctx.sis_img);

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
	uiProcess((int)(SDL_GetTicks()));
}


void init_frame_buffer(void);

void
frame(void)
{
	// double t;
    SDL_GetWindowSize(sdl_window, &winWidth, &winHeight);
    SDL_GL_GetDrawableSize(sdl_window, &fbWidth, &fbHeight);
    // SDL_GetRendererOutputSize(sdl_renderer, &fbWidth, &fbHeight);
	/// Calculate pixel ration for hi-dpi devices.
	pxRatio = (float)fbWidth / (float)winWidth;
	if (use_gl) {
#ifndef SW_ONLY
		glViewport(0, 0, winWidth, winHeight);
#endif
	} else {
		init_frame_buffer();
	}

	/// Update and render
	if (use_gl) {
#ifndef SW_ONLY
		glViewport(0.0f, 0.0f, fbWidth, fbHeight);
		glClearColor(0.20f, 0.20f, 0.20f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
#endif
	}

	nvgBeginFrame(mctx.vg, winWidth, winHeight, pxRatio);
	ui_frame(mctx.vg, winWidth, winHeight);
	nvgEndFrame(mctx.vg);

	if (use_gl) {
#ifndef SW_ONLY
		glEnable(GL_DEPTH_TEST);
#endif
	} else {
		SDL_Rect win_rect = {0, 0, winWidth, winHeight};
		SDL_UpdateTexture(
			sdl_texture,
			&win_rect,
			sw_img_buf,
			winWidth * 4
		);
		// copy and place a portion of the texture to the current rendering target
		// set video size when copying sdl texture to sdl renderer. Texture will be stretched to blit_copy_rect!
		SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &win_rect);
	}
}


unsigned char *
rgb_to_rgba(unsigned char *img, int pixels)
{
	unsigned char *ret = (unsigned char *)malloc(4 * pixels);
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

	SDFcontext *ctx = (SDFcontext *)malloc(sizeof(SDFcontext));
	ctx->fbuffh = ctx->fbuffw = maxAtlasFontPx + 2 * params.sdfPadding + 16;
	// we use dist < 0.0f inside glyph; but for scale > 0, stbtt uses >on_edge_value for inside
	ctx->sdfScale = -params.sdfPixelDist;
	ctx->sdfOffset = 127;       // stbtt on_edge_value

	ctx->fbuff = (float *)malloc(ctx->fbuffw * ctx->fbuffh * sizeof(float));
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
	// mctx = (struct main_ctx) {
		// .mx = 0.0, .my = 0.0, .vg = NULL,
	// };
	if (use_gl) {
#ifndef SW_ONLY
		// fprintf(stderr, "OpenGL version: %s\n", glGetString(GL_VERSION));
		GLenum glew_ret = glewInit();
		if (glew_ret != GLEW_OK) {
			// fprintf(stderr, "Could not init glew: %s\n", glewGetErrorString(glew_ret));
			return false;
		} else {
			// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
			glGetError();
		}
		// SDL_GL_SetSwapInterval(0);
		// fprintf(stderr, "OpenGL version: %s\n", glGetString(GL_VERSION));
		mctx.vg = nvglCreate(nvg_flags);
		if (mctx.vg == NULL) {
			fprintf(stderr, "Could not init nanovg.\n");
			return false;
		}
#endif
	}
	nvgSetFontStash(mctx.vg, createFontstash(mctx.vg, NVG_NO_FONTSTASH, 2 * 48));
	nvgAtlasTextThreshold(mctx.vg, 48.0f);
	bndSetFont(nvgCreateFont(mctx.vg, "system", ASSET_PREFIX "/DejaVuSans.ttf"));
	bndSetIconImage(nvgCreateImage(mctx.vg, ASSET_PREFIX "/blender_icons16.png", 0));
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
#ifndef __plan9__
	if (NFD_Init() != NFD_OKAY) {
		fprintf(stderr,  "Failed to init native file dialog: %s\n", NFD_GetError());
	};
#endif
	init_all(mctx.argc, mctx.argv);
	render_sis();
	init_ui();
}



void
init_frame_buffer(void)
{
	if (!sw_img_buf || winPrevWidth != winWidth || winPrevHeight != winHeight) {
#ifndef __plan9__
		float ddpi, hdpi, vdpi;
		SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(sdl_window), &ddpi, &hdpi, &vdpi);
		fprintf(stderr, "init_frame_buffer() win: [%d,%d] -> [%d,%d], fb: [%d,%d], dpi: [%.1f,%.1f,%.1f]\n", winPrevWidth, winPrevHeight, winWidth, winHeight, fbWidth, fbHeight, ddpi, hdpi, vdpi);
#endif
		winPrevWidth = winWidth;
		winPrevHeight = winHeight;
		sw_img_buf = (unsigned char*)realloc(sw_img_buf, winWidth * winHeight * 4);
		if (sdl_texture != NULL) SDL_DestroyTexture(sdl_texture);
		sdl_texture = SDL_CreateTexture(
			sdl_renderer,
			SDL_PIXELFORMAT_ABGR8888,
			// SDL_TEXTUREACCESS_STREAMING,
			SDL_TEXTUREACCESS_TARGET,  // fast update w/o locking, can be used as a render target
			// set video size as the dimensions of the texture
			winWidth, winHeight
			);
	}
	nvgswSetFramebuffer(mctx.vg, sw_img_buf, winWidth, winHeight, 0, 8, 16, 24);
}


bool
init_sdl_window(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Failed to init SDL: %s\n", SDL_GetError());
		return false;
	}
	if (use_gl) {
#ifndef SW_ONLY
		sdl_window = SDL_CreateWindow(window_title,
		  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		  DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT,
		  sdl_window_flags | SDL_WINDOW_OPENGL);
		// SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		fprintf(stderr, "Using GL backend");
		SDL_GL_SetSwapInterval(0);
		// fprintf(stderr, "OpenGL version: %s\n", glGetString(GL_VERSION));
	    sdlGLContext = SDL_GL_CreateContext(sdl_window);
	    if (!sdlGLContext) {
			fprintf(stderr, "Could not set up GL context: %s\n", SDL_GetError());
			return false;
	    }
		fprintf(stderr, "OpenGL version: %s\n", glGetString(GL_VERSION));
#endif    /// SW_ONLY
	} else {
		sdl_window = SDL_CreateWindow(window_title,
		  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		  DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, sdl_window_flags);
		if (sdl_window == NULL) {
			fprintf(stderr, "Error SDL_CreateWindow: %s\n", SDL_GetError());
			return false;
		}
		fprintf(stderr, "%s\n", "Using SDL renderer with SW backend");
		sdl_renderer = SDL_CreateRenderer(
			sdl_window,
			-1,
			// SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
			// SDL_RENDERER_TARGETTEXTURE);
			0);
		if (sdl_renderer == NULL) {
			fprintf(stderr, "SDL: could not create renderer: %s\n", SDL_GetError());
			return false;
		}
#ifndef __plan9__
		SDL_RendererInfo ri;
		if (SDL_GetRendererInfo(sdl_renderer, &ri) < 0) {
			fprintf(stderr, "Failed to get renderer info: %s\n", SDL_GetError());
		} else {
			fprintf(stderr, "Renderer name: %s, max texture width: %d, height: %d\n", ri.name, ri.max_texture_width, ri.max_texture_height);
		}
		/// At least on OS X the renderer will set up a GL 2.1 context
		fprintf(stderr, "OpenGL version: %s\n", glGetString(GL_VERSION));
#endif
		mctx.vg = nvgswCreate(nvg_flags);
		if (mctx.vg == NULL) {
			fprintf(stderr, "%s\n", "Could not init nanovg");
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
	    init_frame_buffer();
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
		memset(sw_img_buf, 0.3f * 0xff, winWidth * winHeight * 4);
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
		free(sw_img_buf);
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
#ifndef __plan9__
		else if (evt.type == SDL_DROPFILE) {
			SDL_DropEvent *drop_event = (SDL_DropEvent *) &evt;
			dropped_file_len = strlen(drop_event->file);
			strncpy(dropped_file, drop_event->file, PATH_MAX);
			SDL_free(drop_event->file);
		}
#endif
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
#ifndef __plan9__
	NFD_Quit();
#endif
	SDL_DestroyWindow(sdl_window);
	SDL_Quit();
	return 0;
}

