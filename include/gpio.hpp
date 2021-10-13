#ifndef _GPIO_HPP__
#define _GPIO_HPP__


#include "pico/stdlib.h"
#include "hardware/irq.h"

#include "eventDispatcher.hpp"

#include <map>		// For used GPIOs
#include <memory>


// The library now uses a delayed (through an event queue) subscriber event system.
// GPIOs create an event and send it to the dispatcher which queues it before sending
// it to the subscriber.


namespace Pico {
	inline constexpr bool OUT = true;
	inline constexpr bool IN = false;
}

class Responder;

namespace GPIO {

/**
 * Base Class for Interruptible GPIOs
 */
class InterruptibleGPIO {
	
public:
	constexpr uint8_t getPin() const { return pin; }
	
	/**
	 * Handle the interrupt by calling triggered on the gpio that matches the pin.
	 */
	static void gpioInterruptHandler(uint gpio, uint32_t events);

protected:
	const uint8_t pin;

	InterruptibleGPIO(const uint8_t pin);
	~InterruptibleGPIO() { InterruptibleGPIOs.erase(pin); };

	InterruptibleGPIO(const InterruptibleGPIO&); // copy constructor;
	InterruptibleGPIO& operator=(const InterruptibleGPIO& other); // copy assignment;
	InterruptibleGPIO(InterruptibleGPIO&&) = delete; // move constructor;
	InterruptibleGPIO& operator=(InterruptibleGPIO&& other) = delete; // move assignment;

// This is a static map of the GPIOs which contains all used GPIOs.
	inline static std::map<uint8_t, InterruptibleGPIO*> InterruptibleGPIOs;
	virtual void triggered(uint gpio, uint32_t events) = 0;
};



class PushButtonBase;


class PushButtonGPIO : public InterruptibleGPIO {

public:
	enum class ButtonState { NotPressed, Pressed };

	PushButtonGPIO(const uint8_t pin, PushButtonBase* parent, uint debounceMS);
	void triggered(uint gpio, uint32_t events) override;
	
	ButtonState buttonState;

private:
	PushButtonBase* parent;
	const std::size_t debounceMS;
	repeating_timer t;
	uint count;

	static bool debounceTimerCallback(repeating_timer_t *t);
};



class PushButtonBase {
	
public:
	virtual void buttonUp() = 0;
	virtual void buttonDown() = 0;

	virtual std::size_t getButtonDownEventID() const = 0;
	virtual std::size_t getButtonUpEventID() const = 0;
	virtual std::size_t getButtonLPEventID() const = 0;

protected:
	virtual ~PushButtonBase() = default;
};



template <uint8_t Pin>
class PushButton : PushButtonBase {

private:
	PushButtonGPIO buttonGPIO;

	uint longPressTime;
	alarm_id_t longPressAlarmID;
	static int64_t longPressCallback(alarm_id_t id, void* userData);
public:
	using ButtonDownEventType = Event::ButtonDown<Pin>;
	using ButtonUpEventType = Event::ButtonUp<Pin>;
	using ButtonLPEventType = Event::ButtonLongPress<Pin>;

	PushButton(uint longPressTime = 1500, uint debounceMS = 5);

	void buttonUp();
	void buttonDown();

	std::size_t getButtonDownEventID() const override { 
		return EventID::value<ButtonDownEventType>();
	}
	std::size_t getButtonUpEventID() const override { 
		return EventID::value<ButtonUpEventType>(); 
	}
	std::size_t getButtonLPEventID() const override {
		return EventID::value<ButtonLPEventType>(); 
	}
};



class RotaryEncoderBase {
	
public:
	virtual void triggered(uint gpio, uint32_t events) = 0;

	virtual std::size_t getButtonDownEventID() const = 0;
	virtual std::size_t getButtonUpEventID() const = 0;
	virtual std::size_t getEncoderCEventID() const = 0;
	virtual std::size_t getEncoderCCEventID() const = 0;
	virtual std::size_t getButtonLPEventID() const = 0;

protected:
	virtual ~RotaryEncoderBase() = default;
};



class RotaryEncoderEncoderGPIO : public InterruptibleGPIO {

	RotaryEncoderBase* parent;
	void triggered(uint gpio, uint32_t events) override;
public:
	RotaryEncoderEncoderGPIO(uint8_t pin, RotaryEncoderBase* parent);
};



template <uint8_t Pin1, uint8_t Pin2, uint8_t ButtonPin = 255>
class RotaryEncoder : public RotaryEncoderBase {

public:
	RotaryEncoder();
	void buttonDown();
	void buttonUp();

	using CEventType = Event::EncoderClockwise<Pin1, Pin2>;
	using CCEventType = Event::EncoderCounterClockwise<Pin1, Pin2>;
	using ButtonDownEventType = Event::ButtonDown<ButtonPin>;
	using ButtonUpEventType = Event::ButtonUp<ButtonPin>;
	using ButtonLPEventType = Event::ButtonLongPress<ButtonPin>;

	std::size_t getButtonDownEventID() const override {
		return EventID::value<ButtonDownEventType>();
	}
	std::size_t getButtonUpEventID() const override {
		return EventID::value<ButtonUpEventType>();
	}
	std::size_t getEncoderCEventID() const override {
		return EventID::value<CEventType>();
	}
	std::size_t getEncoderCCEventID() const override {
		return EventID::value<CCEventType>();
	}
	std::size_t getButtonLPEventID() const override {
		return EventID::value<ButtonLPEventType>();
	}

	constexpr static std::pair<const uint8_t, const uint8_t> getRotaryPins() { return {Pin1, Pin2 }; }
	constexpr static uint8_t getButtonPin() { return ButtonPin; }

///	const uint8_t pin1, pin2;

private:
	RotaryEncoderEncoderGPIO p1;
	RotaryEncoderEncoderGPIO p2; 
	PushButton<ButtonPin> button;
	uint8_t state;
	void triggered(uint gpio, uint32_t events);
};



// PushButton
template<uint8_t Pin>
PushButton<Pin>::PushButton(uint longPressTime, uint debounceMS) :	
			buttonGPIO(Pin, this, debounceMS),
			longPressTime{longPressTime}
{}



template<uint8_t Pin>
void PushButton<Pin>::buttonUp() {
	cancel_alarm(longPressAlarmID);
	Event::Dispatcher::get().dispatch( std::make_unique<Event::ButtonUp<Pin>>() );
}


template <uint8_t Pin>
void PushButton<Pin>::buttonDown() {
	volatile int i = 1;
	longPressAlarmID = add_alarm_in_ms(longPressTime, &longPressCallback, this, true);
	Event::Dispatcher::get().dispatch( std::make_unique<Event::ButtonDown<Pin>>() );
}

/**
 * \todo \bug Check if discriminates against different pushButtons.
 */
template <uint8_t Pin>
int64_t PushButton<Pin>::longPressCallback(alarm_id_t id, void* userData) {
	Event::Dispatcher::get().dispatch( std::make_unique<Event::ButtonLongPress<Pin>>() );
	return 0;
}


namespace {
// RotaryEncoder

constexpr uint8_t DIR_NONE		{ 0x00 };
constexpr uint8_t DIR_CW		{ 0x10 };
constexpr uint8_t DIR_CCW		{ 0x20 };

// Use the full-step state table (emits a code at 00 only)
constexpr uint8_t R_START		{ 0x0 }; // 0b 0000
constexpr uint8_t R_CW_FINAL 	{ 0x1 }; // 0b 0001
constexpr uint8_t R_CW_BEGIN 	{ 0x2 }; // 0b 0010
constexpr uint8_t R_CW_NEXT 	{ 0x3 }; // 0b 0011
constexpr uint8_t R_CCW_BEGIN 	{ 0x4 }; // 0b 0100
constexpr uint8_t R_CCW_FINAL 	{ 0x5 }; // 0b 0101
constexpr uint8_t R_CCW_NEXT 	{ 0x6 }; // 0b 0110


const std::array<std::array<uint8_t, 4>, 7> ttable {
  // R_START
	std::array<uint8_t, 4>{ !R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START },
  // R_CW_FINAL
    std::array<uint8_t, 4>{ R_CW_NEXT, R_START, R_CW_FINAL, R_START | DIR_CW },
  // R_CW_BEGIN
    std::array<uint8_t, 4>{ R_CW_NEXT, R_CW_BEGIN, R_START,   R_START },
  // R_CW_NEXT
    std::array<uint8_t, 4>{ R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START },
  // R_CCW_BEGIN
    std::array<uint8_t, 4>{ R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START },
  // R_CCW_FINAL
    std::array<uint8_t, 4>{ R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | DIR_CCW },
  // R_CCW_NEXT
    std::array<uint8_t, 4>{ R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START }
};

}

template <uint8_t Pin1, uint8_t Pin2, uint8_t ButtonPin>
RotaryEncoder<Pin1, Pin2, ButtonPin>::RotaryEncoder() : 
				p1(Pin1, this),
				p2(Pin2, this),
				button(),
				state(R_START)
{}


template <uint8_t Pin1, uint8_t Pin2, uint8_t ButtonPin>
void RotaryEncoder<Pin1, Pin2, ButtonPin>::triggered(uint gpio, uint32_t events) {

	uint8_t pinstate = gpio_get(p1.getPin()) | (gpio_get(p2.getPin()) << 1);
	state = ttable[state & 0xF][pinstate];

	if ((state & 0x30) == DIR_CW) {
		Event::Dispatcher::get().dispatch( std::make_unique<Event::EncoderClockwise<Pin1, Pin2>>() );

	} else if ((state & 0x30) == DIR_CCW) {
		Event::Dispatcher::get().dispatch( std::make_unique<Event::EncoderCounterClockwise<Pin1, Pin2>>() );
	}
}

} // namespace GPIO

#endif // _GPIO_HPP__