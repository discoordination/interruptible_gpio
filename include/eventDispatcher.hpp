#ifndef __EVENT_MANAGER_HPP_
#define __EVENT_MANAGER_HPP_

#include "responder.hpp"
#include "event.hpp"

#include <queue>	// For eventQueue
#include <vector>	// For event gpios.
#include <unordered_map>
#include <memory>



namespace GPIO {
namespace Event {

/**
 * Singleton class which queues, processes and dispatches events.
 * It maintains a map of events and subscribers to those events.
 */
class Dispatcher {

	using EventQueue = std::queue<std::unique_ptr<BaseEvent>>;
	using eventId = size_t;

public: 
	static Dispatcher& get();

	void subscribe(eventId event, Responder& target) {
		eventSubscribers[event].push_back(&target);
	}
	void unSubscribe(eventId event, Responder& target) {
		auto& sList = eventSubscribers[event];
		for (auto it = sList.begin(); it != sList.end();) {
			if (*it == &target)
				it = sList.erase(it);
			else ++it;
		}
	}

// Events taken from the queue and then sent to subscribers...  In order?
// Can the event be handled?  ie not processed anymore?
	void dispatch(std::unique_ptr<BaseEvent> e) { eventQueue.push(std::move(e)); }
	void process() {
		while (!eventQueue.empty()) {
			auto &event = *eventQueue.front();
			// This should iterate backwards...
			for (auto& subscriber : eventSubscribers[event.getEventTypeID()]) {
				subscriber->respondToGPIOInterrupt(event);
				if (event.isHandled()) break;
			}
			eventQueue.pop();
		}
	}

private:
	Dispatcher() {}
	Dispatcher(Dispatcher& other) = delete;
	Dispatcher& operator=(const Dispatcher&) = delete;

	EventQueue eventQueue;
	std::unordered_map<size_t, std::vector<Responder*> > eventSubscribers;
	static std::unique_ptr<Dispatcher> dispatcher;
};

} // namespace Event
} // namespace GPIO

#endif // __EVENT_MANAGER_HPP_