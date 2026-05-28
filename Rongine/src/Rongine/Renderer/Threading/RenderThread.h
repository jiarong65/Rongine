#pragma once

#include "Rongine/Core/Core.h"
#include <functional>

struct GLFWwindow;

namespace Rongine{

class RenderThread
{
public:
    static void start(GLFWwindow* window);
    static void shutdown();

    static void submit(std::function<void()> task);
    static void sync();

    static bool isRenderThread();
private:
    static void threadMain(GLFWwindow* window);
};

}
