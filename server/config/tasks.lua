-- Dispatcher
Dispatcher.start() -- Starting.
---- Methods :
	-- Dispatcher.start() (void)
	-- Dispatcher.addTask(function, ...args) (bool [success])
	-- Dispatcher.shutdown() (void)

-- Scheduler
Scheduler.start()
---- Methods :
	-- Scheduler.start() (void)
	-- Scheduler.addEvent(function, delay, ...args) (int [eventId])
	-- Scheduler.stopEvent(eventId) (void)
	-- Scheduler.shutdown() (void)