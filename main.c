// Project:  ADC_SPI_TEST
// Author: Jonas Moosbrugger
// File: main.c

/*  Description:

    New test setup to test the microcontroller functionality as well as the 
    impact of these functions inside an 3T-MR-Scanner. It simulates:
        - Reading a analog voltage via ADC.
        - Reading SPI Sensor/ADC.
    
    Both test run in parallel, using both cores and DMA functionality. 
    The result is printed via UART. Furthermore the system is controlled via 
    UART. The UART connection is optical. To simulate the SPI Sensor/ADC and to 
    create a changing voltage a second raspberry pi pico (RP2040) is used. 
    The two controllers are called main and sub (like SPI hierarchical structure). 
    The function implemented on each of them is:

    - Main:
        - ADC reading and dataprocessing
        - SPI main sending data and reading data.
        - Control and data readout via UART.
    - Sub:
        - Creating analog voltage via PWM and hardware 
          first order passive low pass filter.
        - SPI sub receiving and sending data back to main. 

*/

/*
    //TODO: Improve PWM functionality such that duty cycle values are
            written by DMA to PWM hardware - no work for CPU to do.
            In the mean time handle SPI Data.

            Probably implement also multi core here:

            Core0 is responsible to get user commands, send commands to core1
            and generate the analog voltage ramp, through changing the duty
            cycle of the PWM signal.

            Core1 is responsible to control SPI transmission.
            Maybe implement a communication protocol, to 
            synchronize sender and receiver. Also enable
            SPI transmit via DMA.
*/

// Includes:

// Standard library:
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

// Pico library:
#include "pico/stdlib.h"

#include "pico/multicore.h"
#include "hardware/irq.h"

#include "hardware/timer.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

#include "hardware/spi.h"

// Own library:
#include "uart.h"
#include "pwm.h"

// Preprocessor:

// UART:

#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 250*KHZ //250 kBd

// PWM:
#define PWM_PIN 8
#define PWM_CLK_FREQUENCY 125*MHZ
#define PWM_FREQUENCY 100*KHZ


// SPI:

#define SPI_ID spi1
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 12
#define SPI_CS_PIN 13
#define SPI_CLK_PIN 10

#define SPI_CLK_FREQUENCY 1*MHZ

// Number of samples to generate
#define NUM_GEN_SAMPLES 1024

// Number of samples to send
#define NUM_SAMPLES 100

// Number of values
#define NUM_VALUES 10

// Type definitions:

// Typedefinition: Command from user
typedef enum User_Cmd_e {

    UCMD_STOP_ALL = 0,
    UCMD_START_ALL,
    UCMD_START_ADC,
    UCMD_START_SPI,
    UCMD_STOP_ADC,
    UCMD_STOP_SPI,
    UCMD_RESET,
    UCMD_INV_CMD

}User_Cmd_t;

// Type defintion: State of the system
typedef enum State_e {

    INIT = 0,
    STOP,
    START,
    STOP_PWM,
    START_PWM,
    STOP_SPI,
    START_SPI,
    SPI_WRITE_PWM_OUT,
    SPI_WRITE,
    PWM_OUT,
    SLEEP,
    LOOPING,

}State_t;

// Static variables:

// Static functions declarations:

// Interrupt service routines:

// Command parsing

static User_Cmd_t get_user_cmd(uint8_t *cmd_str);

// SPI configuration

// Function to initialize SPI
static void setup_spi();

// Function to send a burst of data over SPI
static void send_spi_data(uint16_t* data, uint16_t length);

// Sleep mode function:

static void go_to_sleep_mode(void);


// Main function - program entry - core 0
int main() {

    // Main variables:

    // System state:
    State_t state = INIT;
    State_t next_state = SLEEP;
    State_t last_state = INIT;

    // User command
    User_Cmd_t user_cmd = UCMD_STOP_ALL;

    // UART buffer
    uint8_t uart_rx_buffer[MAX_UART_DATA_SIZE];

    // The voltage ramp data (16-bit values)
    uint16_t voltage_ramp[NUM_VALUES][NUM_SAMPLES];

    // Flag for sending loop
    bool exit_send_loop = true;

    // Setup:

    clear_uart_buffer(uart_rx_buffer);

    // Main super loop
    while(true) {

        switch(state) {
        case INIT:
            // Setup UART
            configure_uart_hardware(
                UART_ID, 
                UART_RX_PIN,
                UART_TX_PIN,
                UART_BAUD_RATE,
                8,
                1,
                UART_PARITY_NONE,
                true,
                true,
                '\n'
            );

            // Setup SPI:
            setup_spi();

            // Generate data
            for(uint8_t j = 0; j < NUM_VALUES; j++) {

                for(uint8_t k = 0; k < NUM_SAMPLES; k++) {
                    voltage_ramp[j][k] = j;
                }

            }
           
            last_state = state;
            state = next_state;
            break;
        case START:

            // Configure PWM DAC
            configure_pwm_DAC(
                PWM_PIN, 
                PWM_CLK_FREQUENCY, 
                PWM_FREQUENCY, 
                10, 
                false
            );
            
            // Start PWM
            start_stop_pwm(PWM_PIN, true);

            // Generate data and send data with little wait in between:
            uint8_t j = 0;

            exit_send_loop = false;

            while(!exit_send_loop) {

                send_spi_data(voltage_ramp[j], NUM_SAMPLES);

                if(j == NUM_VALUES) {
                    j = 0;
                }
                else {
                    j++;
                }

                // Check for received command
                if(uart_get_rx_complete_flag(UART_ID)) {
                    clear_uart_buffer(uart_rx_buffer);
                    uart_get_rx_data(UART_ID, uart_rx_buffer);
                    user_cmd = get_user_cmd(uart_rx_buffer);

                    switch(user_cmd) {
                        case UCMD_START_ALL:  
                            break;
                        case UCMD_STOP_ALL:
                            exit_send_loop = true;
                            break;
                        default:
                            break;
                    }
                }
            }
            last_state = state;
            state = next_state;
            next_state = SLEEP;
            break;
        case STOP:

            //Stop PWM
            start_stop_pwm(PWM_PIN, false);

            //Deconfigure PWM
            deconfigure_pwm(PWM_PIN);

            last_state = state;
            state = next_state;
            break;
        case START_PWM:
            // Configure PWM DAC
            configure_pwm_DAC(
                PWM_PIN, 
                PWM_CLK_FREQUENCY, 
                PWM_FREQUENCY, 
                10, 
                false
            );
            
            // Start PWM
            start_stop_pwm(PWM_PIN, true);
            last_state = state;
            state = next_state;
            break;
        case START_SPI:

            // Generate data and send data with little wait in between:
            uint8_t p = 0;

            exit_send_loop = false;

            while(!exit_send_loop) {

                send_spi_data(voltage_ramp[p], NUM_SAMPLES);

                if(p == NUM_VALUES) {
                    p = 0;
                }
                else {
                    p++;
                }

                // Check for received command
                if(uart_get_rx_complete_flag(UART_ID)) {
                    clear_uart_buffer(uart_rx_buffer);
                    uart_get_rx_data(UART_ID, uart_rx_buffer);
                    user_cmd = get_user_cmd(uart_rx_buffer);

                    switch(user_cmd) {
                        case UCMD_STOP_SPI:
                            exit_send_loop = true;
                            break;
                        default:
                            break;
                    }
                }
            }

            last_state = state;
            state = next_state;
            next_state = SLEEP;
            break;
            
        case STOP_PWM:
            //Stop PWM
            start_stop_pwm(PWM_PIN, false);

            //Deconfigure PWM
            deconfigure_pwm(PWM_PIN);
            last_state = state;
            state = next_state;
            break;
        case STOP_SPI:
            last_state = state;
            state = next_state;
            break;
        case LOOPING:
                // Do nothing just wait.
            break;
        case SLEEP:
            go_to_sleep_mode();
            break;
        default:
            break;
        }// end main state machine

        // Check for received command
        if(uart_get_rx_complete_flag(UART_ID)) {
            clear_uart_buffer(uart_rx_buffer);
            uart_get_rx_data(UART_ID, uart_rx_buffer);
            user_cmd = get_user_cmd(uart_rx_buffer);

            //Check command and control system according to that.
            switch(user_cmd) {
                case UCMD_START_ALL:
                    state = START;
                    next_state = STOP;
                    break;
                case UCMD_STOP_ALL:
                    state = STOP;
                    next_state = SLEEP;
                    break;
                case UCMD_START_ADC:
                    state = START_PWM;
                    next_state = LOOPING;
                    break;
                case UCMD_START_SPI:
                    state = START_SPI;
                    next_state = STOP_SPI;
                    break;
                case UCMD_STOP_ADC:
                    state = STOP_PWM;
                    next_state = SLEEP;
                    break;
                case UCMD_STOP_SPI:
                    state = STOP_SPI;
                    next_state = SLEEP;
                    break;
                default:
                    break;
            }
        }

        tight_loop_contents();

    } // end main super loop
} // end main


// Static function implementation:

// Command parsing

static User_Cmd_t get_user_cmd(uint8_t *cmd_str) {

    if(strcmp(cmd_str, "STOP") == 0) {
        return UCMD_STOP_ALL;
    }
    else if(strcmp(cmd_str, "START") == 0) {
        return UCMD_START_ALL;
    }
    else if(strcmp(cmd_str, "RESET") == 0) {
        return UCMD_RESET;
    }
    else if(strcmp(cmd_str, "START_ADC") == 0) {
        return UCMD_START_ADC;
    }
    else if(strcmp(cmd_str, "START_SPI") == 0) {
        return UCMD_START_SPI;
    }
    else if(strcmp(cmd_str, "STOP_ADC") == 0) {
        return UCMD_STOP_ADC;
    }
    else if(strcmp(cmd_str, "STOP_SPI") == 0) {
        return UCMD_STOP_SPI;
    }
    else {
        return UCMD_INV_CMD;
    }

} // end get_user_cmd

// Function to initialize SPI
static void setup_spi() {

    // Init gpio pins
    gpio_init(SPI_MOSI_PIN);
    gpio_init(SPI_MISO_PIN);
    gpio_init(SPI_CS_PIN);
    gpio_init(SPI_CLK_PIN);

    // Set SPI polarity and phase (CPOL = 0, CPHA = 0)
    spi_init(SPI_ID, SPI_CLK_FREQUENCY);
    spi_set_slave(SPI_ID, true);
    spi_set_format(SPI_ID, 16, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

    // Initialize SPI
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_dir(SPI_CS_PIN, false);
    
}

// Function to send a burst of data over SPI
static void send_spi_data(uint16_t *data, uint16_t length) {
    uint16_t rx_dummy = 0;
    for(uint16_t i = 0; i < length; i++) {
        // Check if CS is LOW before trying to send
        if (!gpio_get(SPI_CS_PIN)) {
            spi_write16_read16_blocking(SPI_ID, &data[i], &rx_dummy, 1);
        } else {
            // Exit early or break if CS is high
            break;
        }
    }
}

// Sleep mode function:

static void go_to_sleep_mode(void) {

    // Go to sleep
    __wfi();

    // If wake up wait for some ms
    busy_wait_ms(100);

    return;

} // end go_to_sleep_mode

//end file: main.c
