# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PWM_SPI_Sub C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(PWM_SPI_Sub
    main.c 
    ${CMAKE_SOURCE_DIR}/Libraries/Utility/data_to_byte/src/data_to_byte.c
    ${CMAKE_SOURCE_DIR}/Libraries/Utility/statistic/src/statistic.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/ADC/src/adc.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/SPI/src/spi.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/UART/src/uart.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/PWM/src/pwm.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/FLASH/src/flash.c
    ${CMAKE_SOURCE_DIR}/Libraries/Hardware/CLOCK/src/clock.c
)

pico_set_program_name(PWM_SPI_Sub "PWM_SPI_Sub")
pico_set_program_version(PWM_SPI_Sub "0.1")

# Add the standard library to the build
target_link_libraries(PWM_SPI_Sub
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(PWM_SPI_Sub PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)


# Add any user requested libraries
target_link_libraries(PWM_SPI_Sub 
    pico_stdlib pico_util pico_time pico_rand pico_binary_info hardware_timer hardware_clocks hardware_pio hardware_uart hardware_irq hardware_spi 
    hardware_adc hardware_dma hardware_claim hardware_base hardware_pwm hardware_divider hardware_watchdog hardware_rtc hardware_pll
)


# Add the directories for CMAKE to search for header-files
target_include_directories(PWM_SPI_Sub PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Utility/data_to_byte
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Utility/statistic
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/ADC
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/SPI
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/UART
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/PWM
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/FLASH
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Hardware/CLOCK
)

pico_add_extra_outputs(PWM_SPI_Sub)

# Output information about program size and memory locations for main
add_custom_command(TARGET PWM_SPI_Sub
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Start program size and memory-address information for main:"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND arm-none-eabi-size -A -x --common -t  $<TARGET_FILE:PWM_SPI_Sub>
    COMMAND ${CMAKE_COMMAND} -E echo "End program size and memory-address information for main" 
)


pico_add_extra_outputs(PWM_SPI_Sub)

