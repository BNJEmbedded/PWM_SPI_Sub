//File: uart.h
//Project: Pico_MRI_Test_M

/* Description:

     Custom uart-wrapper around the Raspberry-Pi-Pico-SDK (hardware/uart.h). 
    Let user configure UART, with various baud-rates, stop-bits, parity-bits with RX-Interrupt.
    After configuration user could send and receive data via UART.
    NOTE: This module is not multi core save.

    FUTURE_FEATURE: Allow user to set uart only in rx or tx mode, or maybe even on single wire mode
    FUTURE_FEATURE: Make this module multi-core save.

*/

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:

//Own Libraries:

//Preprocessor constants:
#define MAX_UART_DATA_SIZE 256

//Type definitions:

//Function Prototypes:

//UART hardware configuration:

/**
 * @brief Configures UART hardware with specified parameters.
 * 
 * @param uart_instance Pointer to the UART instance to configure.
 * @param uart_rx_pin GPIO pin used for UART RX.
 * @param uart_tx_pin GPIO pin used for UART TX.
 * @param baud_rate Baud rate for UART communication.
 * @param data_bits Number of data bits per frame.
 * @param stop_bits Number of stop bits per frame.
 * @param parity Parity setting for UART communication.
 * @param termination Termination byte for UART communication.
 * 
 * @return int32_t Returns the configured baud rate on success; returns a negative error code otherwise.
 * 
 */
int32_t configure_uart_hardware(uart_inst_t *uart_instance, uint uart_rx_pin, uint uart_tx_pin, uint baud_rate, uint data_bits, uint stop_bits, uart_parity_t parity, bool cts, bool rts, uint8_t termination);

/**
 * @brief Reconfigures UART hardware with new parameters.
 * 
 * @param uart_instance Pointer to the UART instance to reconfigure.
 * @param baud_rate New baud rate for UART communication.
 * @param data_bits New number of data bits per frame.
 * @param stop_bits New number of stop bits per frame.
 * @param parity New parity setting for UART communication.
 * @param termination New termination byte for UART communication.
 * 
 * @return int32_t Returns the configured baud rate on success; returns a negative error code otherwise.
 * 
 */
int32_t reconfigure_uart_hardware(uart_inst_t *uart_instance, uint baud_rate, uint data_bits, uint stop_bits, uart_parity_t parity, bool cts, bool rts, uint8_t termination);

/**
 * @brief De-configures the UART hardware.
 * 
 * @param uart_instance Pointer to the UART instance to de-configure (e.g., uart0, uart1).
 * 
 * @return int Returns 1 on successful de-configuration; returns a negative error code otherwise.
 * 
 */
int deconfigure_uart_hardware(uart_inst_t *uart_instance);

//UART Interrupt Handling

/**
 * @brief Enables or disables UART receive interrupts.
 * 
 * @param uart_instance Pointer to the UART instance.
 * @param new_uart_rx_interrupt_state Boolean value indicating whether to enable (true) or disable (false) UART receive interrupts.
 * 
 * @return int Returns 1 on successful operation; returns a negative error code otherwise.
 * 
 */
int enable_uart_interrupt(uart_inst_t *uart_instance, bool new_uart_rx_interrupt_state);


//UART RX:

/**
 * @brief Checks the status of the UART receive complete flag.
 * 
 * @param uart_instance Pointer to the UART instance (e.g., UART0_ID, UART1_ID).
 * 
 * @return bool Returns true if the UART receive complete flag is set; returns false otherwise.
 * 
 */
bool uart_get_rx_complete_flag(uart_inst_t *uart_instance);

/**
 * @brief Retrieves received data from the UART receive buffer.
 * 
 * @param uart_instance Pointer to the UART instance (e.g., uart0, uart1).
 * @param rx_data Pointer to the array where received data will be stored.
 * 
 * @return int Returns 1 if data was successfully retrieved; otherwise, returns an error code:
 *             -1: Given parameter does not represent real hardware.
 *             -2: Hardware was not configured.
 *             -3: No data was received.
 * 
 */
int uart_get_rx_data(uart_inst_t *uart_instance, uint8_t *rx_data);

//UART TX:
/**
 * @brief Transmits data over UART.
 * 
 * @param uart_instance Pointer to the UART instance (e.g., uart0, uart1).
 * @param tx_data Pointer to the array containing the data to be transmitted.
 * 
 * @return int Returns 1 upon successful transmission; otherwise, returns an error code:
 *             -1: Given parameter does not represent real hardware.
 *             -2: Hardware was not configured. 
 * 
 * @note Ensure that the UART hardware is properly configured before using this function.
 */
int uart_tx_data(uart_inst_t *uart_instance, uint8_t *tx_data);

/**
 * @brief Transmits data over UART without appending a terminator.
 * 
 * @param uart_instance Pointer to the UART instance (e.g., uart0, uart1).
 * @param tx_data Pointer to the array containing the data to be transmitted.
 * 
 * @return int Returns 1 upon successful transmission; otherwise, returns an error code:
 *             -1: Given parameter does not represent real hardware.
 *             -2: Hardware was not configured.
 * 
 * @note Ensure that the UART hardware is properly configured before using this function.
 */
int uart_tx_data_unterminated(uart_inst_t *uart_instance, uint8_t *tx_data);

/**
 * @brief Clears the UART buffer by setting all elements to null characters ('\0').
 * 
 * @param uart_buffer Pointer to the UART buffer array to be cleared.
 * 
 * This function clears the UART buffer by setting all elements to null characters ('\0').
 * The size of the buffer is determined by the constant MAX_UART_DATA_SIZE.
 */
void clear_uart_buffer(uint8_t *uart_buffer);

//end file uart.h
