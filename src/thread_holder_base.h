
#ifndef THREAD_HOLDER
#define THREAD_HOLDER

#include <thread>
#include <atomic>

enum ThreadState {
	THREAD_STATE_RUNNING,
	THREAD_STATE_CLOSING,
	THREAD_STATE_TERMINATED,
};

template <typename Derived>
class ThreadHolder
{
public:
	ThreadHolder() : threadState(THREAD_STATE_TERMINATED) {}
	void start() {
		setState(THREAD_STATE_RUNNING);
		thread = std::thread(&Derived::threadMain, static_cast<Derived*>(this));
	}

	void stop() {
		setState(THREAD_STATE_CLOSING);
	}

	void join() {
		if (thread.joinable()) {
			thread.join();
		}
	}
protected:
	void setState(ThreadState newState) {
		threadState.store(newState, std::memory_order_relaxed);
	}

	ThreadState getState() const {
		return threadState.load(std::memory_order_relaxed);
	}
private:
	std::atomic<ThreadState> threadState;
	std::thread thread;
};

#endif