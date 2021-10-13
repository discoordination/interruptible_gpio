#include "eventDispatcher.hpp"

// EventQueue eventQueue {};

namespace GPIO {

std::unique_ptr<Event::Dispatcher> Event::Dispatcher::dispatcher = nullptr;
//Event::Dispatcher::EventQueue Event::Dispatcher::eventQueue{};
//Event::Dispatcher::SubscriberMap Event::Dispatcher::eventSubscribers{};

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

		eventSubscribers[event].push_back(&target);
}



void Event::Dispatcher::unSubscribe(eventId event, Responder& target) {
	auto& sList = eventSubscribers[event];
	#ifdef IG_DEBUG
		std::cout << "Responder: " << target.id << " unsubscribing from event: " << event << "\n";
	#endif
	for (auto it = sList.begin(); it != sList.end();) {
		if (*it == &target)
			it = sList.erase(it);
		else ++it;
	}
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
		auto& subsList = eventSubscribers[event.getEventTypeID()];

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
