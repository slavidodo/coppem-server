

#ifndef TASKS
#define TASKS

#include <condition_variable>
#include "thread_holder_base.h"

const int DISPATCHER_TASK_EXPIRATION = 2000;
const auto SYSTEM_TIME_ZERO = std::chrono::system_clock::time_point(std::chrono::milliseconds(0));

class Task
{
public:
	// DO NOT allocate this class on the stack
	Task(uint32_t ms, const std::function<void(void)>& f) : func(f) {
		expiration = std::chrono::system_clock::now() + std::chrono::milliseconds(ms);
	}
	explicit Task(const std::function<void(void)>& f)
		: expiration(SYSTEM_TIME_ZERO), func(f) {}

	virtual ~Task() = default;
	void operator()() {
		func();
	}

	void setDontExpire() {
		expiration = SYSTEM_TIME_ZERO;
	}

	bool hasExpired() const {
		if (expiration == SYSTEM_TIME_ZERO) {
			return false;
		}
		return expiration < std::chrono::system_clock::now();
	}

protected:
	// Expiration has another meaning for scheduler tasks,
	// then it is the time the task should be added to the
	// dispatcher
	std::chrono::system_clock::time_point expiration;
	std::function<void(void)> func;
};

inline Task* createTask(const std::function<void(void)>& f)
{
	return new Task(f);
}

inline Task* createTask(uint32_t expiration, const std::function<void(void)>& f)
{
	return new Task(expiration, f);
}

class Dispatcher : public ThreadHolder<Dispatcher>
{
public:
	void addTask(Task* task, bool push_front = false);
	void shutdown();

	uint64_t getDispatcherCycle() const {
		return dispatcherCycle;
	}
	void threadMain();
protected:
	std::thread thread;
	std::mutex taskLock;
	std::condition_variable taskSignal;

	std::list<Task*> taskList;
	uint64_t dispatcherCycle{ 0 };
};

extern Dispatcher g_dispatcher;

#endif
