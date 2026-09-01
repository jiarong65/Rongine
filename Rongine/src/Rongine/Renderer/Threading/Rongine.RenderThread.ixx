module;
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include <memory>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#endif

export module Rongine.RenderThread;

namespace Rongine {

	static std::thread s_thread;
	static std::thread::id s_renderThreadId{};

	static GLFWwindow* s_window = nullptr;
	static std::mutex s_queueMutex;
	static std::condition_variable s_cv;
	static std::queue<std::function<void()>> s_tasks;
	static std::atomic<bool> s_running{ false };
	static std::atomic<bool> s_started{ false };
}

export namespace Rongine {

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

	void RenderThread::start(GLFWwindow* window)
	{
		s_window = window;
		s_running.store(true);

		s_thread = std::thread(threadMain, window);
		s_started.store(true);

		sync();
	}

	void RenderThread::shutdown()
	{
		s_running.store(false, std::memory_order_release);

		{
			std::lock_guard lock(s_queueMutex);
			s_cv.notify_all();
		}

		if (s_thread.joinable())
			s_thread.join();

		s_window = nullptr;
		s_started.store(false, std::memory_order_release);
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
		auto done = std::make_shared<std::atomic<bool>>(false);
		auto mtx = std::make_shared<std::mutex>();
		auto cv = std::make_shared<std::condition_variable>();

		submit([done, mtx, cv]() {
			done->store(true, std::memory_order_release);
			std::lock_guard lock(*mtx);
			cv->notify_one();
		});

		std::unique_lock lock(*mtx);
		while (!done->load(std::memory_order_acquire))
		{
			cv->wait_for(lock, std::chrono::milliseconds(1));

			if (s_window)
				glfwPollEvents();

#ifdef _WIN32
			MSG msg;
			while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
#endif
		}
	}

	bool RenderThread::isRenderThread()
	{
		return s_started.load(std::memory_order_acquire)
			&& std::this_thread::get_id() == s_renderThreadId;
	}

	void RenderThread::threadMain(GLFWwindow* window)
	{
		s_renderThreadId = std::this_thread::get_id();
		glfwMakeContextCurrent(window);

		while (s_running.load(std::memory_order_acquire))
		{
			std::function<void()> task;
			{
				std::unique_lock lock(s_queueMutex);
				s_cv.wait(lock, [] { return !s_tasks.empty() || !s_running.load(std::memory_order_acquire); });
				if (!s_running.load(std::memory_order_acquire) && s_tasks.empty())
					break;
				task = std::move(s_tasks.front());
				s_tasks.pop();
			}
			if (task)
				task();
		}

		glfwMakeContextCurrent(nullptr);
	}
}
