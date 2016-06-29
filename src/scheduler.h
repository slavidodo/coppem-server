

#ifndef SCHEDULER
#define SCHEDULER

#include "tasks.h"
#include <unordered_set>
#include <queue>

#include "thread_holder_base.h"

#define SCHEDULER_MINTICKS 50

class SchedulerTask : public Task
{
public:
	void setEventId(uint32_t id) {
		eventId = id;
	}
	uint32_t getEventId() const {
		return eventId;
	}

	std::chrono::system_clock::time_point getCycle() const {
		return expiration;
	}

protected:
	SchedulerTask(uint32_t delay, const std::function<void(void)>& f) : Task(delay, f) {
		eventId = 0;
	}

	uint32_t eventId;

	friend SchedulerTask* createSchedulerTask(uint32_t, const std::function<void(void)>&);
};

inline SchedulerTask* createSchedulerTask(uint32_t delay, const std::function<void(void)>& f)
{
	return new SchedulerTask(delay, f);
}

struct TaskComparator {
	bool operator()(const SchedulerTask* lhs, const SchedulerTask* rhs) const {
		return lhs->getCycle() > rhs->getCycle();
	}
};

class Scheduler : public ThreadHolder<Scheduler>
{
public:
	uint32_t addEvent(SchedulerTask* task);
	bool stopEvent(uint32_t eventId);

	void shutdown();

	void threadMain();
protected:
	std::thread thread;
	std::mutex eventLock;
	std::condition_variable eventSignal;

	uint32_t lastEventId{ 0 };
	std::priority_queue<SchedulerTask*, std::deque<SchedulerTask*>, TaskComparator> eventList;
	std::unordered_set<uint32_t> eventIds;
};

extern Scheduler g_scheduler;

#endif
