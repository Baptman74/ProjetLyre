cmake_minimum_required(VERSION 3.18)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(sound_to_lightV1 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


add_executable (sound_to_lightV1
	sound_to_lightV1.cpp
	kiss_fft.c
	kiss_fftr.c
	)

target_link_libraries(sound_to_lightV1
  pico_stdlib
	hardware_dma
	pico_time
	hardware_clocks
	pico_runtime
	pico_standard_link
	hardware_adc
)

pico_enable_stdio_usb(sound_to_lightV1
	1
	)
pico_enable_stdio_uart(sound_to_lightV1
	0
	)

pico_add_extra_outputs(sound_to_lightV1)
