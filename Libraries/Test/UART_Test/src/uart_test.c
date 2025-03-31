//File: uart_test.c
//Project: Pico_MRI_Test_M

/* Description:

    //TODO: Add Description

*/


//Corresponding header-file:
#include "uart_test.h"

//Libraries:

//Standard-C:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h"
#include "pico/rand.h"

//Pico Hardware-Libraries:
#include "hardware/watchdog.h"

//Own Libraries:
#include "uart.h"

//Preprocessor constants:


//File global (static) variables:

//Functions:

//File global (static) function definitions

//Function definitions:

int uart_test_echo_main(Uart_Test_Structure_t uart_tests, Uart_Test_Return_t *return_of_test, bool use_watchdog) {

    uint8_t tx_buffer[MAX_UART_DATA_SIZE];
    uint8_t rx_buffer[MAX_UART_DATA_SIZE];
    volatile bool loop_flag = false;

    //Get random seed for rng
    uint32_t rng_seed = get_rand_32();
    //Set rng with seed
    srand((unsigned int)rng_seed);

    //TODO: Check for error in configuration and return
    //Configure UART Hardware with baudrate and write the real configured baudrate to return of test baudrate
    return_of_test->baudrate = configure_uart_hardware(uart_tests.hardware.uart_instance, uart_tests.hardware.uart_rx_pin, uart_tests.hardware.uart_tx_pin, 
    uart_tests.parameter.parameter_baud_rate, 8, 1, UART_PARITY_NONE, false, false, '\n');
   
    //Send first some dummy bytes and receive
    uart_tx_data(uart_tests.hardware.uart_instance, "x");

    while(!loop_flag) {

        if(uart_get_rx_complete_flag(uart_tests.hardware.uart_instance)) {

            if(use_watchdog) {
                watchdog_update();
            }
            break;
        }
        tight_loop_contents();

    }

    uart_get_rx_data(uart_tests.hardware.uart_instance, rx_buffer);
    clear_uart_buffer(rx_buffer);
    clear_uart_buffer(tx_buffer);

    //Transfers: Get random tx-data and receive it back
    for(uint k = 0; k < uart_tests.parameter.number_of_transfers; k++) {

        //Clear result buffer
        clear_uart_buffer(return_of_test->rx_data[k]);
        clear_uart_buffer(return_of_test->tx_data[k]);

        for(uint l = 0; l < pow(2, k); l++) {
            tx_buffer[l] = ((rand()%(126 - 33 + 1)) + 33);
            return_of_test->tx_data[k][l] = tx_buffer[l];
        }

        //Send random data
        uart_tx_data(uart_tests.hardware.uart_instance, tx_buffer);
        clear_uart_buffer(tx_buffer);

        //Wait till sub sends back data
        while(!loop_flag) {

            if(uart_get_rx_complete_flag(uart_tests.hardware.uart_instance) == true) {
                if(use_watchdog) {
                    watchdog_update();
                }
                break;
            }

            tight_loop_contents();
        }
        //Save rx_data
        uart_get_rx_data(uart_tests.hardware.uart_instance, return_of_test->rx_data[k]);

        if(use_watchdog) {
            watchdog_update();
        }
     
    }

    //Deconfigure uart hardware
    deconfigure_uart_hardware(uart_tests.hardware.uart_instance);
    return 1; //No Error during Testing
    
}//end uart_test_echo_main

void uart_test_echo_sub(Uart_Test_Structure_t uart_tests, bool use_watchdog) {

    uint test_counter = 0;
  
    uint8_t rx_buffer[MAX_UART_DATA_SIZE];
    clear_uart_buffer(rx_buffer);
    //Configure UART Hardware with first test baudrate
    configure_uart_hardware(uart_tests.hardware.uart_instance, uart_tests.hardware.uart_rx_pin, uart_tests.hardware.uart_tx_pin, uart_tests.parameter.parameter_baud_rate,
    8, 1, UART_PARITY_NONE, false, false, '\n');

    while(test_counter < uart_tests.parameter.number_of_transfers+1) {

        //If uart receive something
        if(uart_get_rx_complete_flag(uart_tests.hardware.uart_instance) == true) {
            uart_get_rx_data(uart_tests.hardware.uart_instance, rx_buffer);
            //Echo Data back
            uart_tx_data(uart_tests.hardware.uart_instance, rx_buffer);
            clear_uart_buffer(rx_buffer);
            test_counter++;
        }

        clear_uart_buffer(rx_buffer);
        tight_loop_contents();
    }

    if(use_watchdog) {
        watchdog_update();
    }

    //Deconfigure uart hardware
    deconfigure_uart_hardware(uart_tests.hardware.uart_instance);
    return; //No Error during Testing

}//end uart_test_echo_sub

bool uart_compare_tx_rx_string(uint8_t *rx_data, uint8_t *tx_data) {
    if(strcmp(rx_data, tx_data) == 0) {
        return true;
    }
    return false;
}//end uart_compare_tx_rx
bool uart_compare_tx_rx_byte(uint8_t rx_data, uint8_t tx_data) {
    
    if(rx_data == tx_data) {
        return true;
    }
    return false;

}//end uart_compare_tx_rx

void uart_test_print_test_returns(Uart_Test_Return_t *test_return, uint8_t num_of_tests, uart_inst_t *uart_to_print, Uart_Test_Output_Format_t format) {
    uint8_t out_buff[MAX_UART_DATA_SIZE];
    uint8_t transfer_success_counter = 0;
    uint8_t transfer_failed_counter = 0;
    uint8_t transfer_byte_fail_counter = 0;
    uint8_t transfer_byte_success_counter = 0;
    uint8_t test_success_counter = 0;
    uint8_t test_failed_counter = 0;
    float percentage_of_transfers_failed = 0;
    float percentage_of_transfers_succeeded = 0;
    float byte_fail_rate = 0;
    float byte_success_rate = 0;

    uart_tx_data(uart_to_print, "#################UART-Test-Result-Print:######################");
    uart_tx_data(uart_to_print, "");
    sprintf(out_buff, "%ld tests performed with different baudrates. All tests are performed with Datasize from 1-Byte to %ld-Bytes", num_of_tests, (uint8_t)pow(2, test_return[0].num_of_transfers));
    uart_tx_data(uart_to_print, out_buff);
    clear_uart_buffer;
    uart_tx_data(uart_to_print, "");

    for(uint8_t n = 0; n < num_of_tests; n++) {
        sprintf(out_buff, "########################UART-Test Nr: %ld############################", n+1);
        uart_tx_data(uart_to_print, out_buff);
        clear_uart_buffer;
        uart_tx_data(uart_to_print, "");
        uart_tx_data(uart_to_print, "#################UART-Test-Parameter:#######################");
        sprintf(out_buff, "Baudrate: %ld", test_return[n].baudrate);
        uart_tx_data(uart_to_print, out_buff);
        clear_uart_buffer(out_buff);
        uart_tx_data(uart_to_print, "");
        uart_tx_data(uart_to_print, "################Start-UART-Test-Output-File#################");
        uart_tx_data(uart_to_print, "");
        uint8_t l = 0;
        if(format.single_byte == true) {
            for(uint k = 0; k < test_return[n].num_of_transfers; k++) {
                
               sprintf(out_buff, "#####################Transfer Nr. %ld, with %ld Bytes####################", k+1, (uint8_t) pow(2,k));
               uart_tx_data(uart_to_print, out_buff);
               clear_uart_buffer(out_buff);
               l = 0;
                for(uint j = 0; j < pow(2,k); j++) {

                    if(l < format.bytes_per_line) {  
                        sprintf(out_buff, "TX: 0x%02X RX: 0x%02X%c" ,test_return[n].tx_data[k][j], test_return[n].rx_data[k][j], format.separator);
                        uart_tx_data_unterminated(uart_to_print, out_buff);
                        clear_uart_buffer(out_buff);
                        l++;
                    }
                    else {
                        uart_tx_data(uart_to_print, "");
                        sprintf(out_buff, "TX: 0x%02X RX: 0x%02X%c" ,test_return[n].tx_data[k][j], test_return[n].rx_data[k][j], format.separator);
                        uart_tx_data_unterminated(uart_to_print, out_buff);
                        clear_uart_buffer(out_buff);
                        l = 1;
                    } 
                    if(uart_compare_tx_rx_byte(test_return[n].rx_data[k][j], test_return[n].tx_data[k][j])) {
                        transfer_byte_success_counter++;
                    }
                    else {
                        transfer_byte_fail_counter++; 
                    }
                }//end byte loop
                byte_success_rate = (transfer_byte_success_counter/pow(2,k))*100;
                byte_fail_rate = (transfer_byte_fail_counter/pow(2,k))*100;
                transfer_byte_fail_counter = 0;
                transfer_byte_success_counter = 0;
                uart_tx_data(uart_to_print, "");
                sprintf(out_buff, "######Transfer-nr.%ld, byte-success-rate: %f%c, byte-fail-rate %f%c######", k+1, byte_success_rate, '%',
                byte_fail_rate, '%');
                uart_tx_data(uart_to_print, out_buff);
                clear_uart_buffer(out_buff);
            }//end transfer loop
        }
        else {

            for(uint k = 0; k < test_return[n].num_of_transfers; k++) {

                sprintf(out_buff, "Transfer Nr: %ld", k+1);
                uart_tx_data(uart_to_print, out_buff);
                clear_uart_buffer(out_buff);
                sprintf(out_buff, "TX: %s" ,&test_return[n].tx_data[k][0]);
                uart_tx_data(uart_to_print, out_buff);
                clear_uart_buffer(out_buff);
                sprintf(out_buff, "RX: %s",  &test_return[n].rx_data[k][0]);
                uart_tx_data(uart_to_print, out_buff);
                clear_uart_buffer(out_buff);
                if(uart_compare_tx_rx_string(test_return[n].rx_data[k], test_return[n].tx_data[k])) {
                    sprintf(out_buff, "#####################Transfer Nr %ld: result: PASSED####################", k+1);
                    transfer_success_counter++;
                    uart_tx_data(uart_to_print, out_buff);
                    uart_tx_data(uart_to_print, "");
                    clear_uart_buffer(out_buff);
                }
                else {
                    sprintf(out_buff, "#####################Transfer Nr %ld: result: FAILED####################", k+1);
                    transfer_failed_counter++;
                    uart_tx_data(uart_to_print, out_buff);
                    uart_tx_data(uart_to_print, "");
                    clear_uart_buffer(out_buff);
                }    
            }//end transfer loop

            percentage_of_transfers_succeeded = (transfer_success_counter/test_return[n].num_of_transfers)*100;
            percentage_of_transfers_failed = (transfer_failed_counter/test_return[n].num_of_transfers)*100;
            transfer_success_counter = 0;
            transfer_failed_counter = 0;
            sprintf(out_buff, "Test Nr: %ld with baudrate %ld results: %f%c of transfers succeeded and %f%c of transfers failed.", n+1, 
            test_return[n].baudrate, percentage_of_transfers_succeeded, '%', percentage_of_transfers_failed, '%');
            uart_tx_data(uart_to_print, out_buff);
            clear_uart_buffer(out_buff);
        }
        uart_tx_data(uart_to_print, "");
        uart_tx_data(uart_to_print, "################End-UART-Test-Output-File#################");
    }//End test loop  

}//end uart_test_print_test_returns

//end file uart_test.c
