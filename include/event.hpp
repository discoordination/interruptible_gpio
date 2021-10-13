#ifndef __EVENT_HPP_
#define __EVENT_HPP_

#include "dbgEnable.hpp"
#include <cstdint>
#include <vector>

#ifdef IG_DEBUG
#include <string>
#include <iostream>
#endif


/**
 * Produces a unique_id for every used event.  Automatic for classes which inherit from this.
 */
class EventID {
	static size_t counter;
public:
	template <typename T>
	static size_t value() { static size_t id = counter++; return id; }
};


namespace GPIO {
	
/**
 *	Action:  These describe the type of GPIO action. 
 */
	enum class Action {
		NONE = 0,
		BASE_TYPE,
		PUSH_BUTTON_DOWN,
		PUSH_BUTTON_UP,
		PUSH_BUTTON_LONG_PRESS,
		PUSH_BUTTON_DOUBLE_TAP,
		ROTARY_ENCODER_CLOCKWISE_TICK,
		ROTARY_ENCODER_COUNTERCLOCKWISE_TICK
	};
	

namespace Event {

#ifdef IG_DEBUG
	std::string strFromAction(const Action a);
#endif // IG_DEBUG


using Type = GPIO::Action;
class Dispatcher;	// Forward declared so we can make it a friend of event.
/**
 * Abstract base Event class
 */
class BaseEvent {
	// I want to give Dispatcher free access to event.
	friend class Dispatcher;

public:
	virtual ~BaseEvent() = default;
	BaseEvent (BaseEvent&& other) = default;
	BaseEvent& operator=(BaseEvent&& other) = default;

	/**
	 * Query if button event.
	 * \bug May not be worth maintaining.
	 */
	virtual bool isButtonEvent() const = 0;
	/**
	 * Query if encoder event.
	 * \bug May be worth removing.
	 */
	virtual bool isEncoderEvent() const = 0;
	
	/**
	 * Sets the event as handled so does not propagate to next responder.
	 */
	void setHandled() { handled = true; }

	/**
	 * Get the Type of the event.  This is an GPIO::Event::Type or GPIO::Action.
	 */
	virtual Type getEventType() const = 0;

	/**
	 * Get the unique identifier for the class of the event.
	 */
	virtual size_t getEventTypeID() const = 0;

#ifdef IG_DEBUG
	friend std::ostream& operator<<(std::ostream& os, const BaseEvent& e) {
		return e.print(os);
	}
#endif // IG_DEBUG

protected:
	BaseEvent() {}

	/**
	 * Query if the event has been handled.
	 */ 
	bool isHandled() { return handled; }

	static Type getStaticType() { return Type::BASE_TYPE; }

#ifdef IG_DEBUG
	virtual std::ostream& print(std::ostream& os) const {
		os << strFromAction(getEventType()) << " id:(" << getEventTypeID() << ")"; 
		return os;
	}
#endif

private:
	bool handled = false;
};



template<uint8_t Pin>
class Button : public BaseEvent {

public:
	static constexpr uint8_t getPin() { return Pin; }

	constexpr bool isButtonEvent() const override { return true; }
	constexpr bool isEncoderEvent() const override { return false; }

	virtual bool isButtonDown() const = 0;
	virtual bool isButtonUp() const = 0;
	virtual bool isButtonLongPress() const = 0;


protected:
	Button() = default;

#ifdef IG_DEBUG
	std::ostream& print(std::ostream&) const override;
#endif

private:
	static constexpr uint8_t pin = Pin;
};



template<uint8_t Pin>
class ButtonUp final : public Button<Pin> {

public:
	constexpr bool isButtonDown() const override { return false; }
	constexpr bool isButtonUp() const override { return true; }
	constexpr bool isButtonLongPress() const override { return false; }

	constexpr Type getEventType() const { return ButtonUp::getStaticType(); }
	constexpr size_t getEventTypeID() const override { return EventID::value<ButtonUp<Pin>>(); }

	constexpr static Type getStaticType() { return Type::PUSH_BUTTON_UP; }
};




template<uint8_t Pin>
class ButtonDown final : public Button<Pin> {

public:
	constexpr bool isButtonDown() const override { return true; }
	constexpr bool isButtonUp() const override { return false; }
	constexpr bool isButtonLongPress() const override { return false; }

	constexpr Type getEventType() const override { return ButtonDown::getStaticType(); }
	constexpr size_t getEventTypeID() const override { return EventID::value<ButtonDown<Pin>>(); }
	
	static Type getStaticType() { return Type::PUSH_BUTTON_DOWN; }
};



template<uint8_t Pin>
class ButtonLongPress final : public Button<Pin> {

public:
	constexpr bool isButtonDown() const override { return false; }
	constexpr bool isButtonUp() const override { return false; }
	constexpr bool isButtonLongPress() const override { return true; }

	constexpr Type getEventType() const override { return ButtonLongPress::getStaticType(); }
	constexpr size_t getEventTypeID() const override { return EventID::value<ButtonLongPress<Pin>>(); }

	static Type getStaticType() { return Type::PUSH_BUTTON_LONG_PRESS; }
};



template<uint8_t Pin1, uint8_t Pin2>
class Encoder : public BaseEvent {

	static_assert(Pin1 < Pin2, "Pin1 must always be less than Pin2 to avoid duplicate types.");

public:
	static constexpr std::pair<uint8_t, uint8_t> getPins() { return pins; } 

	constexpr bool isButtonEvent() const override { return false; }
	constexpr bool isEncoderEvent() const override { return true; }

	virtual bool isClockwise() const = 0;
	virtual bool isCounterClockwise() const = 0;
	

#ifdef IG_DEBUG
	std::ostream& print(std::ostream& os) const override;
#endif

protected:
	Encoder() = default;

private:
	static constexpr std::pair<uint8_t, uint8_t> pins = { (Pin1 < Pin2) ? Pin1, Pin2 : Pin2, Pin1 };
};



template<const uint8_t Pin1, const uint8_t Pin2>
class EncoderClockwise final : public Encoder<Pin1, Pin2> {

public:
	constexpr bool isClockwise() const override { return true; }
	constexpr bool isCounterClockwise() const override { return false; }

	constexpr Type getEventType() const override { return EncoderClockwise::getStaticType(); }
	constexpr size_t getEventTypeID() const override { return EventID::value<EncoderClockwise<Pin1, Pin2>>(); }

	static Type getStaticType() { return Type::ROTARY_ENCODER_CLOCKWISE_TICK; }
};



template<uint8_t Pin1, uint8_t Pin2>
class EncoderCounterClockwise final : public Encoder<Pin1, Pin2> {

	static_assert((Pin1 < Pin2), "Pin1 must always be less than Pin2 to avoid duplicate types."); 

public:
	constexpr bool isClockwise() const override { return false; }
	constexpr bool isCounterClockwise() const override { return true; }

	constexpr Type getEventType() const override { return EncoderCounterClockwise::getStaticType(); }
	constexpr size_t getEventTypeID() const override { 
		return EventID::value<EncoderCounterClockwise<Pin1, Pin2>>(); 
	}

	static Type getStaticType() { return Type::ROTARY_ENCODER_COUNTERCLOCKWISE_TICK; }
};

#ifdef IG_DEBUG

	template <uint8_t Pin>
	std::ostream& Button<Pin>::print(std::ostream& os) const {
		BaseEvent::print(os);
		os << " pin: " << static_cast<uint16_t>(Pin);
		return os;
	} 

	template <uint8_t Pin1, uint8_t Pin2>
	std::ostream& Encoder<Pin1, Pin2>::print(std::ostream& os) const {
		BaseEvent::print(os);
		os << " pin1: " << static_cast<uint16_t>(Pin1) << ", pin2: " << static_cast<uint16_t>(Pin2);
		return os;
	} 

#endif // IG_DEBUG


} // namespace Event
} // namespace GPIO

#endif // __EVENT_HPP_