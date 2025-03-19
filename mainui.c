#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define GLFW_BACKEND


#if defined __ANDROID__ || defined __EMSCRIPTEN__
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

#include "sis.h"


#ifdef GLFW_BACKEND
GLFWwindow *window;
#endif


const char *window_title = "SIS Stereogram Generator";

struct main_ctx {
	NVGcontext *vg;
    UIcontext *ui_ctx;
	double mx, my;
	int depth_map_img, texture_img;
} mctx;

typedef enum {
    ST_PANEL = 1,
    ST_BUTTON,
    ST_IMAGE,
} widget_sub_type;

typedef struct {
    int subtype;
    UIhandler handler;
} widget_head;

typedef struct {
    widget_head head;
    int iconid;
    const char *label;
} button_head;

typedef struct {
    widget_head head;
    int img;
} image_head;


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
panel()
{
	int item = uiItem();
	widget_head *data = (widget_head *) uiAllocHandle(item, sizeof(widget_head));
	data->subtype = ST_PANEL;
	data->handler = NULL;
	return item;
}


int
button(int iconid, const char *label, UIhandler handler)
{
	// create new ui item
	int item = uiItem();
	// set size of widget; horizontal size is dynamic, vertical is fixed
	uiSetSize(item, 0, BND_WIDGET_HEIGHT);
	uiSetEvents(item, UI_BUTTON0_HOT_UP);
	// store some custom data with the button that we use for styling
	button_head *data = (button_head *) uiAllocHandle(item, sizeof(button_head));
	data->head.subtype = ST_BUTTON;
	data->head.handler = handler;
	data->iconid = iconid;
	data->label = label;
	return item;
}


int
image(NVGcontext *vg, int img, int w, UIhandler handler)
{
	int item = uiItem();
	// set size of widget; horizontal and vertical sizes are dynamic
	int iw, ih;
	nvgImageSize(vg, img, &iw, &ih);
	ih *= (float)w / (float)iw;
	uiSetSize(item, w, ih);
	uiSetEvents(item, UI_BUTTON0_HOT_UP);
	// store some custom data with the button that we use for styling
	image_head *data = (image_head *) uiAllocHandle(item, sizeof(image_head));
	data->head.subtype = ST_IMAGE;
	data->head.handler = handler;
	data->img = img;
	return item;
}


void
render_ui(NVGcontext * vg, int item, int corners);


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
		case ST_PANEL:{
				bndBevel(vg, rect.x, rect.y, rect.w, rect.h);
				render_ui_items(vg, item, corners);
			}
			break;
		case ST_BUTTON:{
				const button_head *data = (button_head *) head;
				bndToolButton(vg, rect.x, rect.y, rect.w, rect.h,
				              corners, (BNDwidgetState) uiGetState(item),
				              data->iconid, data->label);
			}
			break;
		case ST_IMAGE:{
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

	int panel_width = 200;
	int panel_height = 400;
	int panel_margin_h = 5;
	int panel_margin_v = 5;
	int img_width = panel_width - 2 * panel_margin_h;

	int ctl_panel = panel();
	// Set size of ctl_panel element
	uiSetSize(ctl_panel, panel_width, panel_height);
	uiSetEvents(ctl_panel, UI_BUTTON0_DOWN);
	// uiSetLayout(ctl_panel, UI_TOP);
	uiSetMargins(ctl_panel, 10, 10, 0, 0);
	uiSetBox(ctl_panel, UI_COLUMN);

	int hello_button = button(BND_ICON_GHOST, "Hello SIS", button_handler);
	uiSetLayout(hello_button, UI_HFILL | UI_TOP);
	uiSetMargins(hello_button, panel_margin_h, panel_margin_v, panel_margin_h, panel_margin_v);
	uiInsert(ctl_panel, hello_button);

	int depth_map_view = image(mctx.vg, mctx.depth_map_img, img_width, NULL);
	uiSetLayout(depth_map_view, UI_TOP);
	uiSetMargins(depth_map_view, panel_margin_h, panel_margin_v, panel_margin_h, panel_margin_v);
	uiInsert(ctl_panel, depth_map_view);

	int texture_view = image(mctx.vg, mctx.texture_img, img_width, NULL);
	uiSetLayout(texture_view, UI_TOP);
	uiSetMargins(texture_view, panel_margin_h, panel_margin_v, panel_margin_h, panel_margin_v);
	uiInsert(ctl_panel, texture_view);

	uiEndLayout();

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


void
init_app(void)
{
	mctx = (struct main_ctx) {
		.mx = 0.0, .my = 0.0, .vg = NULL,
	};
#if defined __ANDROID__ || defined __EMSCRIPTEN__
	mctx.vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	mctx.vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#endif
	if (mctx.vg == NULL) {
		printf("Could not init nanovg.\n");
		return;
	}
#ifdef __ANDROID__
	/// Get an asset manager on android
	ANativeActivity *jni_activity = (ANativeActivity *) sapp_android_get_native_activity();
	AAssetManager *pAssetManager = jni_activity->assetManager;
	// JNIEnv *env = jni_activity->env;
	nvgSetAndroidAssetManager(pAssetManager);
    bndSetFont(nvgCreateFont(mctx.vg, "system", "DejaVuSans.ttf"));
    bndSetIconImage(nvgCreateImage(mctx.vg, "blender_icons16.png", 0));
#else
    bndSetFont(nvgCreateFont(mctx.vg, "system", "assets/DejaVuSans.ttf"));
    bndSetIconImage(nvgCreateImage(mctx.vg, "assets/blender_icons16.png", 0));
    mctx.depth_map_img = nvgCreateImage(mctx.vg, "depthmaps/flowers.png", 0);
    mctx.texture_img = nvgCreateImage(mctx.vg, "textures/clover.png", 0);
#endif
#ifndef GLFW_BACKEND
	stm_setup();
#endif
    mctx.ui_ctx = uiCreateContext(4096, 1<<20);
    uiMakeCurrent(mctx.ui_ctx);
    uiSetHandler(event_handler);
}


void
cleanup(void)
{
    uiDestroyContext(mctx.ui_ctx);

#if defined __ANDROID__ || defined __EMSCRIPTEN__
	nvgDeleteGLES3(mctx.vg);
#else
	nvgDeleteGL3(mctx.vg);
#endif
}


#ifdef GLFW_BACKEND

static void
mousebutton(GLFWwindow * window, int button, int action, int mods)
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
cursorpos(GLFWwindow * window, double x, double y)
{
	NVG_NOTUSED(window);
	uiSetCursor((int)x, (int)y);
}

static void
scrollevent(GLFWwindow * window, double x, double y)
{
	NVG_NOTUSED(window);
	uiSetScroll((int)x, (int)y);
}

static void
charevent(GLFWwindow * window, unsigned int value)
{
	NVG_NOTUSED(window);
	uiSetChar(value);
}

static void
key(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	uiSetKey(key, mods, action);
}

int
main(void)
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

	init_app();
	while (!glfwWindowShouldClose(window)) {
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		frame();
		/* Swap front and back buffers */
		glfwSwapBuffers(window);
		/* Poll for and process events */
		glfwPollEvents();
	}
	cleanup();
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
