
#include "gpio.hpp"
#include <iostream>
#include <cassert>

namespace GPIO {


// InterruptibleGPIOBase

void InterruptibleGPIOBase::gpioInterruptHandler(uint gpio, uint32_t events) {
	const auto& t = InterruptibleGPIOs.find(gpio);
	if (t != std::end(InterruptibleGPIOs))
		t->second->triggered(gpio, events);
}


} // namespace GPIO
