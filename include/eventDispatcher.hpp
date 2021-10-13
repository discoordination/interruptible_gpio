#ifndef __EVENT_MANAGER_HPP_
#define __EVENT_MANAGER_HPP_

#include "responder.hpp"
#include "event.hpp"

#include <queue>	// For eventQueue
#include <vector>	// For event gpios.
#include <unordered_map>
#include <memory>

#ifdef IG_DEBUG
	#include <iostream>
#endif


namespace GPIO {
namespace Event {

/**
 * Singleton class which queues, processes and dispatches events.
 * It maintains a map of events and subscribers to those events.
 */
class Dispatcher {

	using EventQueue = std::queue<std::unique_ptr<BaseEvent>>;
	using SubscriberMap = std::unordered_map<size_t, std::vector<Responder*>>;
	using eventId = size_t;

public: 
	/**
	 * Returns a reference to the Event::Dispatcher singleton object.
	 */
	static Dispatcher& get();

	/**
	 * Subscribes a target to an event using the unique eventID.  This can be found
	 * by calling GPIO::Event::EventID::value<EventType>().
	 */
	void subscribe(eventId event, Responder& target);

	/**
	 * Unsubscribe from an event using the unique eventID.  This can be found by
	 * calling GPIO::Event::EventID::value<EventType>().
	 */
	void unSubscribe(eventId event, Responder& target);


	/**
	 * Place an event into the eventQueue.  It will then be propagated on the next
	 * call to process.
	 */
	void dispatch(std::unique_ptr<BaseEvent> e);

	/**
	 * Remove events from the eventQueue and call the list of responders to that
	 * event's respondToGPIOInterrupt(event) function.  Events can be declared
	 * handled in which case propagation ends.
	 */
	void process();

	/**
	 * Drop the next event.
	 */
	inline void dropNext();

	/**
	 * Suspend all event processing.  Events are ignored.
	 */ 
	inline void suspend();

	/**
	 * Resume from suspend.
	 */
	inline void resume();

private:
	Dispatcher() : m_dropNext{false}, m_suspended{false} {}
	Dispatcher(Dispatcher& other) = delete;
	Dispatcher& operator=(const Dispatcher&) = delete;

	inline static EventQueue eventQueue;
	inline static SubscriberMap eventSubscribers;
	static std::unique_ptr<Dispatcher> dispatcher;

	bool m_dropNext;
	bool m_suspended;
};

} // namespace Event
} // namespace GPIO

#endif // __EVENT_MANAGER_HPP_