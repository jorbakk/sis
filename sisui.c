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


struct main_ctx {
	NVGcontext *vg;
    UIcontext *ui_ctx;
	double mx, my;
} mctx;

typedef enum {
    ST_PANEL = 1,
    ST_BUTTON,
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


void
event_handler(int item, UIevent event)
{
	widget_head *data = (widget_head *) uiGetHandle(item);
	if (data && data->handler) {
		data->handler(item, event);
	}
}


static void
root_handler(int parent, UIevent event)
{
	switch (event) {
	default:
		break;
	case UI_BUTTON0_DOWN:{
			printf("root panel: %d clicks\n", uiGetClicks());
		}
		break;
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
		}
	}
}


void
ui_frame(NVGcontext * vg, float w, float h)
{
	/// Layout user interface
	uiBeginLayout();

    int root = panel();
    // Set size of root element
    // uiSetSize(root, w, h);
    uiSetSize(root, 200, 100);
    ((widget_head*)uiGetHandle(root))->handler = root_handler;
    uiSetEvents(root, UI_BUTTON0_DOWN);
    uiSetBox(root, UI_COLUMN);

	int hello_button = button(BND_ICON_GHOST, "Hello OUI", button_handler);
    uiSetLayout(hello_button, UI_HFILL | UI_TOP);
    // uiSetLayout(hello_button, UI_HCENTER | UI_TOP);
    uiInsert(root, hello_button);

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

#ifdef SOKOL_APP_INCLUDED
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



#ifdef SOKOL_APP_INCLUDED
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
		.width = 650,.height = 650,
		.window_title = "OUI Button",
		// .logger.func = slog_func,
	};
}
#endif   /// SOKOL_APP_INCLUDED


// #ifdef _glfw3h_
#ifdef GLFW_BACKEND
int
main(void)
{
    GLFWwindow* window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        /* Poll for and process events */
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

#endif    /// _glfw3h_
