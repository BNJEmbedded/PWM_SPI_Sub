//File: adc_test.c
//Project: Pico_MRI_Test_M

/* Description:

    This module is for user communication, printing of the test results, and saving the test results on flash.
    This module uses the hardware specific test handler (ADC, SPI, UART).
    It provides functions to read user input (RTC Time, Settings).
    It provides functions to read the system parameters - system voltage, system clock frequency, system temperature
    It provides functions to save test results to flash (ADC; SPI, UART), and the test header (with time, test type, system parameters)
    It provides functions to print out - menus, test results (ADC, SPI, UART) and the test header (time,test type, system parameter)
    It provides functions to controller user LEDs

*/

//Corresponding header-file:
#include "test_suite.h"

//Libraries:

//Standard-C:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

//Pico Hardware-Libraries:
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "hardware/adc.h"

//Own Libraries:

//Utility:
#include "data_to_byte.h"
#include "statistic.h"

//Hardware:
#include "uart.h"
#include "adc.h"
#include "flash.h"

//Preprocessor constants:

//File global (static) variables:

//Functions:

//File global (static) function definitions:

//Function definition:

//Printing the menu over given uart instance
int test_suite_print_gui(uart_inst_t *uart_to_print, datetime_t *time, uint16_t num_of_test_saved_to_flash) {
    char uart_buffer[MAX_UART_DATA_SIZE];
    char time_buffer[50];
    datetime_to_str(time_buffer, 50, time);

    uart_tx_data(uart_to_print, "");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "-------------------------------Test Suite---------------------------------");
    sprintf(uart_buffer, "--------------------Time: %s-------------------", time_buffer);
    uart_tx_data(uart_to_print, uart_buffer);
    sprintf(uart_buffer, "--------------------Number of automatic test saved to flash: %ld-----------", num_of_test_saved_to_flash);
    uart_tx_data(uart_to_print, uart_buffer);
    uart_tx_data(uart_to_print, "-----------------------------Choose Command:------------------------------");
    uart_tx_data(uart_to_print, "-----------------'AUTOMATIC_TEST': Launch automatic test:-----------------");
    uart_tx_data(uart_to_print, "-------------------------'UART': Launch UART test:------------------------");
    uart_tx_data(uart_to_print, "---------------------------'SPI': Launch SPI test:------------------------");
    uart_tx_data(uart_to_print, "---------------------------'ADC': Launch ADC test:------------------------");
    uart_tx_data(uart_to_print, "-------------'FLASH_DELETE': Delete stored test data in flash:------------");
    uart_tx_data(uart_to_print, "---------------'FLASH_LOAD': Load last test result to flash:--------------");
    uart_tx_data(uart_to_print, "---------------'GET_SYSTEM_PARAMETER': Get system parameter:--------------");
    uart_tx_data(uart_to_print, "----------'CHANGE_SETTINGS': Change the settings of test suite:-----------");
    uart_tx_data(uart_to_print, "---------'GET_SETTINGS': Get the current settings of test suite:----------");
    uart_tx_data(uart_to_print, "-------------------------'RESET': Reset MCU:------------------------------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "");

}//end test_suite_print_gui

//Change settings menu
int print_change_settings_menu(uart_inst_t *uart_to_print) {

    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "--------------------------Test Suite settings-----------------------------");
    uart_tx_data(uart_to_print, "-----------------------------Choose Command:------------------------------");
    uart_tx_data(uart_to_print, "--------------'TIME': Change time and date of test suite:-----------------");
    uart_tx_data(uart_to_print, "----'NUMBER_OF_TESTS': Change the number of automatic Tests performed:----");
    uart_tx_data(uart_to_print, "-------'DELAY': Change the delay in ms between  automatic tests:----------");
    uart_tx_data(uart_to_print, "---'TOGGLE_SAVE_TO_FLASH': Toggles the setting which determines if test---");
    uart_tx_data(uart_to_print, "--------------------results are stored to flash after test:---------------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "---'TOGGLE_PRINT_TO_UART': Toggles the setting which determines if test---");
    uart_tx_data(uart_to_print, "--------------------results are printed to uart after test----------------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "--'TOGGLE_UART_CONNECTION_DISABLE': Toggles the setting which determines--");
    uart_tx_data(uart_to_print, "--------------------if UART is disabled during auto mode testing.---------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "'TOGGLE_UART_TEST_IN_AUTOMATIC_TEST': Toggles the setting which determines");
    uart_tx_data(uart_to_print, "-------------------if UART Test is enabled during automatic test.---------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "'TOGGLE_SPI_TEST_IN_AUTOMATIC_TEST': Toggles the setting which determines-");
    uart_tx_data(uart_to_print, "-------------------if SPI Test is enabled during automatic test.----------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "'TOGGLE_ADC_TEST_IN_AUTOMATIC_TEST': Toggles the setting which determines-");
    uart_tx_data(uart_to_print, "-------------------if ADC Test is enabled during automatic test.----------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    uart_tx_data(uart_to_print, "---------------------'EXIT': Exit settings menu:--------------------------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");

}//end print_change_settings_menu

//Gets command from input string
Menu_Command_t get_command_from_string(char *command_string) {

    if(strcmp(command_string, "AUTOMATIC_TEST") == 0) {
        return LAUNCH_AUTOMATIC_TEST;
    }
    else if(strcmp(command_string, "UART") == 0) {
        return LAUNCH_UART_TEST;
    }
    else if(strcmp(command_string, "SPI") == 0) {
        return LAUNCH_SPI_TEST;
    }
    else if(strcmp(command_string, "ADC") == 0) {
        return LAUNCH_ADC_TEST;
    }
    else if(strcmp(command_string, "FLASH_DELETE") == 0) {
        return DELETE_TEST_RESULTS_FROM_FLASH;
    }
    else if(strcmp(command_string, "FLASH_LOAD") == 0) {
        return LOAD_TEST_RESULTS_FROM_FLASH;
    }
    else if(strcmp(command_string, "GET_SYSTEM_PARAMETER") == 0) {
        return GET_SYSTEM_PARAMETER;
    }
    else if(strcmp(command_string, "CHANGE_SETTINGS") == 0) {
        return CHANGE_SETTINGS;
    }
    else if(strcmp(command_string, "RESET") == 0) {
        return RESET_TEST_SUITE;
    }
    else if(strcmp(command_string, "GET_SETTINGS") == 0) {
        return GET_SETTINGS;
    }
    else {
        return NOT_DEFINED_COMMAND;
    }

}//end get_menu_command

Settings_Menu_Command_t get_test_settings_menu_command_from_string(char *command_string) {
   
    if(strcmp(command_string, "TIME") == 0) {
        return CHANGE_TIME;
    }
    else if(strcmp(command_string, "NUMBER_OF_TESTS") == 0) {
        return CHANGE_NUMBER_OF_TESTS;
    }
    else if(strcmp(command_string, "DELAY") == 0) {
        return CHANGE_DELAY;
    }
    else if(strcmp(command_string, "TOGGLE_SAVE_TO_FLASH") == 0) {
        return TOGGLE_WRITE_TO_FLASH;
    }
    else if(strcmp(command_string, "TOGGLE_PRINT_TO_UART") == 0) {
        return TOGGLE_PRINT_TO_UART;
    }
    else if(strcmp(command_string, "TOGGLE_UART_CONNECTION_DISABLE") == 0) {
        return TOGGLE_UART_CONNECTION_DISABLE;
    }
    else if(strcmp(command_string, "TOGGLE_UART_TEST_IN_AUTOMATIC_TEST") == 0) {
        return TOGGLE_UART_TEST_IN_AUTOMATIC_TEST;
    }
    else if(strcmp(command_string, "TOGGLE_SPI_TEST_IN_AUTOMATIC_TEST") == 0) {
        return TOGGLE_SPI_TEST_IN_AUTOMATIC_TEST;
    }
    else if(strcmp(command_string, "TOGGLE_ADC_TEST_IN_AUTOMATIC_TEST") == 0) {
        return TOGGLE_ADC_TEST_IN_AUTOMATIC_TEST;
    }
    else if(strcmp(command_string, "EXIT") == 0) {
        return EXIT_SETTINGS_MENUE;
    }
    else {
        return NOT_DEFINED_COMMAND_SM;
    }

}//end get_test_settings_menu_command_from_string

//Get settings from user
int get_number_of_tests_from_user(Test_Suite_Settings_t *settings, uart_inst_t *user_uart) {

    volatile bool exit = false;
    uint8_t buff;
    char uart_rx_buff[MAX_UART_DATA_SIZE];

    sprintf(uart_rx_buff, "Put in the number of tests that are performed in one automatic test, note the maximum amount is %ld:", MAX_NUMBER_OF_AUTOMATIC_TESTS);
    uart_tx_data(user_uart, uart_rx_buff);
    clear_uart_buffer(uart_rx_buff);

    while(!exit) {
        if(uart_get_rx_complete_flag(user_uart)) {
            uart_get_rx_data(user_uart, uart_rx_buff);
            buff = (uint8_t)atoi(uart_rx_buff);
            if(buff > MAX_NUMBER_OF_AUTOMATIC_TESTS) {
                return -1;
            }
            
            settings->num_of_automatic_tests = buff;
            exit = true;
        }
    }

    return 1;

}//end get_number_of_tests_from_user
int get_delay_between_test_from_user(Test_Suite_Settings_t *settings, uart_inst_t *user_uart) {

    volatile bool exit = false;
    uint16_t buff;
    char uart_rx_buff[MAX_UART_DATA_SIZE];

    sprintf(uart_rx_buff, "Put in the delay time in ms between tests that are performed in one automatic test, note the maximum amount is %ld:", MAX_DELAY_BETWEEN_TESTS_IN_MS);
    uart_tx_data(user_uart, uart_rx_buff);
    clear_uart_buffer(uart_rx_buff);

    while(!exit) {
        if(uart_get_rx_complete_flag(user_uart)) {
            uart_get_rx_data(user_uart, uart_rx_buff);
            buff = (uint16_t)atoi(uart_rx_buff);
            if(buff > MAX_DELAY_BETWEEN_TESTS_IN_MS) {
                return -1;
            }
            settings->delay_between_single_tests_in_ms = buff;
            exit = true;
        }
    }

    return 1;

}//end get_delay_between_test_from_user

//Printing settings
int print_settings(Test_Suite_Settings_t settings, uart_inst_t *uart_to_print) {

    char uart_buffer[MAX_UART_DATA_SIZE];

    uart_tx_data(uart_to_print, "-----------------------------Current settings:----------------------------");
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");
    sprintf(uart_buffer, "-----------------Number of tests in automatic tests: %2ld-------------------", settings.num_of_automatic_tests);
    uart_tx_data(uart_to_print, uart_buffer);
    sprintf(uart_buffer, "-----------------------Delay between tests is: %5ld ms-------------------", settings.delay_between_single_tests_in_ms);
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.print_test_results_after_tests) {
        sprintf(uart_buffer, "-----------------------Print to UART after test: ON-----------------------");
    }
    else {
        sprintf(uart_buffer, "-----------------------Print to UART after test: OFF----------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.save_test_results_to_flash_after_test) {
        sprintf(uart_buffer, "-----------------------Save to flash after test: ON-----------------------");
    }
    else {
        sprintf(uart_buffer, "-----------------------Save to flash after test: OFF----------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.disable_uart_in_auto_mode) {
        sprintf(uart_buffer, "-----------------------UART in auto mode: DISABLED------------------------");
    }
    else {
        sprintf(uart_buffer, "-----------------------UART in auto mode: ENABLED-------------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.test_choosing.UART_enabled) {
        sprintf(uart_buffer, "---------------UART Test in Automatic Test: ENABLED-----------------------");
    }
    else {
        sprintf(uart_buffer, "---------------UART Test in Automatic Test: DISABLED----------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.test_choosing.SPI_enabled) {
        sprintf(uart_buffer, "-----------------SPI Test in Automatic Test: ENABLED----------------------");
    }
    else {
        sprintf(uart_buffer, "-----------------SPI Test in Automatic Test: DISABLED---------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    if(settings.test_choosing.ADC_enabled) {
        sprintf(uart_buffer, "-----------------ADC Test in Automatic Test: ENABLED----------------------");
    }
    else {
        sprintf(uart_buffer, "-----------------ADC Test in Automatic Test: DISABLED---------------------");
    }
    uart_tx_data(uart_to_print, uart_buffer);
    uart_tx_data(uart_to_print, "--------------------------------------------------------------------------");


}//end print_settings

//Get system parameter function
int get_system_parameters(System_Parameters_t *system_parameters) {
    
    uint16_t temperature_samples_bytes[NUM_OF_SAMPLES_SYSTEM_PARAMETER];
    uint8_t system_voltage_samples_bytes[NUM_OF_SAMPLES_SYSTEM_PARAMETER];
    
    float temperature_samples[NUM_OF_SAMPLES_SYSTEM_PARAMETER];
    float temp_test_samples[NUM_OF_SAMPLES_SYSTEM_PARAMETER];
    float system_voltage_samples[NUM_OF_SAMPLES_SYSTEM_PARAMETER];

    float temperature_mean = 0;
    float vsys_mean = 0;
    float temperature_sigma = 0;
    float vsys_sigma = 0;

    int return_value = 0;

    //Configure adc as single channel
    return_value = configure_adc_single_channel(3, 500*1000, 0, 1);
    if(return_value < 0) {
        return return_value;
    }

    adc_start_measurement(true);
    busy_wait_us_32(2);
    for(uint16_t k = 0; k < NUM_OF_SAMPLES_SYSTEM_PARAMETER; k++) {
        adc_get_gpio_measurement(29, &system_voltage_samples_bytes[k]);
        busy_wait_us_32(4);
    }
    adc_start_measurement(false);

    return_value = deconfigure_adc();
    if(return_value < 0) {
        return return_value;
    }
   
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    for(uint16_t p = 0; p < NUM_OF_SAMPLES_SYSTEM_PARAMETER; p++) {
        float temp_volt = 0;
        temperature_samples_bytes[p] = adc_read();
        busy_wait_us_32(4);
    }

    adc_set_temp_sensor_enabled(false);
    
    for(uint16_t k = 0; k < NUM_OF_SAMPLES_SYSTEM_PARAMETER; k++) {

        float temp_voltage = 0;
        system_voltage_samples[k] = (((float)(system_voltage_samples_bytes[k])*3.261*3)/(255));
        temp_voltage = (((float)(temperature_samples_bytes[k])*3.261)/(4095));
        temperature_samples[k] = 27 - (temp_voltage-0.706)/0.001721;
    }

    vsys_mean = get_mean_value(system_voltage_samples, NUM_OF_SAMPLES_SYSTEM_PARAMETER);
    temperature_mean = get_mean_value(temperature_samples, NUM_OF_SAMPLES_SYSTEM_PARAMETER);
    vsys_sigma = get_std_deviation(system_voltage_samples, NUM_OF_SAMPLES_SYSTEM_PARAMETER);
    temperature_sigma = get_std_deviation(temperature_samples, NUM_OF_SAMPLES_SYSTEM_PARAMETER);

    system_parameters->system_voltage = vsys_mean;
    system_parameters->sigma_system_voltage = vsys_sigma;
    system_parameters->system_temperature = temperature_mean;
    system_parameters->sigma_system_temperature = temperature_sigma;
    system_parameters->system_clock_frequency = (float)clock_get_hz(clk_sys);

    return 1;

}//end get system parameters
int print_system_parameters(System_Parameters_t system_parameter, uart_inst_t *uart_to_print) {
    uint8_t out_buff[MAX_UART_DATA_SIZE];
     
    uart_tx_data(uart_to_print, "################################System-Parameter:####################################");
    sprintf(out_buff, "############################System-CLK: %f MHz###############################", system_parameter.system_clock_frequency/MHZ);
    uart_tx_data(uart_to_print, out_buff);
    sprintf(out_buff, "####################Temperature: %f °C +/- %f °C########################", system_parameter.system_temperature, 2*system_parameter.sigma_system_temperature);
    uart_tx_data(uart_to_print, out_buff);
    sprintf(out_buff, "#########################Voltage: %f V +/- %f V##########################", system_parameter.system_voltage, 2*system_parameter.sigma_system_voltage);
    uart_tx_data(uart_to_print, out_buff);

}

//Get RTC time and date from user
int get_rtc_time_date_from_user(uart_inst_t *user_uart, datetime_t *date_time) {
    uint8_t rx_buffer[MAX_UART_DATA_SIZE];
    uint8_t buff[2];
    uint8_t year_buff[4];
    uint8_t weekday_buffer[20];

    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t weekday = 0;

    volatile bool exit = false;
   
    uart_tx_data(user_uart, "Put in actual time and date in the following form:");
    uart_tx_data(user_uart, "dd.mm.yyyy-hh:mm:ss-weekday");

    clear_uart_buffer(rx_buffer);

    while(!exit) {

        if(uart_get_rx_complete_flag(user_uart)) {

            uart_get_rx_data(user_uart, rx_buffer);
        
            if(isdigit(rx_buffer[0]) && isdigit(rx_buffer[1])) {
                buff[0] = rx_buffer[0];
                buff[1] = rx_buffer[1];
                day = atoi(buff);
                if(day > 31 || day == 0) {
                    return -1; //Error: there are no days with 0 or more then 31 days
                } 
            }
            else {
                return -1;
            }
            if(isdigit(rx_buffer[3]) && isdigit(rx_buffer[4])) {
                buff[0] = rx_buffer[3];
                buff[1] = rx_buffer[4];
                month = atoi(buff);
                if(month > 12 || month == 0) {
                    return -1; //Error: there are no days with 0 or
                }
                if(month == 2 || month == 4 || month == 6 || month == 9 || month == 11) {
                    if(day == 31) {
                        return -1; //Error: there are no 31 days in these months
                    }
                }
            }
            else {
                return -1;
            }
            if(isdigit(rx_buffer[6]) && isdigit(rx_buffer[7])&& isdigit(rx_buffer[8]) && isdigit(rx_buffer[9])) {
                year_buff[0] = rx_buffer[6];
                year_buff[1] = rx_buffer[7];
                year_buff[2] = rx_buffer[8];
                year_buff[3] = rx_buffer[9];
                year = atoi(year_buff);
                if(year < 2024) {
                    return -1; //Error: this could not be at the time writing this code the year is 2024
                } 
            }     
            else {
                return -1;
            } 
            if(isdigit(rx_buffer[11]) && isdigit(rx_buffer[12])) {
                buff[0] = rx_buffer[11];
                buff[1] = rx_buffer[12];
                hour = atoi(buff);
                if(hour > 23) {
                    return -1; //Error: There are only 0 to 23 hours
                } 
            }
            else {
                return -1;
            }
            if(isdigit(rx_buffer[14]) && isdigit(rx_buffer[15])) {
                buff[0] = rx_buffer[14];
                buff[1] = rx_buffer[15];
                minute = atoi(buff);
                if(minute > 59) {
                    return -1; //Error: Max minutes is 60
                } 
            }
            else {
                return -1;
            }
            if(isdigit(rx_buffer[17]) && isdigit(rx_buffer[18])) {
                buff[0] = rx_buffer[16];
                buff[1] = rx_buffer[17];
                second = atoi(buff);
                if(second > 59) {
                    return -1; //Error: Max seconds is 60
                } 
            }
            else {
                return -1;
            }
            uint8_t j = 0;
            for(uint8_t k = 20; k < 30; k++) {
                weekday_buffer[j] = rx_buffer[k];
                j++;
            }

            if(strcmp(weekday_buffer, "Monday") == 0) {
                weekday = 1;
            }
            else if(strcmp(weekday_buffer, "Tuesday") == 0) {
                weekday = 2;
            }
            else if(strcmp(weekday_buffer, "Wednesday") == 0) {
                weekday = 3;
            }
            else if(strcmp(weekday_buffer, "Thursday") == 0) {
                weekday = 4;
            }
            else if(strcmp(weekday_buffer, "Friday") == 0) {
                weekday = 5;
            }
            else if(strcmp(weekday_buffer, "Saturday") == 0) {
                weekday = 6;
            }
            else if(strcmp(weekday_buffer, "Sunday") == 0) {
                weekday = 0;
            }
            else {
                return -1; //Error: wrong name for weekday
            }

            date_time->day = day;
            date_time->dotw = weekday;
            date_time->month = month;
            date_time->year = year;
            date_time->sec = second;
            date_time->min = minute;
            date_time->hour = hour;
            exit = true;
        }

        tight_loop_contents();

    }

    return 1;

}//end get_rtc_time_date_from_user
void print_date_time(uart_inst_t *uart_to_print, datetime_t *date_time) {

    char rtc_date_time_string[50];
    datetime_to_str(rtc_date_time_string, 50, date_time);
    uart_tx_data_unterminated(uart_to_print, rtc_date_time_string);


}//end print_date_time

//Get Test Header
int get_test_header(Test_Header_t *header, Test_Type_t test_type, uint8_t number_of_tests_to_perform, datetime_t date_time) {

    int return_value = 0;

    //Get system parameters: temperature and system voltage
    return_value = get_system_parameters(&header->system_parameter);
    if(return_value < 0) {
        return return_value;
    }

    //Assign time stamp from given date_time to header
    header->time_stamp.day = date_time.day;
    header->time_stamp.dotw = date_time.dotw;
    header->time_stamp.month = date_time.month;
    header->time_stamp.year = date_time.year;
    header->time_stamp.hour = date_time.hour;
    header->time_stamp.min = date_time.min;
    header->time_stamp.sec = date_time.sec;

    //Assign test type
    header->type = test_type;

    return 1;

}//end get_test_header
int print_test_header(Test_Header_t test_header, uart_inst_t *uart_to_print) {

    uint8_t test_type_string[10];
    uint8_t out_buff[MAX_UART_DATA_SIZE];
    uint16_t test_counter = 0;
    clear_uart_buffer(out_buff);
    
    switch(test_header.type) {
        case UART:  sprintf(test_type_string, "UART");
                    break;
        case SPI:   sprintf(test_type_string, "SPI");
                    break;
        case ADC:   sprintf(test_type_string, "ADC");
                    break;
        default:    return -1; //Error: Wrong Parameter
    }
    uart_tx_data(uart_to_print, "");
    sprintf(out_buff, "Test Type: %s", test_type_string);
    uart_tx_data_unterminated(uart_to_print, out_buff);
    clear_uart_buffer(out_buff);
    sprintf(out_buff, " Time: ");
    uart_tx_data_unterminated(uart_to_print, out_buff);
    clear_uart_buffer(out_buff);
    print_date_time(uart_to_print, &test_header.time_stamp);
    sprintf(out_buff, " MRI-Sequence: FILL IN MRI SEQUENCE AFTER TEST");
    uart_tx_data(uart_to_print, out_buff);
    clear_uart_buffer(out_buff);
    print_system_parameters(test_header.system_parameter, uart_to_print);
    uart_tx_data(uart_to_print, "");

    return 1;
    
}//end print_test_header

//Printing test results from test returns with header
int print_uart_test_results(Test_Header_t header, Uart_Test_Return_t *test_return, uint8_t num_of_tests, Uart_Test_Output_Format_t output_format, uart_inst_t *uart_to_print) {

    print_test_header(header, uart_to_print);
    uart_test_print_test_returns(test_return, num_of_tests, uart_to_print, output_format);
    return 1;

}//end print_uart_test_results
int print_spi_test_results(Test_Header_t header, SPI_Test_Return_t *test_return, uint8_t num_of_tests, SPI_Test_Output_Format_t output_format, uart_inst_t *uart_to_print) {

    print_test_header(header, uart_to_print);
    spi_test_print_test_returns(test_return, num_of_tests, uart_to_print, output_format);
    return 1;

}//end print_spi_test_results
int print_adc_test_results(Test_Header_t header, ADC_Test_Return_t *test_return, uint8_t num_of_tests, ADC_Test_Output_Format_t output_format, uart_inst_t *uart_to_print) {

    print_test_header(header, uart_to_print);
    adc_test_print_test_returns(test_return, uart_to_print, output_format);
    return 1;


}//end print_adc_test_results

//Flash test results functions

//Saving uart test results to flash and load uart test results from flash
int save_uart_test_results_to_flash(Uart_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t *flash_read_address) {
   
    //Array of bytes to store to flash
    uint8_t uart_byte_stream[8000];
    uint16_t stream_count = 0;
    uint8_t byte_buff[4];
    int return_value = 0;

    for(uint8_t l = 0; l < number_of_tests; l++) {
        
        //Write baudrate to stream
        uint32_t_to_byte_array(test_returns[l].baudrate, byte_buff);
        for(uint8_t p = 0; p < 4; p++) {
            uart_byte_stream[stream_count] = byte_buff[p];
            stream_count++;
        }
        uart_byte_stream[stream_count] = test_returns[l].num_of_transfers;
        stream_count++;

        //Write first rx data and then tx data to byte stream for all transfers
        for(uint16_t k = 0; k < test_returns[l].num_of_transfers; k++) {
            for(uint16_t j = 0; j < pow(2, k); j++) {
                uart_byte_stream[stream_count] = test_returns[l].rx_data[k][j];
                stream_count++;
                uart_byte_stream[stream_count] = test_returns[l].tx_data[k][j];
                stream_count++;
            }
        }
    }
    
    return_value = write_bytes_to_flash(flash_read_address, uart_byte_stream, stream_count);

    if(return_value > 0) {
        return stream_count;
    }

    return return_value;

}//end save_uart_test_results_to_flash
int read_uart_test_results_from_flash(Uart_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash) {

    //Array of bytes to load from flash
    uint8_t uart_byte_stream[8000];
    int ret_value = 0;
    uint8_t byte_buff[4];
    uint16_t stream_count = 0;

    //Read bytes from flash
    ret_value = read_bytes_from_flash(flash_read_address, number_of_bytes_written_to_flash, uart_byte_stream); 
    
    if(ret_value < 0) {
        return ret_value;
    }
   
    //Get baudrate and number of transfers
    for(uint8_t l = 0; l < number_of_tests; l++) {

        for(uint8_t p = 0; p < 4; p++) {
            byte_buff[p] = uart_byte_stream[stream_count];
            stream_count++;
        }
        test_returns[l].baudrate = byte_array_to_uint32_t(byte_buff);
        test_returns[l].num_of_transfers = uart_byte_stream[stream_count];
        stream_count++;
        

        //Write bytes from byte stream that was loaded from flash and store it in the right field
        for(uint16_t k = 0; k < test_returns[l].num_of_transfers; k++) {
            for(uint16_t j = 0; j < pow(2, k); j++) {
                test_returns[l].rx_data[k][j] = uart_byte_stream[stream_count];
                stream_count++;
                test_returns[l].tx_data[k][j] = uart_byte_stream[stream_count];
                stream_count++;
            }
        }

    }
    return 1;
}//end read_uart_test_results_from_flash

//Saving spi test results to flash and load spi test results from flash
int save_spi_test_results_to_flash(SPI_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t *flash_read_address) {
    //Array of bytes to store to flash
    uint8_t spi_byte_stream[8000];
    uint16_t stream_count = 0;
    uint8_t byte_buff[4];
    int return_value = 0;

    for(uint8_t l = 0; l < number_of_tests; l++) {

        //Write frequency to stream
        uint32_t_to_byte_array(test_returns[l].spi_clk_frequency, byte_buff);
        for(uint8_t p = 0; p < 4; p++) {
            spi_byte_stream[stream_count] = byte_buff[p];
            stream_count++;
        }
        spi_byte_stream[stream_count] = test_returns[l].num_of_transfers;
        stream_count++;

        //Write first rx data and then tx data to byte stream for all transfers
        for(uint16_t k = 0; k < test_returns[l].num_of_transfers; k++) {
            for(uint16_t j = 0; j < pow(2, k+2); j++) {
                spi_byte_stream[stream_count] = test_returns[l].rx_data_from_sub_to_main[k][j];
                stream_count++;
                spi_byte_stream[stream_count] = test_returns[l].tx_data_from_main_to_sub[k][j];
                stream_count++;
            }
        }
    }
    
    return_value = write_bytes_to_flash(flash_read_address, spi_byte_stream, stream_count);
    if(return_value > 0) {
        return (int)stream_count;
    }
    
    return return_value;
}//end save_spi_test_results_to_flash
int read_spi_test_results_from_flash(SPI_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash) {

    //Array of bytes to load from flash
    uint8_t spi_byte_stream[8000];
    int ret_value = 0;
    uint8_t byte_buff[4];
    uint16_t stream_count = 0;

    //Read bytes from flash
    ret_value = read_bytes_from_flash(flash_read_address, number_of_bytes_written_to_flash, spi_byte_stream); 

    if(ret_value < 0) {
        return ret_value;
    }

    //Get clk frequencies and number of transfers
    for(uint8_t l = 0; l < number_of_tests; l++) {
        
        for(uint8_t p = 0; p < 4; p++) {
            byte_buff[p] = spi_byte_stream[stream_count];
            stream_count++;
        }
        
        test_returns[l].spi_clk_frequency = byte_array_to_uint32_t(byte_buff);
        test_returns[l].num_of_transfers = spi_byte_stream[stream_count];
        stream_count++;

        //Write bytes from byte stream that was loaded from flash and store it in the right field
        for(uint16_t k = 0; k < test_returns[l].num_of_transfers; k++) {
            for(uint16_t j = 0; j < pow(2, k+2); j++) {
                test_returns[l].rx_data_from_sub_to_main[k][j] = spi_byte_stream[stream_count];
                stream_count++;
                test_returns[l].tx_data_from_main_to_sub[k][j] = spi_byte_stream[stream_count];
                stream_count++;
            }
        }

    }
    return 1;

}//end read_spi_test_results_from_flash

int save_adc_test_results_to_flash(ADC_Test_Return_t *test_returns, uint32_t *flash_read_address) {
    //Array of bytes to store to flash
    #ifndef USE_OPTIMIZED_SETUP
    uint8_t adc_byte_stream[MAX_ADC_SAMPLES_PER_RAMP*MAX_ADC_SAMPLES_PER_RAMP_STEP];
    #else
    //Floats are four byte in size, the structure stores two float values
    uint8_t adc_byte_stream[2*MAX_ADC_SAMPLES_PER_RAMP*4];
    #endif

    uint16_t stream_count = 0;
    int return_value = 0;

    #ifndef USE_OPTIMIZED_SETUP
    for(uint32_t k = 0; k < test_returns->number_of_samples; k++) {
        adc_byte_stream[k] = test_returns->adc_samples[k];
    }
    return_value = write_bytes_to_flash(flash_read_address, adc_byte_stream, test_returns->number_of_samples);
    #else 
    uint32_t j = 0;
    uint8_t mean_byte_buff[4];
    uint8_t std_byte_buff[4];
    for(uint32_t k = 0; k < (2*(test_returns->number_of_samples_per_ramp)*4); k += 8) {

        //Convert mean value and std-dev from float to byte arrays
        float_to_byte_array(test_returns->mean_value[j], mean_byte_buff);
        float_to_byte_array(test_returns->std_deviation[j], std_byte_buff);
    
        //Store float bytes of mean and std deviation into byte stream
        adc_byte_stream[k] = mean_byte_buff[0];
        adc_byte_stream[k+1] = mean_byte_buff[1];
        adc_byte_stream[k+2] = mean_byte_buff[2];
        adc_byte_stream[k+3] = mean_byte_buff[3];
        adc_byte_stream[k+4] = std_byte_buff[0];
        adc_byte_stream[k+5] = std_byte_buff[1];
        adc_byte_stream[k+6] = std_byte_buff[2];
        adc_byte_stream[k+7] = std_byte_buff[3];
        j++;
    }
    return_value = write_bytes_to_flash(flash_read_address, adc_byte_stream, 2*test_returns->number_of_samples_per_ramp*4);
    #endif
    
    if(return_value > 0) {
        return (int)(2*test_returns->number_of_samples_per_ramp*4);
    }
    
    return return_value;
}//end save_adc_test_results_to_flash
int read_adc_test_results_from_flash(ADC_Test_Return_t *test_returns, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash) {
    #ifndef USE_OPTIMIZED_SETUP
    uint8_t adc_byte_stream[MAX_ADC_SAMPLES_PER_RAMP*MAX_ADC_SAMPLES_PER_RAMP_STEP];
    #else
    uint8_t adc_byte_stream[2*MAX_ADC_SAMPLES_PER_RAMP*4];
    #endif

    int ret_value = 0;
    //Read bytes from flash
    ret_value = read_bytes_from_flash(flash_read_address, number_of_bytes_written_to_flash, adc_byte_stream); 

    if(ret_value < 0) {
        return ret_value;
    }

    #ifndef USE_OPTIMIZED_SETUP
    for(uint32_t k = 0; k < number_of_bytes_written_to_flash; k++) {
        test_returns->adc_samples[k] = adc_byte_stream[k];
    }
    #else
    uint32_t j = 0;
    uint8_t mean_byte_buff[4];
    uint8_t std_byte_buff[4];
    for(uint32_t k = 0; k < number_of_bytes_written_to_flash; k += 8) {

        mean_byte_buff[0] = adc_byte_stream[k];
        mean_byte_buff[1] = adc_byte_stream[k+1];
        mean_byte_buff[2] = adc_byte_stream[k+2];
        mean_byte_buff[3] = adc_byte_stream[k+3];
        std_byte_buff[0] = adc_byte_stream[k+4];
        std_byte_buff[1] = adc_byte_stream[k+5];
        std_byte_buff[2] = adc_byte_stream[k+6];
        std_byte_buff[3] = adc_byte_stream[k+7];

        test_returns->mean_value[j] = byte_array_to_float(mean_byte_buff);
        test_returns->std_deviation[j] = byte_array_to_float(std_byte_buff);
        j++;
    }
    #endif

    return 1;
}//end save_adc_test_results_from_flash

int save_test_header_to_flash(Test_Header_t *test_header, uint8_t num_of_headers, uint32_t *flash_read_address) {
    
    //Array of bytes to store to flash
    uint8_t header_byte_stream[sizeof(Test_Header_t)*3];
    uint16_t stream_count = 0;
    int return_value = 0;
    uint8_t byte_buffer[4];

        for(uint8_t n = 0; n < num_of_headers; n++) {

        //Write system parameter to stream
        float_to_byte_array(test_header[n].system_parameter.system_clock_frequency, byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            header_byte_stream[stream_count] = byte_buffer[k];
            stream_count++;
        }
        float_to_byte_array(test_header[n].system_parameter.system_temperature, byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            header_byte_stream[stream_count] = byte_buffer[k];
            stream_count++;
        }
        float_to_byte_array(test_header[n].system_parameter.sigma_system_temperature, byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            header_byte_stream[stream_count] = byte_buffer[k];
            stream_count++;
        }
        float_to_byte_array(test_header[n].system_parameter.system_voltage, byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            header_byte_stream[stream_count] = byte_buffer[k];
            stream_count++;
        }
        float_to_byte_array(test_header[n].system_parameter.sigma_system_voltage, byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            header_byte_stream[stream_count] = byte_buffer[k];
            stream_count++;
        }

        //Write time stamp to stream
        int16_t_to_byte_array(test_header[n].time_stamp.year, byte_buffer);
        header_byte_stream[stream_count] = byte_buffer[0];
        stream_count++;
        header_byte_stream[stream_count] = byte_buffer[1];
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.day;
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.dotw;
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.month;
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.hour;
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.min;
        stream_count++;
        header_byte_stream[stream_count] = test_header[n].time_stamp.sec;
        stream_count++;

        //Write type to stream
        header_byte_stream[stream_count] = (uint8_t) test_header[n].type;
        stream_count++;

    }

    //Write stream to flash
    return_value = write_bytes_to_flash(flash_read_address, header_byte_stream, stream_count);

    if(return_value > 0) {
        return (int) stream_count;
    }

    return return_value;

}//end save_test_header_to_flash
int read_test_header_from_flash(Test_Header_t *test_header, uint8_t num_of_headers, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash) {

    //Array of bytes to store to flash
    uint8_t header_byte_stream[sizeof(Test_Header_t)*3];
    uint16_t stream_count = 0;
    int return_value = 0;
    uint8_t byte_buffer[4];

    //Read stream from flash
    return_value = read_bytes_from_flash(flash_read_address, number_of_bytes_written_to_flash, header_byte_stream);
    
    if(return_value < 0) {
        return return_value;
    }
    for(uint8_t n = 0; n < num_of_headers; n++) {

        //Read system parameters from stream
        for(uint8_t k = 0; k < 4; k++) {
            byte_buffer[k]  = header_byte_stream[stream_count];  
            stream_count++;
        }
        test_header[n].system_parameter.system_clock_frequency = byte_array_to_float(byte_buffer);

        for(uint8_t k = 0; k < 4; k++) {
            byte_buffer[k]  = header_byte_stream[stream_count];  
            stream_count++;
        }
        test_header[n].system_parameter.system_temperature = byte_array_to_float(byte_buffer);

        for(uint8_t k = 0; k < 4; k++) {
            byte_buffer[k] = header_byte_stream[stream_count];  
            stream_count++;
        }
        test_header[n].system_parameter.sigma_system_temperature = byte_array_to_float(byte_buffer);

        for(uint8_t k = 0; k < 4; k++) {
            byte_buffer[k] = header_byte_stream[stream_count];  
            stream_count++;
        }
        test_header[n].system_parameter.system_voltage = byte_array_to_float(byte_buffer);
        for(uint8_t k = 0; k < 4; k++) {
            byte_buffer[k]  = header_byte_stream[stream_count];  
            stream_count++;
        }
        test_header[n].system_parameter.sigma_system_voltage = byte_array_to_float(byte_buffer);

        //Read time stamp from stream
        byte_buffer[0] = header_byte_stream[stream_count];
        stream_count++;
        byte_buffer[1] = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.year = byte_array_to_int16_t(byte_buffer);
        test_header[n].time_stamp.day = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.dotw = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.month = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.hour = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.min = header_byte_stream[stream_count];
        stream_count++;
        test_header[n].time_stamp.sec = header_byte_stream[stream_count];
        stream_count++;

        //Read type from stream
        test_header[n].type = (Test_Type_t)header_byte_stream[stream_count];
        stream_count++;
    }
    
    return 1;

}//end read_test_header_from_flash

//Function to set visual controller evaluation LEDs
void set_up_evaluation_leds_gpio(uint *gpio_array) {

    for(uint8_t k = 0; k < 3; k++) {
        gpio_init(gpio_array[k]);
        gpio_set_dir(gpio_array[k], true);
    }

}//end set_up_evaluation_led_gpio

void set_evaluation_leds(Program_State_t state, uint *gpio_array) {

    switch(state) {
        case INIT:  gpio_put(gpio_array[0], true);
                    gpio_put(gpio_array[1], false);
                    gpio_put(gpio_array[2], false);
                    break;
        case IN_MENUE:  gpio_put(gpio_array[0], false);
                        gpio_put(gpio_array[1], true);
                        gpio_put(gpio_array[2], false);
                        break;
        case TESTING:   gpio_put(gpio_array[0], false);
                        gpio_put(gpio_array[1], false);
                        gpio_put(gpio_array[2], true);  
                        break;
        default:    gpio_put(gpio_array[0], false);
                    gpio_put(gpio_array[1], false);
                    gpio_put(gpio_array[2], false);
                    break;
    }

}//end set_evaluation_leds

//Functions to save settings and test counter to flash
void store_settings_to_flash(Test_Suite_Settings_t settings) {
    uint8_t utility_user_section_byte_stream[15];
    uint8_t delay_buffer[2];

    //Read all the settings and test counter from flash
    flash_read_from_utility_user_space(utility_user_section_byte_stream, 15);
    
    //Convert settings to byte stream
    utility_user_section_byte_stream[2] = settings.num_of_automatic_tests;
    uint16_t_to_byte_array(settings.delay_between_automatic_tests_in_ms, delay_buffer);
    utility_user_section_byte_stream[3] = delay_buffer[0];
    utility_user_section_byte_stream[4] = delay_buffer[1];
    uint16_t_to_byte_array(settings.delay_between_single_tests_in_ms, delay_buffer);
    utility_user_section_byte_stream[5] = delay_buffer[0];
    utility_user_section_byte_stream[6] = delay_buffer[1];
    utility_user_section_byte_stream[7] = (uint8_t)settings.print_test_results_after_tests;
    utility_user_section_byte_stream[8] = (uint8_t)settings.save_test_results_to_flash_after_test;
    utility_user_section_byte_stream[9] = (uint8_t)settings.disable_uart_in_auto_mode;
    utility_user_section_byte_stream[10] = (uint8_t)settings.test_choosing.UART_enabled;
    utility_user_section_byte_stream[11] = (uint8_t)settings.test_choosing.SPI_enabled;
    utility_user_section_byte_stream[12] = (uint8_t)settings.test_choosing.ADC_enabled;
    utility_user_section_byte_stream[13] = 1; 

    //Write byte stream to utility user section
    flash_write_to_utility_user_space(utility_user_section_byte_stream, 15);
    
}//end store_settings_to_flash

void store_test_counter_to_flash(uint16_t test_counter) {
    uint8_t utility_user_section_byte_stream[15];
    uint8_t test_counter_buff[2];
    
    //Convert flash counter to byte array
    uint16_t_to_byte_array(test_counter, test_counter_buff);

    //Read settings and test counter from flash
    flash_read_from_utility_user_space(utility_user_section_byte_stream, 15);

    //Write test counter to byte stream
    utility_user_section_byte_stream[0] = test_counter_buff[0];
    utility_user_section_byte_stream[1] = test_counter_buff[1];
    utility_user_section_byte_stream[14] = 1;

    //Write byte stream to flash
    flash_write_to_utility_user_space(utility_user_section_byte_stream, 15);
    

}//end store_test_counter_to_flash

//Functions to get settings and test counter to flash
void get_settings_from_flash(Test_Suite_Settings_t *settings) {
    uint8_t utility_user_section_byte_stream[15];
    uint8_t delay_buffer[2];

    //Read all the settings and test counter from flash
    flash_read_from_utility_user_space(utility_user_section_byte_stream, 15);
    
    //Convert byte stream to settings
    settings->num_of_automatic_tests = utility_user_section_byte_stream[2];
    delay_buffer[0] = utility_user_section_byte_stream[3];
    delay_buffer[1] = utility_user_section_byte_stream[4];
    settings->delay_between_automatic_tests_in_ms = byte_array_to_uint16_t(delay_buffer);
    delay_buffer[0] = utility_user_section_byte_stream[5];
    delay_buffer[1] = utility_user_section_byte_stream[6];
    settings->delay_between_single_tests_in_ms = byte_array_to_uint16_t(delay_buffer);
    settings->print_test_results_after_tests = (bool)utility_user_section_byte_stream[7];
    settings->save_test_results_to_flash_after_test = (bool)utility_user_section_byte_stream[8];
    settings->disable_uart_in_auto_mode = (bool)utility_user_section_byte_stream[9];
    settings->test_choosing.UART_enabled = (bool)utility_user_section_byte_stream[10];
    settings->test_choosing.SPI_enabled = (bool)utility_user_section_byte_stream[11];
    settings->test_choosing.ADC_enabled = (bool)utility_user_section_byte_stream[12];
    
}//end get_settings_from_flash

uint16_t get_test_counter_from_flash(void) {
    uint8_t test_counter_buff[2];

    //Get test counter from flash
    flash_read_from_utility_user_space(test_counter_buff, 2);

    return byte_array_to_uint16_t(test_counter_buff);

}//end get_test_counter_from_flash

//Functions to check for test counter and settings
bool are_settings_stored(void) {
    uint8_t utility_user_section_byte_stream[15];
    flash_read_from_utility_user_space(utility_user_section_byte_stream, 15);

    if(utility_user_section_byte_stream[13] == 1) {
        return true;
    }

    return false;
}//end are_settings_stored

bool is_test_counter_stored(void) {
    uint8_t utility_user_section_byte_stream[15];
    flash_read_from_utility_user_space(utility_user_section_byte_stream, 15);

    if(utility_user_section_byte_stream[14] == 1) {
        return true;
    }

    return false;
}//end is_test_counter_stored

//end file test_suite.c
