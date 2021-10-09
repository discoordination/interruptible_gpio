#ifndef __RESPONDER_HPP_
#define __RESPONDER_HPP_

#include "event.hpp"


class BaseEvent;

/**
 *  This is the class that all items that respond to gpio interrupts should inherit
 * from.  The gpio will call the respondToGPIOInterrupt function on any one object.
 * This can then pass the command up to the parent object.
 */
namespace GPIO {
namespace Event {

class Responder {

protected:
	Responder* nextResponder = nullptr;
public:
	virtual ~Responder() = default;
	// setNextResponder was virtual
//	Responder* setNextResponder(Responder*);
	virtual void respondToGPIOInterrupt(BaseEvent& event) = 0;
};

} // namespace Event
} // namespace GPIO

#endif // __RESPONDER_HPP_