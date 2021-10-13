
#include "gpio.hpp"
#include <iostream>
#include <cassert>

namespace GPIO {

// InterruptibleGPIO

InterruptibleGPIO::InterruptibleGPIO(const uint8_t pin) : 
										pin(pin) {
	assert(pin < 30 && "GPIO pin out of range.  RP2040 has only 30 GPIOs");
	InterruptibleGPIOs[pin] = this;
	gpio_set_dir(pin, Pico::IN);
}


InterruptibleGPIO& InterruptibleGPIO::operator=(const InterruptibleGPIO& other) {
	std::cout << "Called copy assignment operator" << std::endl;
	assert(1 && "This should not be called.");
	return *this;
}


InterruptibleGPIO::InterruptibleGPIO(const InterruptibleGPIO& other) : InterruptibleGPIO(other.pin) {
	std::cout << "Called copy constructor" << std::endl;
	assert(1 && "This should not be called.");;
}


void InterruptibleGPIO::gpioInterruptHandler(uint gpio, uint32_t events) {
	const auto& t = InterruptibleGPIOs.find(gpio);
	if (t != std::end(InterruptibleGPIOs))
		t->second->triggered(gpio, events);
}



// PushButtonGPIO
PushButtonGPIO::PushButtonGPIO(const uint8_t pin, PushButtonBase* parent, const std::size_t debounceMS) : 
		InterruptibleGPIO(pin),
		buttonState(ButtonState::NotPressed),
		parent(parent),
		debounceMS(debounceMS),
		t(),
		count(0) 
{
	gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &InterruptibleGPIO::gpioInterruptHandler);
	gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, false);
}



bool PushButtonGPIO::debounceTimerCallback(repeating_timer_t* t) {

	PushButtonGPIO* gpio = static_cast<PushButtonGPIO*>(t->user_data);

	if (gpio_get(gpio->pin) == 0 && gpio->buttonState == ButtonState::Pressed) {

		if (++gpio->count == gpio->debounceMS) {
			
			gpio->count = 0;
			gpio_set_irq_enabled_with_callback(gpio->pin, GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIO::gpioInterruptHandler);
			gpio->parent->buttonDown();

			return false;
		}

	} else if (gpio_get(gpio->pin) == 1 && gpio->buttonState == ButtonState::NotPressed) {
		
		if (++gpio->count == gpio->debounceMS) {

			gpio->count = 0;
			gpio_set_irq_enabled_with_callback(gpio->pin, GPIO_IRQ_EDGE_FALL, true, &InterruptibleGPIO::gpioInterruptHandler);
			gpio->parent->buttonUp();

			return false;
		}

	} else if (gpio->buttonState == ButtonState::Pressed) {

		gpio->buttonState = ButtonState::NotPressed;
		gpio_set_irq_enabled_with_callback(gpio->pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIO::gpioInterruptHandler);

		return false;

	} else {

		gpio->buttonState = ButtonState::Pressed;
		gpio_set_irq_enabled_with_callback(gpio->pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIO::gpioInterruptHandler);

		return false;
	}

	return true;
} 


// tell it the button state is what it is.  turn off the interrupt and set an alarm
void PushButtonGPIO::triggered(uint gpio, uint32_t events) {

	gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, false);

	if (events == GPIO_IRQ_EDGE_FALL) 
		buttonState = ButtonState::Pressed;
	else if (events == GPIO_IRQ_EDGE_RISE)
		buttonState = ButtonState::NotPressed;
	
	// Called every ms and then the gpio keeps count of the number of calls until debounceMS time.
	add_repeating_timer_ms(1, debounceTimerCallback, this, &t);
}



// RotaryEncoderEncoderGPIO

RotaryEncoderEncoderGPIO::RotaryEncoderEncoderGPIO(uint8_t pin, RotaryEncoderBase* parent) : InterruptibleGPIO(pin), parent(parent) {

	gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL + GPIO_IRQ_EDGE_RISE, true, &InterruptibleGPIO::gpioInterruptHandler);
}

void RotaryEncoderEncoderGPIO::triggered(uint gpio, uint32_t events) { 
	parent->triggered(gpio, events); 
}

} // namespace GPIO
