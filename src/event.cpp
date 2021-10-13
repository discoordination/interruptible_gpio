#include "event.hpp"

size_t EventID::counter = 1;

#ifdef IG_DEBUG
namespace GPIO::Event {

std::string strFromAction(const Action a) {
	switch (a) {
		case Action::NONE: return "None";
		case Action::BASE_TYPE: return "BaseEvent";
		case Action::PUSH_BUTTON_DOWN: return "PushButtonDown";
		case Action::PUSH_BUTTON_UP: return "PushButtonUp";
		case Action::PUSH_BUTTON_LONG_PRESS: return "PushButtonLongPress";
		case Action::PUSH_BUTTON_DOUBLE_TAP: return "PushButtonDblTap";
		case Action::ROTARY_ENCODER_CLOCKWISE_TICK: return "RotaryCTick";
		case Action::ROTARY_ENCODER_COUNTERCLOCKWISE_TICK: return "RotaryCCTick";
		default: return "";
	};
}

} // namespace GPIO::Event
#endif // IG_DEBUG