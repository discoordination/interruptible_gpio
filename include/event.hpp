#ifndef __EVENT_HPP_
#define __EVENT_HPP_

#include <cstdint>
#include <vector>



// Only use in this file.
//namespace {

/**
 * Produces a unique_id for every used event.  Automatic for classes which inherit from this.
 */
class EventID {
	static size_t counter;
public:
	template <typename T>
	static size_t value() { static size_t id = counter++; return id; }
};
//}



namespace GPIO {
	
/**
 *	Action:  These describe the type of GPIO action. 
 */
	enum class Action {
		none = 0,
		BASE_TYPE,
		PUSH_BUTTON_DOWN,
		PUSH_BUTTON_UP,
		PUSH_BUTTON_LONG_PRESS,
		PUSH_BUTTON_DOUBLE_TAP,
		ROTARY_ENCODER_CLOCKWISE_TICK,
		ROTARY_ENCODER_COUNTERCLOCKWISE_TICK
	};



namespace Event {

using Type = GPIO::Action;

/**
 * Abstract base Event class
 */
class BaseEvent {

public:
	virtual ~BaseEvent() = default;
	BaseEvent (BaseEvent&& other) = default;
	BaseEvent& operator=(BaseEvent&& other) = default;

	virtual bool isButtonEvent() const = 0;
	virtual bool isEncoderEvent() const = 0;
	
	void setHandled() { handled = true; }
	bool isHandled() { return handled; }

	virtual Type getEventType() const = 0;
	virtual size_t getEventTypeID() const = 0;

protected:
	BaseEvent() {}
	static Type getStaticType() { return Type::BASE_TYPE; }

private:
	bool handled = false;
};



template<uint8_t Pin>
class Button : public BaseEvent {

public:
	uint16_t getPin() const { return pin; }

	bool isButtonEvent() const override { return true; }
	bool isEncoderEvent() const override { return false; }

	virtual bool isButtonDown() const = 0;
	virtual bool isButtonUp() const = 0;
	virtual bool isButtonLongPress() const = 0;

protected:
	Button() = default;

private:
	uint8_t pin = Pin;
};



template<uint8_t Pin>
class ButtonUp final : public Button<Pin> {

public:
	bool isButtonDown() const override { return false; }
	bool isButtonUp() const override { return true; }
	bool isButtonLongPress() const override { return false; }

	Type getEventType() const { return ButtonUp::getStaticType(); }
	size_t getEventTypeID() const override { return EventID::value<ButtonUp<Pin>>(); }
	static Type getStaticType() { return Type::PUSH_BUTTON_UP; }
};



template<uint8_t Pin>
class ButtonDown final : public Button<Pin> {

public:
	bool isButtonDown() const override { return true; }
	bool isButtonUp() const override { return false; }
	bool isButtonLongPress() const override { return false; }

	Type getEventType() const override { return ButtonDown::getStaticType(); }
	size_t getEventTypeID() const override { return EventID::value<ButtonDown<Pin>>(); }
	static Type getStaticType() { return Type::PUSH_BUTTON_UP; }
};



template<uint8_t Pin>
class ButtonLongPress final : public Button<Pin> {

public:
	bool isButtonDown() const override { return false; }
	bool isButtonUp() const override { return false; }
	bool isButtonLongPress() const override { return true; }

	Type getEventType() const override { return ButtonLongPress::getStaticType(); }
	size_t getEventTypeID() const override { return EventID::value<ButtonLongPress<Pin>>(); }
	static Type getStaticType() { return Type::PUSH_BUTTON_LONG_PRESS; }
};



template<uint8_t Pin1, uint8_t Pin2>
class Encoder : public BaseEvent {

	static_assert(Pin1 < Pin2, "Pin1 must always be less than Pin2 to avoid duplicate types.");

public:
	std::pair<uint8_t, uint8_t> getPins() const { return pins; } 

	bool isButtonEvent() const override { return false; }
	bool isEncoderEvent() const override { return true; }
	virtual bool isClockwise() const = 0;
	virtual bool isCounterClockwise() const = 0;
	
protected:
	Encoder() = default;

private:
	std::pair<uint8_t, uint8_t> pins = { (Pin1 < Pin2) ? Pin1, Pin2 : Pin2, Pin1 };
};



template<const uint8_t Pin1, const uint8_t Pin2>
class EncoderClockwise final : public Encoder<Pin1, Pin2> {

public:
	bool isClockwise() const override { return true; }
	bool isCounterClockwise() const override { return false; }

	Type getEventType() const override { return EncoderClockwise::getStaticType(); }
	size_t getEventTypeID() const override { return EventID::value<EncoderClockwise<Pin1, Pin2>>(); }
	static Type getStaticType() { return Type::ROTARY_ENCODER_CLOCKWISE_TICK; }
};



template<uint8_t Pin1, uint8_t Pin2>
class EncoderCounterClockwise final : public Encoder<Pin1, Pin2> {

	static_assert((Pin1 < Pin2), "Pin1 must always be less than Pin2 to avoid duplicate types."); 

public:
	bool isClockwise() const override { return false; }
	bool isCounterClockwise() const override { return true; }

	Type getEventType() const override { return EncoderCounterClockwise::getStaticType(); }
	size_t getEventTypeID() const override { return EventID::value<EncoderCounterClockwise<Pin1, Pin2>>(); }
	static Type getStaticType() { return Type::ROTARY_ENCODER_COUNTERCLOCKWISE_TICK; }
};

} // namespace Event
} // namespace GPIO

// namespace std {
// 	template<uint16_t Pin1>
// 	struct hash<GPIO::Event::ButtonDown<Pin1>> {
// 		size_t operator()(const GPIO::Event::ButtonDown<Pin1>& e) const {
// 			return (hash<size_t>()(e.getEventTypeID()));
// 		}
// 	};

// 	template<uint16_t Pin1>
// 	struct hash<GPIO::Event::ButtonUp<Pin1>> {
// 		size_t operator()(const GPIO::Event::ButtonDown<Pin1>& e) const {
// 			return (hash<size_t>()(e.getEventTypeID()));
// 		}
// 	};

// 	template<uint16_t Pin1, uint16_t Pin2>
// 	struct hash<GPIO::Event::EncoderClockwise<Pin1, Pin2>> {
// 		size_t operator()(const GPIO::Event::EncoderClockwise<Pin1,Pin2>& e) const {
// 			return (hash<size_t>()(e.getEventTypeID()));
// 		}
// 	};

// 	template<uint16_t Pin1, uint16_t Pin2>
// 	struct hash<GPIO::Event::EncoderCounterClockwise<Pin1, Pin2>> {
// 		size_t operator()(const GPIO::Event::EncoderCounterClockwise<Pin1,Pin2>& e) const {
// 			return (hash<size_t>()(e.getEventTypeID()));
// 		}
// 	};
// }

#endif // __EVENT_HPP_