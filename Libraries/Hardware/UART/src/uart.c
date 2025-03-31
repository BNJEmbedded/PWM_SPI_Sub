//File: uart.c
//Project: Pico_MRI_Test_M

/* Description:

    Custom uart-wrapper around the Raspberry-Pi-Pico-SDK (hardware/uart.h). 
    Let user configure UART, with various baud-rates, stop-bits, parity-bits with RX-Interrupt.
    After configuration user could send and receive data via UART.
    NOTE: This module is not multi core save.

    FUTURE_FEATURE: Allow user to set uart only in rx or tx mode, or maybe even on single wire mode
    FUTURE_FEATURE: Make this module multi-core save.

*/


//Corresponding header-file:
#include "uart.h"

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h"

//Pico Hardware-Libraries:
#include "hardware/uart.h"
#include "hardware/claim.h"
#include "hardware/irq.h"

//Own Libraries:

//Preprocessor constants:
#define UART0_ID uart0
#define UART1_ID uart1

//Typedefs:

typedef struct Uart_Config_s {

    uart_inst_t *uart_instance;
    uint8_t uart_tx_pin;
    uint8_t uart_rx_pin;
    bool is_uart_configured;
    volatile bool uart_rx_complete_flag;
    uint8_t uart_rx_data[MAX_UART_DATA_SIZE];
    uint8_t uart_terminator;

}Uart_Config_t;

//File global (static) function definition and implementation

//File global (static) variables:

static Uart_Config_t uart_config_array[2];
static volatile size_t uart_rx_length[2] = {0,0};

//Functions:

//File global (static) function definitions

static void uart0_rx_interrupt_handler(void) { 

    while (uart_is_readable(uart0)) {
        uint8_t data_rx = uart_getc(uart0); //Read the received data
        if (data_rx == uart_config_array[0].uart_terminator || uart_rx_length[0] >= MAX_UART_DATA_SIZE - 1) {
            uart_config_array[0].uart_rx_data[uart_rx_length[0]] = '\0'; //Terminate the string
            uart_config_array[0].uart_rx_complete_flag = true; //Indicate that receiving is complete
            return;
        }
        uart_config_array[0].uart_rx_data[uart_rx_length[0]++] = data_rx;
    }

}//end uart0_rx_interrupt_handler

static void uart1_rx_interrupt_handler(void) {

    while (uart_is_readable(uart1)) {
        uint8_t data_rx = uart_getc(uart1); //Read the received data
        if(data_rx == uart_config_array[1].uart_terminator || uart_rx_length[1] >= MAX_UART_DATA_SIZE - 1) {
            uart_config_array[1].uart_rx_data[uart_rx_length[1]] = '\0'; //Terminate the string
            uart_config_array[1].uart_rx_complete_flag = true; //Indicate that receiving is complete
            return;
        }
        uart_config_array[1].uart_rx_data[uart_rx_length[1]++] = data_rx;
    }

}//end uart0_rx_interrupt_handler

//Function definition:

//UART hardware configuration:
int32_t configure_uart_hardware(uart_inst_t *uart_instance, uint uart_rx_pin, uint uart_tx_pin, uint baud_rate, uint data_bits, uint stop_bits, uart_parity_t parity, bool cts, bool rts, uint8_t termination) {
    
    uint return_baudrate = 0;
    uint8_t config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
        uart_config_array[config_index].uart_instance = uart0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
        uart_config_array[config_index].uart_instance = uart1;
    }
    else {
        return -1; //Error: Given parameter does not represent real hardware
    }

    if(uart_config_array[config_index].is_uart_configured) {
        return -2; //Error: Hardware is already configured
    }

    //TODO: Check Wrong Parameters: baud_rate, data_bits, stop_bits, parity,

    //TODO: Check if Hardware is claimed

    //Init UART0 and the GPIO Pins
        
    gpio_init(uart_rx_pin);
    gpio_init(uart_tx_pin);
    gpio_set_function(uart_rx_pin, GPIO_FUNC_UART);
    gpio_set_function(uart_tx_pin, GPIO_FUNC_UART);

    return_baudrate = uart_init(uart_config_array[config_index].uart_instance, 2400); //Init UART with  basic baudrate
    return_baudrate = uart_set_baudrate(uart_config_array[config_index].uart_instance, baud_rate); //Set the desired baudrate as close as possible
        
    uart_set_hw_flow(uart_config_array[config_index].uart_instance, cts, rts); //Set hardware flow control depending on the settings
    uart_set_format(uart_config_array[config_index].uart_instance, data_bits, stop_bits, parity); //Set Bits, Data, Stop, Parity
    uart_set_fifo_enabled(uart_config_array[config_index].uart_instance, false); //Disable the FIFO of UART

    //Save pin and termination byte information
    uart_config_array[config_index].uart_terminator = termination;
    uart_config_array[config_index].uart_rx_pin = uart_rx_pin;
    uart_config_array[config_index].uart_tx_pin = uart_tx_pin;

    //Setup UART-RX-Interrupt

    //Reset rx flags
    uart_config_array[config_index].uart_rx_complete_flag = false;
        
    uart_set_irq_enables(uart_config_array[config_index].uart_instance, true, false); // Enable UART RX interrupt, No UART TX interrupt
    if(config_index == 0) {
        irq_set_exclusive_handler(UART0_IRQ, uart0_rx_interrupt_handler); // Set interrupt handler
        irq_set_enabled(UART0_IRQ, true); // Enable UART interrupt in the processor
    }
    else {
        irq_set_exclusive_handler(UART1_IRQ, uart1_rx_interrupt_handler); // Set interrupt handler
        irq_set_enabled(UART1_IRQ, true); // Enable UART interrupt in the processor
    }
   

    //Clear input buffer
    clear_uart_buffer(uart_config_array[config_index].uart_rx_data);
       
    //Set flag
    uart_config_array[config_index].is_uart_configured = true;

    //Return no error
    return (int32_t)return_baudrate;

}//end configure_uart_hardware

int32_t reconfigure_uart_hardware(uart_inst_t *uart_instance, uint baud_rate, uint data_bits, uint stop_bits, uart_parity_t parity, bool cts, bool rts, uint8_t termination) {
    
    //TODO: Check for wrong parameters

    uint8_t config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
    }
    else {
        return -1; //Error: the given parameter does not represent real hardware
    }

    if(uart_config_array[config_index].is_uart_configured == false) {
        return -2; //Error: the hardware was not configured - no reconfiguration possible, configure the hardware first
    }

    if(deconfigure_uart_hardware(uart_config_array[config_index].uart_instance) < 0) {
        return -3; //Error: error in de-configuration of hardware check if parameters given ar right
    }

    //Configure uart with the given parameters and return
    return (int32_t)configure_uart_hardware(uart_config_array[config_index].uart_instance, uart_config_array[config_index].uart_tx_pin, uart_config_array[config_index].uart_rx_pin,
    baud_rate, data_bits, stop_bits, parity, cts, rts, termination);

}//end reconfigure_uart_hardware

int deconfigure_uart_hardware(uart_inst_t *uart_instance) {

    uint8_t config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
    }
    else {
        return -1; //Error: Given parameter does not represent real hardware
    }

    if(!(uart_config_array[config_index].is_uart_configured)) {
        return -2; //Error: Hardware was never configured, no need to de-configure
    }

    //TODO: Check if Hardware is claimed, if claimed release claim

    //If a transmission ist still going on wait till transmission is over
    uart_tx_wait_blocking(uart_config_array[config_index].uart_instance);

    //If there is still data in input buffer reset flag and clear buffer
    if(uart_config_array[config_index].uart_rx_complete_flag == true) {
        //Reset flag:
        uart_config_array[config_index].uart_rx_complete_flag = false;
        //Clear buffer
        clear_uart_buffer(uart_config_array[config_index].uart_rx_data);
    }

    //Deinit UART and the corresponding rx and tx pin
    gpio_deinit(uart_config_array[config_index].uart_rx_pin);
    gpio_deinit(uart_config_array[config_index].uart_tx_pin);
    uart_deinit(uart_config_array[config_index].uart_instance);

    //Disable UART-RX-Interrupt
    if(config_index == 0) {
        irq_set_enabled(UART0_IRQ, true);
        //Removes IRQ Handler 
        irq_remove_handler(UART0_IRQ, uart0_rx_interrupt_handler);
    }
    else {
        irq_set_enabled(UART1_IRQ, true);
        //Removes IRQ Handler 
        irq_remove_handler(UART1_IRQ, uart1_rx_interrupt_handler);
    }
    
    //Reset flag
    uart_config_array[config_index].is_uart_configured = false;
       
    return 1;

}//end deconfigure_uart_hardware

//UART Interrupt Handling
int enable_uart_interrupt(uart_inst_t *uart_instance, bool new_uart_rx_interrupt_state) {

    uint8_t config_index = 0;
    uint8_t irq_num = 0;

    if(uart_instance == uart0) {
        config_index = 0;
        irq_num = UART0_IRQ;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
        irq_num = UART1_IRQ;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(!(uart_config_array[config_index].is_uart_configured)) {
        return -2; //Error hardware is not configured
    }

    irq_set_enabled(irq_num, new_uart_rx_interrupt_state);
    return 1;

}//end enable_uart_interrupt

//UART RX:
bool uart_get_rx_complete_flag(uart_inst_t *uart_instance) {

    if(uart_instance == UART0_ID) {
        return uart_config_array[0].uart_rx_complete_flag;   
    }
    else if(uart_instance == UART1_ID) {
        return uart_config_array[1].uart_rx_complete_flag;
    }
    
    return false; //No RX-Data if wrong instance

}//end uart_get_rx_complete_flag

int uart_get_rx_data(uart_inst_t *uart_instance, uint8_t *rx_data) {

    uint config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(!(uart_config_array[config_index].is_uart_configured)) {
        return -2; //Error: hardware was not configured
    }

    if(!(uart_config_array[config_index].uart_rx_complete_flag)) {
        return -3; //Error: no rx data was received
    }

    //Write data to given array and reset static buffer
    for(uint i = 0; i < uart_rx_length[config_index]; i++) {
        rx_data[i] = uart_config_array[config_index].uart_rx_data[i];
        uart_config_array[config_index].uart_rx_data[i] = '\0';
    }

    //Reset length
    uart_rx_length[config_index] = 0;
    //Reset rx complete flag
    uart_config_array[config_index].uart_rx_complete_flag = false;

    return 1;

}//end uart_get_rx_data

//UART TX:
int inline uart_tx_data(uart_inst_t *uart_instance, uint8_t *tx_data) {

    uint8_t config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
    }
    else {
        return -1; //Error: Given parameter does not represent real hardware
    }

    if(!(uart_config_array[config_index].is_uart_configured)) {
        return -2; //Error: hardware is not configured
    }

    while(*tx_data) {
        uart_putc(uart_config_array[config_index].uart_instance, *tx_data++);
    }
    uart_putc(uart_config_array[config_index].uart_instance, uart_config_array[config_index].uart_terminator);


}//end uart_tx_data

int uart_tx_data_unterminated(uart_inst_t *uart_instance, uint8_t *tx_data) {

    uint8_t config_index = 0;

    if(uart_instance == uart0) {
        config_index = 0;
    }
    else if(uart_instance == uart1) {
        config_index = 1;
    }
    else {
        return -1; //Error: Given parameter does not represent real hardware
    }

    if(!(uart_config_array[config_index].is_uart_configured)) {
        return -2; //Error: hardware is not configured
    }

    while(*tx_data) {
        uart_putc(uart_config_array[config_index].uart_instance, *tx_data++);
    }

}//end uart_tx_data_unterminated

//Clear Buffer
void clear_uart_buffer(uint8_t *uart_buffer) {
    for(uint16_t k = 0; k < MAX_UART_DATA_SIZE; k++) {
        uart_buffer[k] = '\0';
    }

}//end clear_uart_buffer

//end file uart.c
