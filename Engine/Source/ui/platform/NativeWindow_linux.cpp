#include "ui/NativeWindow.h"

#if defined(GOWTOOL_OS_LINUX)

#include <GLFW/glfw3.h>

namespace NativeWindow {

void setFullFrameCallback(void(*)()) {}
void setup(GLFWwindow*, bool) {}
void beginFrame(GLFWwindow*, float) {}
void endFrame(GLFWwindow*)          {}

} // namespace NativeWindow
#endif
