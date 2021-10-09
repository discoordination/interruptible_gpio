#include "eventDispatcher.hpp"

// EventQueue eventQueue {};

namespace GPIO {

std::unique_ptr<Event::Dispatcher> Event::Dispatcher::dispatcher = {};

Event::Dispatcher& Event::Dispatcher::get() {
	if (dispatcher == nullptr) {
		dispatcher = std::unique_ptr<Dispatcher>(new Dispatcher());
	}
	return *dispatcher;
}

}