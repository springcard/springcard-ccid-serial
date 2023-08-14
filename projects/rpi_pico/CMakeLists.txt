cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ccid-serial_project)

set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(ccid-serial
	../../src/sample/rpi_pico/pcsc-serial-sample-main-rpi_pico.c
	../../src/sample/pcsc-serial-sample.c
	../../src/hal/rpi_pico/rpi_pico_hal.c	
	../../src/ccid/ccid_convert.c
	../../src/ccid/ccid_exchange.c
	../../src/ccid/ccid_helpers.c
	../../src/ccid/ccid_serial_receiver.c
	../../src/ccid/ccid_serial_sender.c
	../../src/scard/scard_core.c
	../../src/scard/scard_helpers.c
)

add_compile_options(
	-Wextra
	-Wall
	-Werror	
    -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -Wno-unused-function # we have some for the docs that aren't called
    -Wno-maybe-uninitialized
    -fpermissive
)
		
pico_enable_stdio_usb(ccid-serial 1)
pico_enable_stdio_uart(ccid-serial 0)

pico_add_extra_outputs(ccid-serial)

target_include_directories(ccid-serial PUBLIC
	./
)

target_link_libraries(ccid-serial
	pico_stdlib
	pico_cyw43_arch_none
	hardware_uart
)
