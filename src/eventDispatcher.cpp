#include "eventDispatcher.hpp"
#include <algorithm>
// EventQueue eventQueue {};

namespace GPIO {

std::unique_ptr<Event::Dispatcher> Event::Dispatcher::dispatcher = nullptr;


Event::Dispatcher& Event::Dispatcher::get() {
	if (dispatcher == nullptr) {
		dispatcher = std::unique_ptr<Dispatcher>(new Dispatcher{});
	}
	return *dispatcher;
}



void Event::Dispatcher::subscribe(eventId event, Responder& target) {

	#ifdef IG_DEBUG
		std::cout << "Responder: " << target.id << " subscribing to event: " << event << "\n";
	#endif

	// Check if event already in the subscriberList.
	if (eventSubscribers.find(event) != std::end(eventSubscribers)) {

		auto& subs = eventSubscribers[event];

		// Check if already subscribed to the event.
		if (std::find(std::begin(subs), std::end(subs), &target) != std::end(subs)) {
			return;  // Return early as already subscribed....
		}
	}
	// If event not already mapped and target not subscribed add the subscriber for the event...
	eventSubscribers[event].push_back(&target);
}



void Event::Dispatcher::unSubscribe(eventId event, Responder& target) {

	#ifdef IG_DEBUG
		std::cout << "Responder: " << target.id << " unsubscribing from event: " << event << "\n";
	#endif

	auto& sList = eventSubscribers[event];
	
	sList.erase(
		std::remove_if( std::begin(sList), std::end(sList),
			[&target](const Responder* t) {
				return (&target == t); 
			}
		), std::end(sList)
	);
}



void Event::Dispatcher::dispatch(std::unique_ptr<BaseEvent> e) {
	if (m_suspended) return;
	eventQueue.push(std::move(e)); 
}



void Event::Dispatcher::process() {

	while (!eventQueue.empty()) {

		if (m_dropNext) {
			m_dropNext = false;
			eventQueue.pop();
			continue;
		}

		auto& event = *eventQueue.front();
		
		// Warning!!! you are modifying your subscribers list from inside the loop.
		// Solution... copy the list first then iterate over it.  Changes will only take effect 	on the next call of process.

		// Create subslist as a copy.
		auto subsList = eventSubscribers[event.getEventTypeID()];
		std::cout << "Processing event: " << strFromAction(event.getEventType()) << '\n';
		std::cout << "\tSubscribers are:\n";
		for (auto&& subs : eventSubscribers[event.getEventTypeID()]) {
			std::cout << "\t\t" << subs->id << " at " << subs << '\n';
		}

		for (auto rIt = subsList.rbegin(); rIt != subsList.rend(); ++rIt) {

			(*rIt)->respondToGPIOInterrupt(event);
			if (event.isHandled()) break;
		}

		eventQueue.pop();
	}
}



void Event::Dispatcher::dropNext() {
	m_dropNext = true;
}



void Event::Dispatcher::suspend() {
	m_suspended = true;
}



void Event::Dispatcher::resume() {
	m_suspended = false;
}

} // namespace GPIO
