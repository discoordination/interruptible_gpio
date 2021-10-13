
add_library(	${lib_name}
				STATIC
				gpio.cpp
				responder.cpp
				event.cpp
				eventDispatcher.cpp
				../include/dbgEnable.hpp
				../include/gpio.hpp
				../include/interruptible_gpio.hpp
				../include/event.hpp
				../include/eventDispatcher.hpp
				../include/responder.hpp
)


target_include_directories	(	${lib_name}
#								"${CMAKE_CURRENT_SOURCE_DIR}"
								PUBLIC
								../include
)

target_link_libraries	( 	${lib_name}
							PRIVATE 
							pico_stdlib
							hardware_irq 
)

