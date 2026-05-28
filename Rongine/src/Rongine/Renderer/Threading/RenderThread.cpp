#include "Rongpch.h"
#include "Rongine/Renderer/Threading/RenderThread.h"

#include <GLFW/glfw3.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace Rongine{

static std::thread s_thread;
static std::thread::id s_renderThreadId{};

static GLFWwindow* s_window=nullptr;
static std::mutex s_queueMutex;
static std::condition_variable s_cv;
static std::queue<std::function<void()>> s_tasks;
static std::atomic<bool> s_running{false};
static std::atomic<bool> s_started{false};

void RenderThread::start(GLFWwindow* window)
{
    s_window=window;
    s_running.store(true);

    s_thread=std::thread(threadMain,window);
    s_started.store(true);

    sync();
}

void RenderThread::shutdown()
{
    s_running.store(false,std::memory_order_release);

    {
        std::lock_guard lock(s_queueMutex);
        s_cv.notify_all();
    }

    if(s_thread.joinable())
        s_thread.join();
    
    s_window=nullptr;
    s_started.store(false,std::memory_order_release);
}

void RenderThread::submit(std::function<void()> task)
{
    {
        std::lock_guard lock(s_queueMutex);
        s_tasks.push(task);
    }
    s_cv.notify_one();
}

void RenderThread::sync()
{
    auto done=std::make_shared<std::atomic<bool>>(false);
    auto mtx=std::make_shared<std::mutex>();
    auto cv=std::make_shared<std::condition_variable>();

    submit([done,mtx,cv](){
        done->store(true,std::memory_order_release);
        std::lock_guard lock(*mtx);
        cv->notify_one();
    });

    std::unique_lock lock(*mtx);
    cv->wait(lock,[&]{return done->load(std::memory_order_acquire);});
}

bool RenderThread::isRenderThread()
{
    return std::this_thread::get_id()==s_renderThreadId;
}

void RenderThread::threadMain(GLFWwindow* window)
{
    s_renderThreadId=std::this_thread::get_id();
    //将窗口上下文搬到渲染线程
    glfwMakeContextCurrent(window);  
    
    while(s_running.load(std::memory_order_acquire))
    {
        std::function<void()> task;
        {
            std::unique_lock lock(s_queueMutex);
            s_cv.wait(lock,[]{return !s_tasks.empty()|| !s_running.load(std::memory_order_acquire);});
            if(!s_running.load(std::memory_order_acquire)&&s_tasks.empty())
                break;
            task=std::move(s_tasks.front());
            s_tasks.pop();
        }
        if(task)
            task();
    }

    glfwMakeContextCurrent(nullptr);
}

}
