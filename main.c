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

#define SPI_ID spi0
#define SPI_TX_PIN
#define SPI_RX_PIN 
#define SPI_CS_PIN
#define SPI_CLK_PIN

#define SPI_DATA_BUFFER_SIZE 100
#define SPI_DMA_CH


// Type definitions:

// Typedefinition: Command from user
typedef enum User_Cmd_e {

    UCMD_STOP = 0,
    UCMD_START,
    UCMD_RESET,
    UCMD_INV_CMD

}User_Cmd_t;

// Type defintion: State of the system
typedef enum State_e {

    INIT = 0,
    STOP,
    START,
    LOOPING,

}State_t;


// Static variables:

// Static functions declarations:

// Interrupt service routines:


// Command parsing

static User_Cmd_t get_user_cmd(uint8_t *cmd_str);

static void assemble_frame(uint8_t *data);


// ADC configuration
static int8_t setup_adc_dma(uint dma_ch, uint8_t *dest, void *dma_isr);
static int8_t start_adc_dma(uint dma_ch);

// SPI configuration


// Main function - program entry - core 0
int main() {

    // Main variables:

    // System state:
    State_t state = INIT;
    State_t last_state = INIT;

    // User command
    User_Cmd_t user_cmd = UCMD_STOP;

    // UART buffer
    uint8_t uart_rx_buffer[MAX_UART_DATA_SIZE];

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
            last_state = state;
            state = LOOPING;
            break;
        case START:

            // Configure PWM DAC
            configure_pwm_DAC(
                PWM_PIN, 
                PWM_CLK_FREQUENCY, 
                PWM_FREQUENCY, 
                10, 
                false);
            
            // Start PWM
            start_stop_pwm(PWM_PIN, true);
            break;
        case STOP:

            //Stop PWM
            start_stop_pwm(PWM_PIN, false);

            //Deconfigure PWM
            deconfigure_pwm(PWM_PIN);

        case LOOPING:
                // Do nothing just wait.
            break;
        default:
            break;
        }// end main state machine

        // Check for received command
        if(uart_get_rx_complete_flag(UART_ID)) {
            clear_uart_buffer(uart_rx_buffer);
            uart_get_rx_data(UART_ID, uart_rx_buffer);
            user_cmd = get_user_cmd(uart_rx_buffer);

            switch(user_cmd) {
                case UCMD_START:
                    last_state = state;
                    state = START;
                    break;
                case UCMD_STOP:
                    last_state = state;
                    state = START;
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
        return UCMD_STOP;
    }
    else if(strcmp(cmd_str, "START") == 0) {
        return UCMD_START;
    }
    else if(strcmp(cmd_str, "RESET") == 0) {
        return UCMD_RESET;
    }
    else {
        return UCMD_INV_CMD;
    }

} // end get_user_cmd

//end file: main.c
