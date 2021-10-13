#ifndef __RESPONDER_HPP_
#define __RESPONDER_HPP_

#include "event.hpp"
#include <set>
#include <limits>

class BaseEvent;

/**
 *  This is the class that all items that respond to gpio interrupts should inherit
 * from.  The gpio will call the respondToGPIOInterrupt function on any one object.
 * This can then pass the command up to the parent object.
 */
namespace GPIO {
namespace Event {

// Unique_id
class UniqueID {

public:
	const std::size_t id;

protected:
	UniqueID() : id(UniqueID::getNextID()) {}
	~UniqueID() { ids.erase(id); };

private:
	static std::size_t getNextID() { 		
		for (std::size_t i = 1; i < std::numeric_limits<std::size_t>::max(); ++i) {
			auto elem = ids.find(i);
			if(elem == std::end(ids)) { 
				ids.insert(i);
				return i;
			}
		}
	#ifdef IG_DEBUG
		std::cout << "Crashing due to too many items.\n";
		std::exit(EXIT_FAILURE);
	#endif
		return -1; // Impossible and can never get here.
	}
	static std::set<std::size_t> ids;
};



class Responder : public UniqueID {

public:
	virtual ~Responder() = default;
	virtual void respondToGPIOInterrupt(BaseEvent& event) = 0;
};

} // namespace Event
} // namespace GPIO

#endif // __RESPONDER_HPP_