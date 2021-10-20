#ifndef _GPIO_HPP__
#define _GPIO_HPP__

#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "eventDispatcher.hpp"

#include <map>		// For used GPIOs
#include <memory>
#include <array>



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
 * Abstract base class containing virtual functions and static map of all interrupts.
 */
class InterruptibleGPIOBase{

public:
	
	virtual void triggered(uint gpio, uint32_t events) = 0;
	
	/**
	 * Handle the interrupt by calling triggered on the gpio that matches the pin.
	 */
	static void gpioInterruptHandler(uint gpio, uint32_t events);

	/**
 	 *  This is a static map of the GPIOs which contains all used GPIOs.
 	 */
	static inline std::map<uint8_t, InterruptibleGPIOBase*> InterruptibleGPIOs;

protected:

	InterruptibleGPIOBase() = default;
	virtual ~InterruptibleGPIOBase() = default;

	InterruptibleGPIOBase(InterruptibleGPIOBase&&) = delete; // move constructor;
	InterruptibleGPIOBase& operator=(InterruptibleGPIOBase&&) = delete; // move assignment;
	InterruptibleGPIOBase(InterruptibleGPIOBase const&) = delete; // copy constructor;
	InterruptibleGPIOBase& operator=(const InterruptibleGPIOBase&) = delete; // copy assignment;
};




/**
 * Base Class for Interruptible GPIOs
 */
template <uint8_t Pin>
class InterruptibleGPIO : public InterruptibleGPIOBase {
	
public:
	static constexpr uint8_t getPin() { return Pin; }

protected:
	InterruptibleGPIO();
	~InterruptibleGPIO() { InterruptibleGPIOs.erase(Pin); };

};



class PushButtonBase;

template <uint8_t Pin>
class PushButtonGPIO : public InterruptibleGPIO<Pin> {

public:
	enum class ButtonState { NotPressed, Pressed };

	PushButtonGPIO(PushButtonBase* parent, uint debounceMS);
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
	PushButtonGPIO<Pin> buttonGPIO;

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



class RotaryEncoderBase;


template <uint8_t Pin>
class RotaryEncoderEncoderGPIO : public InterruptibleGPIO<Pin> {

	RotaryEncoderBase* parent;
	void triggered(uint gpio, uint32_t events) override;
public:
	RotaryEncoderEncoderGPIO(RotaryEncoderBase* parent);
};



struct EncoderClickTimer {
	
	EncoderClickTimer() : prevClickType(Action::ROTARY_ENCODER_CLOCKWISE_TICK), prevClickTime{to_ms_since_boot(get_absolute_time())} {
		resetTimeBetweenClicks();
	}

	uint16_t getClicksPerSecond() const { 
		uint32_t sum = 0; 
		for (auto& click : timeBetweenClicks) sum += click;
		return 1000 / (sum >> 3);
	}

	void addClick(Action type) {
		
		const auto cTime = to_ms_since_boot(get_absolute_time());
		const auto tChange = cTime - prevClickTime;

		if (tChange > 1100 || type != prevClickType) {
			resetTimeBetweenClicks();
			prevClickType = type;
		}
		
		*inputPtr++ = tChange;
		prevClickTime = cTime;
		if (inputPtr == timeBetweenClicks.end()) inputPtr = timeBetweenClicks.begin();
	}
	
private:
	void resetTimeBetweenClicks() { 
		for(auto& time : timeBetweenClicks) time = 15000;
		inputPtr = timeBetweenClicks.begin();
	}

	std::array<uint32_t, 8> timeBetweenClicks;
	std::array<uint32_t, 8>::iterator inputPtr;
	
	Action prevClickType;
	uint32_t prevClickTime;
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

	constexpr static std::pair<const uint8_t, const uint8_t> getRotaryPins() { return { Pin1, Pin2 }; }
	constexpr static uint8_t getButtonPin() { return ButtonPin; }

protected:
	static uint16_t getStepsFromSpeed(uint16_t spd) {
		return
		(spd < 25) 	? 	1 :
		(spd < 50) 	? 	2 :
		(spd < 75) 	? 	5 :
		(spd < 100) ? 	10 :
		(spd < 125) ? 	25 :
		(spd < 150) ? 	50 :
						100; 
	}

private:
	RotaryEncoderEncoderGPIO<Pin1> p1;
	RotaryEncoderEncoderGPIO<Pin2> p2; 
	PushButton<ButtonPin> button;
	uint8_t state;
	EncoderClickTimer timer;

	void triggered(uint gpio, uint32_t events);
};




// InterruptibleGPIO
template <uint8_t Pin>
InterruptibleGPIO<Pin>::InterruptibleGPIO() {
	static_assert(Pin < 30, "GPIO pin out of range.  RP2040 has only 30 GPIOs");
	InterruptibleGPIOs[Pin] = this;
	gpio_set_dir(Pin, Pico::IN);
}



// PushButtonGPIO
template <uint8_t Pin>
PushButtonGPIO<Pin>::PushButtonGPIO(PushButtonBase* parent, const std::size_t debounceMS) : 
		InterruptibleGPIO<Pin>(),
		buttonState(ButtonState::NotPressed),
		parent(parent),
		debounceMS(debounceMS),
		t(),
		count{0}
{
	gpio_set_irq_enabled_with_callback(Pin, GPIO_IRQ_EDGE_FALL, true, &InterruptibleGPIO<Pin>::gpioInterruptHandler);
	gpio_set_irq_enabled(Pin, GPIO_IRQ_EDGE_RISE, false);
}



template <uint8_t Pin>
bool PushButtonGPIO<Pin>::debounceTimerCallback(repeating_timer_t* t) {

	PushButtonGPIO<Pin>* gpio = static_cast<PushButtonGPIO*>(t->user_data);

	if (gpio_get(gpio->getPin()) == 0 && gpio->buttonState == ButtonState::Pressed) {

		if (++gpio->count == gpio->debounceMS) {
			
			gpio->count = 0;
			gpio_set_irq_enabled_with_callback(gpio->getPin(), GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIOBase::gpioInterruptHandler);
			gpio->parent->buttonDown();

			return false;
		}

	} else if (gpio_get(gpio->getPin()) == 1 && gpio->buttonState == ButtonState::NotPressed) {
		
		if (++gpio->count == gpio->debounceMS) {

			gpio->count = 0;
			gpio_set_irq_enabled_with_callback(gpio->getPin(), GPIO_IRQ_EDGE_FALL, true, &InterruptibleGPIOBase::gpioInterruptHandler);
			gpio->parent->buttonUp();

			return false;
		}

	} else if (gpio->buttonState == ButtonState::Pressed) {

		gpio->buttonState = ButtonState::NotPressed;
		gpio_set_irq_enabled_with_callback(gpio->getPin(), GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIOBase::gpioInterruptHandler);

		return false;

	} else {

		gpio->buttonState = ButtonState::Pressed;
		gpio_set_irq_enabled_with_callback(gpio->getPin(), GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIOBase::gpioInterruptHandler);

		return false;
	}

	return true;
} 


// tell it the button state is what it is.  turn off the interrupt and set an alarm
template <uint8_t Pin>
void PushButtonGPIO<Pin>::triggered(uint gpio, uint32_t events) {

	gpio_set_irq_enabled(Pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, false);

	if (events == GPIO_IRQ_EDGE_FALL) 
		buttonState = ButtonState::Pressed;
	else if (events == GPIO_IRQ_EDGE_RISE)
		buttonState = ButtonState::NotPressed;
	
	// Called every ms and then the gpio keeps count of the number of calls until debounceMS time.
	add_repeating_timer_ms(1, debounceTimerCallback, this, &t);
}




// PushButton

template<uint8_t Pin>
PushButton<Pin>::PushButton(uint longPressTime, uint debounceMS) :	
			buttonGPIO(this, debounceMS),
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





// RotaryEncoderEncoderGPIO

template<uint8_t Pin>
RotaryEncoderEncoderGPIO<Pin>::RotaryEncoderEncoderGPIO(RotaryEncoderBase* parent) : InterruptibleGPIO<Pin>(), parent(parent) {

	gpio_set_irq_enabled_with_callback(Pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIOBase::gpioInterruptHandler);
}



template <uint8_t Pin>
void RotaryEncoderEncoderGPIO<Pin>::triggered(uint gpio, uint32_t events) { 
	parent->triggered(gpio, events); 
}



// RotaryEncoder
namespace {

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
} // namespace ::


template <uint8_t Pin1, uint8_t Pin2, uint8_t ButtonPin>
RotaryEncoder<Pin1, Pin2, ButtonPin>::RotaryEncoder() : 
				p1(this),
				p2(this),
				button(),
				state(R_START)
{}


template <uint8_t Pin1, uint8_t Pin2, uint8_t ButtonPin>
void RotaryEncoder<Pin1, Pin2, ButtonPin>::triggered(uint gpio, uint32_t events) {

	const uint8_t pinstate = gpio_get(p1.getPin()) | (gpio_get(p2.getPin()) << 1);
	state = ttable[state & 0xF][pinstate];
	auto& dispatcher = Event::Dispatcher::get();

	if ((state & 0x30) == DIR_CW) {

		timer.addClick(Action::ROTARY_ENCODER_CLOCKWISE_TICK);
		const auto encoderSpeed = timer.getClicksPerSecond();
		const auto clicks = getStepsFromSpeed(encoderSpeed);
		
		dispatcher.dispatch( std::make_unique<Event::EncoderClockwise<Pin1, Pin2>>(clicks) );

	} else if ((state & 0x30) == DIR_CCW) {

		timer.addClick(Action::ROTARY_ENCODER_CLOCKWISE_TICK);
		const auto encoderSpeed = timer.getClicksPerSecond();
		const auto clicks = getStepsFromSpeed(encoderSpeed);

		dispatcher.dispatch( std::make_unique<Event::EncoderCounterClockwise<Pin1, Pin2>>(clicks));
	}
}

} // namespace GPIO

#endif // _GPIO_HPP__