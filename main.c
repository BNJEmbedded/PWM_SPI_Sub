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
    //TODO: 
*/

// Includes:

// Standard library:
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

// Pico libraries:
#include "pico/stdlib.h"
#include "pico/multicore.h"

// Pico hardware libraries:
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"

// Own library:
#include "uart.h"
#include "pwm.h"

// Preprocessor:

// UART:

#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 250*KHZ //250 kBd

/* 
    Preprocessor that controls if ctrl messages get transmitted.
    This is only on sub to disable transmission of control messages,
    because sub UART TX may not be connected to PC, therefore the
    UART TX has no use and only introduces additional noise.
    If not connected replace true with false, if connected set to true.
*/
#define EN_UART_TX false

// PWM:
#define PWM_PIN 8
#define PWM_CLK_FREQUENCY 125*MHZ
#define PWM_FREQUENCY 100*KHZ

#define PWM_LVL_DMA_CH 0

#define PWM_HOLD_TIME_MS 10 // Hold every value for 10 ms
#define PWM_LVL_TABLE_SIZE 1025 // 10 Bit resolution 

// SPI:

#define SPI_ID spi1
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 12
#define SPI_CS_PIN 13
#define SPI_CLK_PIN 10

#define SPI_CLK_FREQUENCY 10*MHZ

#define SPI_TX_DATA_DMA_CH 1
#define SPI_TX_CTRL_DMA_CH 2

#define SPI_HOLD_TIME_MS 10 // Hold every value for 10 ms

// Number of samples to generate
#define NUM_GEN_SAMPLES 1024

// Type definitions:

// Typedefinition: States state machine on core0
typedef enum CORE0_State_e {
    C0_INIT = 0,
    C0_STOP,
    C0_START,
    C0_GENERATE,
    C0_SLEEP,
}CORE0_State_t;

// Typedefinition: States state machine on core1
typedef enum CORE1_State_e {
    C1_INIT = 0,
    C1_STOP,
    C1_START,
    C1_GENERATE,
    C1_SLEEP,
}CORE1_State_t;

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

// Typedefinition: Command from CORE0 to CORE1
typedef enum Core_Cmd_e {
    CCMD_STOP = 0,
    CCMD_START,
    CCMD_INV_CMD
}Core_Cmd_t;

// Typedefinition: Control message type
typedef enum Ctrl_Msg_Type_e {

    CMD_RCVD = 0,
    FIN_INIT,
    GO_SLEEP,
    WAKEUP,
    STOP_ADC,
    START_ADC,
    STOP_SPI,
    START_SPI,
    RESET,
    CHANGE_STATE,
    DEBUG,
    ERROR

}Ctrl_Msg_Type_t;

// Typedefinition: Core ID for control messages
typedef enum Core_ID_e {
    CORE0 = 0,
    CORE1
}Core_ID_t;

// Typedefinition: Core control message
typedef struct Ctrl_Msg_s {

    Ctrl_Msg_Type_t type;
    Core_ID_t src_id;
    uint8_t data[MAX_UART_DATA_SIZE];

}Ctrl_Msg_t;


// Static variables:

// Core0 variables (Do not use them in CORE1 ISR or static functions!!!)

// PWM DAC pwm lvl table:
static uint16_t pwm_lvl_table[PWM_LVL_TABLE_SIZE];

// PWM hold time timer:
static struct repeating_timer pwm_ht_timer;

// Index of pwm lvl table
static uint16_t pwm_lvl_table_index = 0;

// Core1 variables (Do not use them in CORE0 ISR or static functions!!!)

// Flag to set that core1 received message interrupt from core0
static volatile bool core1_received_cmd = false;

// CMD from core0
Core_Cmd_t cmd_from_core0 = CCMD_STOP;

// PWM hold time timer:
static struct repeating_timer spi_ht_timer;

// Index of spi dc table
static uint16_t spi_dc_table_index = 0;

// Data table to send for hold time
static uint16_t spi_dc_int_table[PWM_LVL_TABLE_SIZE];

// Actual src that is transmitted by DMA to SPI continuously
static uint16_t actual_dc_val = 0;

// Core0 and core1 variables (The semaphore to synchronize)
semaphore_t uart_sem;

// Static functions declarations:

// Interrupt service routine core 0: 
// Corresponding ISRs need to be enabled on core 0.

static bool pwm_hold_time_timer_cb(struct repeating_timer *t);

// Interrupt service routine core 1: 
// Corresponding ISRs need to be enabled on core 1.

static bool spi_hold_time_timer_cb(struct repeating_timer *t);

// Multicore communication interrupt in core1
static void __not_in_flash_func (core1_fifo_isr)(void);

// State to string functions:
static void get_str_from_c0_state(CORE0_State_t state, uint8_t *state_str);
static void get_str_from_c1_state(CORE1_State_t state, uint8_t *state_str);
static void get_c0_state_change_string(CORE0_State_t state,
CORE0_State_t new_state, uint8_t *str);
static void get_c1_state_change_string(CORE1_State_t state, 
CORE1_State_t new_state, uint8_t *str);

// Command parsing

static User_Cmd_t get_user_cmd(uint8_t *cmd_str);
static Core_Cmd_t get_core_cmd(uint32_t cmd);

// PWM Setup:

static uint16_t setup_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    float pwm_clk_freq, float pwm_freq, uint16_t *pwm_lvl_table);
static int8_t start_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    uint16_t *pwm_lvl_table, uint32_t hold_time, bool hold_time_ms, 
    struct repeating_timer *timer, repeating_timer_callback_t hold_time_timer_cb);
static int8_t stop_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    struct repeating_timer *timer);

// SPI configuration

static int8_t setup_spi(spi_inst_t *spi_inst, uint8_t mosi_pin, uint8_t miso_pin,
    uint8_t cs_pin, uint8_t clk_pin, uint clk_frequency);
static int8_t setup_spi_tx_dma(uint dma_data_ch, uint dma_ctrl_ch,
    spi_inst_t *spi_inst, uint16_t *src);
static int8_t start_spi_tx(uint dma_data_ch,uint32_t hold_time, 
    bool hold_time_ms, struct repeating_timer *timer, 
    repeating_timer_callback_t hold_time_timer_cb);
static int8_t stop_spi_tx(uint dma_data_ch, uint dma_ctrl_ch, 
    struct repeating_timer *timer);

// Core control message functions:
static Ctrl_Msg_t get_ctrl_msg(Ctrl_Msg_Type_t type, Core_ID_t id, 
    uint8_t *data);
static void get_ctrl_msg_str(Ctrl_Msg_t ctrl_msg, uint8_t *ctrl_msg_str);
static void send_ctrl_msg(Ctrl_Msg_t ctrl_msg, uart_inst_t *uart_hw, 
semaphore_t *uart_sem);

// Sleep mode function:

static void go_to_sleep_mode(void);

// Core 1 function

void main_core1(void) {

    //Core 1 variables:

    // Core 1 state:
    CORE1_State_t state = C1_INIT;
    CORE1_State_t last_state = C1_INIT;
    CORE1_State_t next_state = C1_SLEEP;

    // Core 1 control message:
    Ctrl_Msg_t core1_ctrl_msg;

    // Buffer to transmit acquired data
    uint8_t uart_tx_buffer[MAX_UART_DATA_SIZE];
    clear_uart_buffer(uart_tx_buffer);

    // Core 1 super loop
    while(true) {

        // Core 1 state machine
        switch(state) {
            case C1_INIT:

                // Enable core 1 FIFO interrupt - to communicate with core0
                multicore_fifo_clear_irq();
                irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_fifo_isr);
                irq_set_enabled(SIO_IRQ_PROC1, true);

                // Reset flag
                core1_received_cmd = false;

                // Init SPI

                setup_spi(
                    SPI_ID, 
                    SPI_MOSI_PIN, 
                    SPI_MISO_PIN, 
                    SPI_CS_PIN, 
                    SPI_CLK_PIN,
                    SPI_CLK_FREQUENCY
                );

                // Set index to beginning
                spi_dc_table_index = 0;

                // Set first actual dc value
                actual_dc_val = spi_dc_int_table[spi_dc_table_index];

                // Init SPI TX and RX DMA channels.

                setup_spi_tx_dma(
                    SPI_TX_DATA_DMA_CH,
                    SPI_TX_CTRL_DMA_CH,
                    SPI_ID,
                    &actual_dc_val
                );

                // Generate data - Linear duty cycle table from 0 to 1024
                for(uint16_t i = 0; i < PWM_LVL_TABLE_SIZE; i++) {
                    spi_dc_int_table[i] = i;
                }

                core1_ctrl_msg = get_ctrl_msg(FIN_INIT, CORE1, NULL);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                last_state = state;

                get_c1_state_change_string(state, next_state, uart_tx_buffer);
                core1_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE1, uart_tx_buffer);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                state = next_state;

                break;
            case C1_STOP:
                core1_ctrl_msg = get_ctrl_msg(STOP_SPI, CORE1, NULL);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                // Stop dma channel and timer for SPI TX
                stop_spi_tx(SPI_TX_DATA_DMA_CH, SPI_TX_CTRL_DMA_CH,
                    &spi_ht_timer);

                // If stop go to sleep
                last_state = state;

                get_c1_state_change_string(state, next_state, uart_tx_buffer);
                core1_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE1, uart_tx_buffer);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                state = next_state;
                break;
            case C1_START:
                core1_ctrl_msg = get_ctrl_msg(START_SPI, CORE1, NULL);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                // Init SPI

                setup_spi(
                    SPI_ID, 
                    SPI_MOSI_PIN, 
                    SPI_MISO_PIN, 
                    SPI_CS_PIN, 
                    SPI_CLK_PIN,
                    SPI_CLK_FREQUENCY
                );

                // Set index to beginning
                spi_dc_table_index = 0;

                // Set first actual dc value
                actual_dc_val = spi_dc_int_table[spi_dc_table_index];

                // Init SPI TX and RX DMA channels.

                setup_spi_tx_dma(
                    SPI_TX_DATA_DMA_CH,
                    SPI_TX_CTRL_DMA_CH,
                    SPI_ID,
                    &actual_dc_val
                );

                // Start DMA channel and timer 

                start_spi_tx(SPI_TX_DATA_DMA_CH, SPI_HOLD_TIME_MS, true,
                &spi_ht_timer, spi_hold_time_timer_cb);

                last_state = state;

                get_c1_state_change_string(state, next_state, uart_tx_buffer);
                core1_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE1, uart_tx_buffer);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);

                state = next_state;
                break;
            case C1_SLEEP:
                core1_ctrl_msg = get_ctrl_msg(GO_SLEEP, CORE1, NULL);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);
                // Set core to sleep
                __wfi();
                core1_ctrl_msg = get_ctrl_msg(WAKEUP, CORE1, NULL);
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);
                break;
            default:    
                break;
        } // end core 1 state machine

        // Check for received core0 cmd
        if(core1_received_cmd) {
            // Command was read in ISR

            // Reset flag core1_received_cmd
            core1_received_cmd = false;

            // Parse command and set next state
            if(cmd_from_core0 == CCMD_START) {
                core1_ctrl_msg = get_ctrl_msg(CMD_RCVD, CORE1, "START");
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);
                last_state = state;
                state = C1_START;
                next_state = C1_SLEEP;
            }
            else if(cmd_from_core0 == CCMD_STOP) {
                core1_ctrl_msg = get_ctrl_msg(CMD_RCVD, CORE1, "STOP");
                send_ctrl_msg(core1_ctrl_msg, UART_ID, &uart_sem);
                last_state = state;
                state = C1_STOP;
                next_state = C1_SLEEP;
            }
            else {
                last_state = state;
                state = C1_SLEEP;
            }
        }
    } // end core 1 super loop
    
}// end main_core1


// Main function - program entry - core 0

int main() {

    //Core 0 variables:

    //Core 0 state:

    CORE0_State_t state = C0_INIT;
    CORE0_State_t last_state = C0_INIT;
    CORE0_State_t next_state = C0_SLEEP;

    // Core 1 state: Is used for having the state for commanding the other core
    CORE1_State_t c1_state = C1_INIT;

    // UART RX and TX buffers
    uint8_t uart_tx_buffer[MAX_UART_DATA_SIZE];
    uint8_t uart_rx_buffer[MAX_UART_DATA_SIZE];
    clear_uart_buffer(uart_tx_buffer);
    clear_uart_buffer(uart_rx_buffer);

    // User command
    User_Cmd_t user_cmd = UCMD_STOP_ALL;

    // Command to core1
    Core_Cmd_t cmd_to_core1 = UCMD_STOP_ALL;

    // Control message to user
    Ctrl_Msg_t core0_ctrl_msg;

    // Main super loop
    while(true) {

        // Core 0 state machine
        switch(state) {
            case C0_INIT:

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

                // Init semaphore
                sem_init(&uart_sem, 1, 1);

                core0_ctrl_msg = get_ctrl_msg(FIN_INIT, CORE0, NULL);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                // Start program on core1
                multicore_launch_core1(&main_core1);

                // Core 1 goes to sleep - keep track of the state
                c1_state = C1_SLEEP;

                // Init PWM
                uint16_t pwm_wrap = 
                setup_pwm_dac(PWM_PIN, PWM_LVL_DMA_CH, PWM_CLK_FREQUENCY, 
                    PWM_FREQUENCY, pwm_lvl_table);

                /*
                    Generate pwm lvl table.
                    Essentially creates linear increasing 
                    duty cycles from 0 to 100%, with step size of choosen
                    resolution.
                */
                for(uint16_t k = 0; k < PWM_LVL_TABLE_SIZE; k++) {
                    pwm_lvl_table[k] = k*(pwm_wrap/PWM_LVL_TABLE_SIZE);
                }

                // Send control message when state change ocurs
                get_c0_state_change_string(state, next_state, uart_tx_buffer);
                core0_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE0, uart_tx_buffer);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                last_state = state;
                state = next_state;
                break;
            case C0_STOP:
                core0_ctrl_msg = get_ctrl_msg(STOP_ADC, 
                    CORE0, 
                    uart_rx_buffer
                );
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                // Stop PWM
                stop_pwm_dac(PWM_PIN, PWM_LVL_DMA_CH, &pwm_ht_timer);

                last_state = state;

                get_c0_state_change_string(state, next_state, uart_tx_buffer);
                core0_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE0, uart_tx_buffer);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                state = next_state;
                break;
            case C0_START:
                core0_ctrl_msg = get_ctrl_msg(START_ADC, CORE0,uart_rx_buffer);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                // Start PWM
                start_pwm_dac(PWM_PIN, PWM_LVL_DMA_CH, pwm_lvl_table, 
                    PWM_HOLD_TIME_MS, true, &pwm_ht_timer, pwm_hold_time_timer_cb);
                
                last_state = state;
                
                get_c0_state_change_string(state, next_state, uart_tx_buffer);
                core0_ctrl_msg = 
                get_ctrl_msg(CHANGE_STATE, CORE0, uart_tx_buffer);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);

                state = next_state;
                next_state = C0_STOP;
                break;
            case C0_SLEEP:
                core0_ctrl_msg = get_ctrl_msg(GO_SLEEP, CORE0, NULL);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);
                go_to_sleep_mode();
                core0_ctrl_msg = get_ctrl_msg(WAKEUP, CORE0, NULL);
                send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);
                break;             
            default:
                break;
        } // end core 0 state machine

        // Check for received command
        if(uart_get_rx_complete_flag(UART_ID)) {

            clear_uart_buffer(uart_rx_buffer);
            uart_get_rx_data(UART_ID, uart_rx_buffer);
            core0_ctrl_msg = get_ctrl_msg(CMD_RCVD, CORE0, uart_rx_buffer);
            send_ctrl_msg(core0_ctrl_msg, UART_ID, &uart_sem);
            user_cmd = get_user_cmd(uart_rx_buffer);

            //Check command and control system according to that.
            switch(user_cmd) {
                case UCMD_START_ALL:
                    state = C0_START;
                    next_state = C0_SLEEP;
                    //If core1 is not already processing command to read/proc
                    if(c1_state != C1_GENERATE) {
                        multicore_fifo_push_blocking(1);
                        c1_state = C1_GENERATE;
                    }
                    break;
                case UCMD_STOP_ALL:
                    state = C0_STOP;
                    next_state = C0_SLEEP;
                    //If core1 is not sleeping command it to do so
                    if(c1_state != C1_SLEEP) {
                        multicore_fifo_push_blocking(0);
                        c1_state = C1_SLEEP;
                    }
                    break;
                case UCMD_START_ADC:
                    state = C0_START;
                    next_state = C0_SLEEP;
                    break;
                case UCMD_START_SPI:
                    // Send command to core1 to start SPI
                    //If core1 is not already reading processing command it
                    if(c1_state != C1_GENERATE) {
                        multicore_fifo_push_blocking(1);
                        c1_state = C1_GENERATE;
                    }
                    state = C0_SLEEP;
                    next_state = C0_SLEEP;
                    break;
                case UCMD_STOP_ADC:
                    state = C0_STOP;
                    next_state = C0_SLEEP;
                    break;
                case UCMD_STOP_SPI:
                    //If core1 is not sleeping command it to do so
                    if(c1_state != C1_SLEEP) {
                        multicore_fifo_push_blocking(0);
                        c1_state = C1_SLEEP;
                    }
                    state = C0_SLEEP;
                    next_state = C0_SLEEP;
                    break;
                default: break;
            }
        }

       tight_loop_contents();
 
    } // end main super loop
} // end main

// Static function implementation:

// Static function implementation:

// Interrupt service routine core 0: 

static bool pwm_hold_time_timer_cb(struct repeating_timer *t) {

    // If index reached end of table restart again.
    if(pwm_lvl_table_index == PWM_LVL_TABLE_SIZE) {
        pwm_lvl_table_index = 0;
    }

    // Transfer new lvl
    dma_channel_set_read_addr(PWM_LVL_DMA_CH, 
        &pwm_lvl_table[pwm_lvl_table_index], true);
    
    // Set pwm_table_index to next entry
    pwm_lvl_table_index++;

    return true;

} // end pwm_hold_time_timer_cb

// Interrupt service routine core 1: 

// Multicore communication interrupt in core1

static void __not_in_flash_func (core1_fifo_isr)(void) {

    // Flag that core1 program is notified that action needs to be taken
    core1_received_cmd = true;

    // Clear interrupt
    multicore_fifo_clear_irq();

    // If valid data is in the FIFO
    if(multicore_fifo_rvalid()) {

        // Read all data that is in the FIFO (Should be one uint32_t)
        while(multicore_fifo_rvalid()) {
            uint32_t core0_msg = multicore_fifo_pop_blocking();
            // Parse command for core 1
            cmd_from_core0 = get_core_cmd(core0_msg);
            break;
        }
        // Drain FIFO for next command.
        multicore_fifo_drain();
    }

} // end core1_fifo_isr

static bool spi_hold_time_timer_cb(struct repeating_timer *t) {

    // If index reached end of table restart again.
    if(spi_dc_table_index == PWM_LVL_TABLE_SIZE) {
        spi_dc_table_index = 0;
    }

    // Set new dc
    actual_dc_val = spi_dc_int_table[spi_dc_table_index];
    
    // Set spi_dc_table_index to next entry
    spi_dc_table_index++;

    return true;
} // end spi_hold_time_timer_cb

// State to string functions:

static void get_str_from_c0_state(CORE0_State_t state, uint8_t *state_str) {

    switch(state) {
    case C0_INIT:
        strcpy(state_str, "INIT");
        return;
    case C0_START:
        strcpy(state_str, "START");
        return;
    case C0_STOP:
        strcpy(state_str, "STOP");
        return;
    case C0_GENERATE:
        strcpy(state_str, "PROCESS");
        return;
    case C0_SLEEP:
        strcpy(state_str, "SLEEP");
        return;
    default:
        strcpy(state_str, "UNKOWN_STATE");
        return;
        break;
    }

} // end get_str_from_c0_state

static void get_str_from_c1_state(CORE1_State_t state, uint8_t *state_str) {

    switch(state) {
        case C1_INIT:
            strcpy(state_str, "INIT");
            return;
        case C1_START:
            strcpy(state_str, "START");
            return;
        case C1_STOP:
            strcpy(state_str, "STOP");
            return;
        case C1_GENERATE:
            strcpy(state_str, "GENERATE");
            return;
        case C1_SLEEP:
            strcpy(state_str, "SLEEP");
            return;
        default:
            strcpy(state_str, "UNKOWN_STATE");
            return;
            break;
        }

} // end get_str_from_c1_state

static void get_c0_state_change_string(CORE0_State_t state, 
CORE0_State_t new_state, uint8_t *str) {

    uint8_t new_state_str[50];
    uint8_t old_state_str[50];
    get_str_from_c0_state(state, old_state_str);
    get_str_from_c0_state(new_state, new_state_str);
    sprintf(str,"CHANGING FROM STATE: %s TO STATE: %s",
            old_state_str, new_state_str);

} // end get_c0_state_change_string

static void get_c1_state_change_string(CORE1_State_t state, 
    CORE1_State_t new_state, uint8_t *str) {
        
        uint8_t new_state_str[50];
        uint8_t old_state_str[50];
        get_str_from_c1_state(state, old_state_str);
        get_str_from_c1_state(new_state, new_state_str);
        sprintf(str,"CHANGING FROM STATE: %s TO STATE: %s",
                old_state_str, new_state_str);
    
} // end get_c1_state_change_string

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

static Core_Cmd_t get_core_cmd(uint32_t cmd) {

    if(cmd == 0) {
        return CCMD_STOP;
    }
    else if(cmd == 1) {
        return CCMD_START;
    }

    // Else return invalid command
    return CCMD_INV_CMD;

} // end get_core_cmd

// PWM Setup:

static uint16_t setup_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    float pwm_clk_freq, float pwm_freq, uint16_t *pwm_lvl_table) {

    // Get slice and channel from gpio pin
    uint8_t pwm_slice = pwm_gpio_to_slice_num(pwm_dac_gpio);
    uint8_t pwm_channel = pwm_gpio_to_channel(pwm_dac_gpio);

    // Init gpio pin
    gpio_init(pwm_dac_gpio);
    gpio_set_function(pwm_dac_gpio, GPIO_FUNC_PWM);
    gpio_set_dir(pwm_dac_gpio, true);

    //Get prescaler value
    float pwm_clk_div = (float)(clock_get_hz(clk_sys)/pwm_clk_freq);
    //Set pwm prescaler
    pwm_set_clkdiv(pwm_slice, pwm_clk_div);
    
    /*
        Set pwm wrap. 
        If pwm counter reaches wrap -> toggle. Therefore sets high time
    */
    uint16_t pwm_wrap = (uint16_t)(pwm_clk_freq/pwm_freq);
    pwm_set_wrap(pwm_slice, pwm_wrap);

    /*
        Set pwm lvl. Start with lvl 0.
        If pwm counter reaches lvl-> toggle. Therefore sets low time
    */

    uint16_t pwm_lvl = (uint16_t)(pwm_wrap*0);
    pwm_set_chan_level(pwm_slice, pwm_channel, pwm_lvl);

    // Configure DMA channel to transfer pwm lvl
    dma_channel_config cfg = dma_channel_get_default_config(dma_ch);

    // PWM lvl is 16 bit unsigned integer value therefore set transfer size to 16
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16); 
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, false);

    // Configure DMA transfer channel
    dma_channel_configure(
        dma_ch, 
        &cfg,
        &pwm_hw->slice[pwm_slice].cc,  // Destination is counter compare reg
        pwm_lvl_table,  // Source is first entry of the pwm_lvl_table
        1,  
        false           
    );

    return pwm_wrap;

} // end setup_pwm_dac

static int8_t start_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    uint16_t *pwm_lvl_table, uint32_t hold_time, bool hold_time_ms, 
    struct repeating_timer *timer, repeating_timer_callback_t hold_time_timer_cb) {

    // Start pwm
    pwm_set_enabled(pwm_gpio_to_slice_num(pwm_dac_gpio), true);

    // Start first transfer - first entry of pwm lvl table
    dma_channel_set_read_addr(dma_ch, &pwm_lvl_table[pwm_lvl_table_index], true);

    // Set pwm_lvl_table_index to next entry
    pwm_lvl_table_index++;

    // Add repeating timer
    if(hold_time_ms) {
        // Add ms timer if hold time should be in ms
        add_repeating_timer_ms(hold_time, hold_time_timer_cb, NULL, timer);
    }
    else {
        // Else add us timer
        add_repeating_timer_us(hold_time, hold_time_timer_cb, NULL, timer);
    }

    return 0;

} // end start_pwm_dac

static int8_t stop_pwm_dac(uint8_t pwm_dac_gpio, uint dma_ch, 
    struct repeating_timer *timer) {

        // Stop pwm
        pwm_set_enabled(pwm_gpio_to_slice_num(pwm_dac_gpio), false);

        // Stop repeating timer
        cancel_repeating_timer(timer);

        // Set index to 0
        pwm_lvl_table_index = 0;

        return 0;

} // end stop_pwm_dac

// SPI configuration:

static int8_t setup_spi(spi_inst_t *spi_inst, uint8_t mosi_pin, uint8_t miso_pin,
    uint8_t cs_pin, uint8_t clk_pin, uint clk_frequency) {
    
    // Init gpio pins
    gpio_init(mosi_pin);
    gpio_init(miso_pin);
    gpio_init(cs_pin);
    gpio_init(clk_pin);
    
    // Set SPI polarity and phase (CPOL = 0, CPHA = 0)
    spi_init(SPI_ID, SPI_CLK_FREQUENCY);
    spi_set_slave(SPI_ID, true);
    spi_set_format(SPI_ID, 16, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

    // Initialize SPI
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_dir(SPI_CS_PIN, false);
    
    return 0;
    
} // end setup_spi

static int8_t setup_spi_tx_dma(uint dma_data_ch, uint dma_ctrl_ch,
    spi_inst_t *spi_inst, uint16_t *src) {

   /* 
       Transmit channel is configured in chained DMA configuration to send
       continuously the same byte (or value).
   */

   uint dreq = 0;
   spi_hw_t *spi_hw;

   // Pace transfers based on availability of ADC samples
   if(spi_inst == spi0) {
       dreq = DREQ_SPI0_TX;
       spi_hw = spi0_hw;
   }
   else {
       dreq = DREQ_SPI1_TX;
       spi_hw = spi1_hw;
   }

   // 1. Configure data (TX) channel
   dma_channel_config cfg_dc = dma_channel_get_default_config(dma_data_ch);
   channel_config_set_transfer_data_size(&cfg_dc, DMA_SIZE_16); 
   channel_config_set_read_increment(&cfg_dc, false);         
   channel_config_set_write_increment(&cfg_dc, false);        
   channel_config_set_dreq(&cfg_dc, dreq);
   channel_config_set_chain_to(&cfg_dc, dma_ctrl_ch); 

   // Data channel does not start immediately
   dma_channel_configure(
       dma_data_ch,
       &cfg_dc,
       &spi_hw->dr,   
       src,          
       1,             
       false          
   );

   // Configure control channel to reconfigure and restart the data channel
   dma_channel_config cfg_cc = dma_channel_get_default_config(dma_ctrl_ch);
   channel_config_set_transfer_data_size(&cfg_cc, DMA_SIZE_32); 
   channel_config_set_read_increment(&cfg_cc, true);           
   channel_config_set_write_increment(&cfg_cc, true);          
   channel_config_set_chain_to(&cfg_cc, dma_data_ch);

   // Create a control block in RAM that sets up the data channel registers again
   static dma_channel_config saved_cfg_dc;
   saved_cfg_dc = cfg_dc;

   /*
       Create a control block to copy 4 words 
       (WRITE_ADDR, READ_ADDR, TRANS_COUNT, CTRL_TRIG)
   */
   static uint32_t control_block[4];
   control_block[0] = (uint32_t)&spi_hw->dr;        
   control_block[1] = (uint32_t)src;                
   control_block[2] = 1;                           
   control_block[3] = *((uint32_t*)&cfg_dc) | DMA_CH0_CTRL_TRIG_EN_BITS; 

   dma_channel_configure(
       dma_ctrl_ch,
       &cfg_cc,
       (void *)(&dma_hw->ch[dma_data_ch].write_addr),
       control_block,                                 
       4,                                             
       false                                          
    );

} // end setup_spi_tx_dma

static int8_t start_spi_tx(uint dma_data_ch, uint32_t hold_time, 
    bool hold_time_ms, struct repeating_timer *timer, 
    repeating_timer_callback_t hold_time_timer_cb) {

    // Enable SPI DMA and send continuously data
    dma_start_channel_mask(1u << dma_data_ch);

    // Increment index
    spi_dc_table_index++;

    // Add repeating timer
    if(hold_time_ms) {
        // Add ms timer if hold time should be in ms
        add_repeating_timer_ms(hold_time, hold_time_timer_cb, NULL, timer);
    }
    else {
        // Else add us timer
        add_repeating_timer_us(hold_time, hold_time_timer_cb, NULL, timer);
    }

    return 0;
    
} // end start_spi_tx

static int8_t stop_spi_tx(uint dma_data_ch, uint dma_ctrl_ch, 
    struct repeating_timer *timer) {

    // Stop timer
    cancel_repeating_timer(timer);

    // Disable SPI TX DMAs 
    dma_channel_abort(dma_data_ch);
    dma_channel_abort(dma_ctrl_ch);

    // Clean channels
    dma_channel_cleanup(dma_data_ch);
    dma_channel_cleanup(dma_ctrl_ch);

    // Reset index
    spi_dc_table_index = 0;

    // Reset actual value
    actual_dc_val = spi_dc_int_table[spi_dc_table_index];

    return 0;

} // end stop_spi_tx

// Core control message functions:

static Ctrl_Msg_t get_ctrl_msg(Ctrl_Msg_Type_t type, Core_ID_t id, 
    uint8_t *data) {
    
        Ctrl_Msg_t new_ctrl_msg;
        new_ctrl_msg.src_id = id;
        new_ctrl_msg.type = type;
        strcpy(new_ctrl_msg.data,data);
        return new_ctrl_msg;
    
    } // end get_ctrl_msg

static void get_ctrl_msg_str(Ctrl_Msg_t ctrl_msg, uint8_t *ctrl_msg_str) {

    uint8_t core_str[6];
    if(ctrl_msg.src_id == CORE0) {
        strcpy(core_str, "CORE0");
    }
    else {
        strcpy(core_str, "CORE1");
    }

    switch(ctrl_msg.type) {
    case FIN_INIT:
        sprintf(ctrl_msg_str, "%s INFO: INITIALIZATION FINISHED", core_str);
        return;
    case GO_SLEEP:
        sprintf(ctrl_msg_str, "%s INFO: CORE GOES SLEEPING", core_str);
        return;
    case WAKEUP:
        sprintf(ctrl_msg_str, "%s INFO: CORE WAKEUP FROM INTERRUPT", core_str);
        return;
    case START_ADC:
        sprintf(ctrl_msg_str, "%s INFO: START_PWM", core_str);
        return;
    case STOP_ADC:
        sprintf(ctrl_msg_str, "%s INFO: STOP_PWM", core_str);
        return;
    case START_SPI:
        sprintf(ctrl_msg_str, "%s INFO: START_SPI", core_str);
        return;
    case STOP_SPI:
        sprintf(ctrl_msg_str, "%s INFO: STOP_SPI", core_str);
        return;
    case RESET:
        sprintf(ctrl_msg_str, "%s INFO: RESET", core_str);
        return;
    case CMD_RCVD:
        sprintf(
            ctrl_msg_str,
            
            "%s INFO: COMMAND: ""%s"" RECEIVED ", 
            core_str, ctrl_msg.data
        );
        return;
    case CHANGE_STATE:
        sprintf(ctrl_msg_str,"%s INFO: %s",core_str, ctrl_msg.data);
        return;
    case ERROR:
        sprintf(ctrl_msg_str,"%s ERROR: %s",core_str,ctrl_msg.data);
        return;
    case DEBUG:
        sprintf(ctrl_msg_str,"%s DEBUG: %s",core_str,ctrl_msg.data);
        return;
    default:
        strcpy(ctrl_msg_str, "EMPTY");
        break;
    }

}// end get_ctrl_msg_str

static void send_ctrl_msg(Ctrl_Msg_t ctrl_msg, uart_inst_t *uart_hw,
semaphore_t *uart_sem) {

    uint8_t ctrl_msg_str[MAX_UART_DATA_SIZE];
    get_ctrl_msg_str(ctrl_msg, ctrl_msg_str);

    // Preprocessor that controls if ctrl messages get transmitted.
    #if EN_UART_TX
        // Block till semaphore is free
        sem_acquire_blocking(uart_sem);

        // Send control message
        uart_tx_data(uart_hw, ctrl_msg_str);

        // Release semaphore again signaling the resource is not in use anymore.
        sem_release(uart_sem);
    #endif

}// end send_ctrl_msg

// Sleep mode function:

static void go_to_sleep_mode(void) {

    // Go to sleep
    __wfi();

    // If wake up wait for some ms
    busy_wait_ms(100);

    return;

} // end go_to_sleep_mode

//end file: main.c
