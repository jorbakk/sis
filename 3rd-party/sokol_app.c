#define SOKOL_APP_IMPL
#if defined __EMSCRIPTEN__
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE33
#endif
#include "sokol_app.h"
