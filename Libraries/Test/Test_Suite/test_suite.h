//File: test_suite.h
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

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:

//Own Libraries:
#include "uart_test.h"
#include "spi_test.h"
#include "adc_test.h"

//Preprocessor constants:

#define NUM_OF_SAMPLES_SYSTEM_PARAMETER 100
#define MAX_NUMBER_OF_AUTOMATIC_TESTS 180
#define MAX_DELAY_BETWEEN_TESTS_IN_MS 10*1000

//Type definitions:

typedef enum Test_Type_e {
    UART,
    SPI,
    ADC,
}Test_Type_t;

//Defines commands received from user in test suite
typedef enum Menu_Command_e {
    NOP = 0,
    //Automatic test goes through the queue
    LAUNCH_AUTOMATIC_TEST,
    //UART test
    LAUNCH_UART_TEST,
    //SPI test
    LAUNCH_SPI_TEST,
    //ADC test
    LAUNCH_ADC_TEST,
    //Save the last test results to flash
    DELETE_TEST_RESULTS_FROM_FLASH,
    //Load test results from flash
    LOAD_TEST_RESULTS_FROM_FLASH,
    //Get system parameter: temperature, system-voltage and system clock frequency
    GET_SYSTEM_PARAMETER,
    //Exit test suite, the controller will be in endless loop after exiting
    RESET_TEST_SUITE,
    //Opens the sub menu to change test settings
    CHANGE_SETTINGS,
    //Print current settings
    GET_SETTINGS,

    //TODO:
    
    LAUNCH_SUITE_FROM_OTHER_CORE,
    NOT_DEFINED_COMMAND,

}Menu_Command_t;

typedef enum Settings_Menu_Command_e {

    NONE = 0,
    CHANGE_TIME,
    CHANGE_NUMBER_OF_TESTS,
    CHANGE_DELAY,
    TOGGLE_WRITE_TO_FLASH,
    TOGGLE_PRINT_TO_UART,
    TOGGLE_UART_CONNECTION_DISABLE,
    TOGGLE_UART_TEST_IN_AUTOMATIC_TEST,
    TOGGLE_SPI_TEST_IN_AUTOMATIC_TEST,
    TOGGLE_ADC_TEST_IN_AUTOMATIC_TEST,
    EXIT_SETTINGS_MENUE,
    NOT_DEFINED_COMMAND_SM,

}Settings_Menu_Command_t;

//Defines the states the MCU is in at the moment. This to visually check if the controller is working from outside.
typedef enum Program_State_e {

    INIT = 0,
    IN_MENUE,
    TESTING,

}Program_State_t;

//Defines structure with the mean values of system voltage, system temperature and their corresponding standard deviation
typedef struct System_Parameters_s {

    float system_temperature;
    float sigma_system_temperature;
    float system_clock_frequency;
    float system_voltage;
    float sigma_system_voltage;

}System_Parameters_t;

//Defines structure with a time stamp, the test type, nr and the system parameters recorded before a test
typedef struct Test_Header_s {

    datetime_t time_stamp;
    Test_Type_t type;
    System_Parameters_t system_parameter;

}Test_Header_t;

//Defines structure that contains flags for setting certain hardware testing during automatic test
typedef struct Automatic_Test_HW_Test_Enable_s {

    bool UART_enabled;
    bool SPI_enabled;
    bool ADC_enabled;

}Automatic_Test_HW_Test_Enable_t;
//Defines setting of the test suite: Number of tests in automatic test, delay between single tests in automatic test, delay between automatic tests, 
//data printing and storing, uart connection to pc and enabling what hardware tests are launched during a automatic test.
typedef struct Test_Suite_Settings_s {

    uint8_t num_of_automatic_tests;
    uint16_t delay_between_single_tests_in_ms;
    uint16_t delay_between_automatic_tests_in_ms;
    bool print_test_results_after_tests;
    bool save_test_results_to_flash_after_test;
    bool disable_uart_in_auto_mode;
    Automatic_Test_HW_Test_Enable_t test_choosing;

}Test_Suite_Settings_t;

//Function Prototypes:

//Printing the menu over given uart instance
int test_suite_print_gui(uart_inst_t *uart_to_print, datetime_t *time, uint16_t num_of_tests_saved_to_flash);
//Change settings menu
int print_change_settings_menu(uart_inst_t *uart_to_print);

//Gets command from input string
Menu_Command_t get_command_from_string(char *command_string);
Settings_Menu_Command_t get_test_settings_menu_command_from_string(char *command_string);

//Get settings from user
int get_number_of_tests_from_user(Test_Suite_Settings_t *settings, uart_inst_t *user_uart);
int get_delay_between_test_from_user(Test_Suite_Settings_t *settings, uart_inst_t *user_uart);

//Printing settings
int print_settings(Test_Suite_Settings_t settings, uart_inst_t *uart_to_print);

//Get system parameter function
int get_system_parameters(System_Parameters_t *system_parameters);
int print_system_parameters(System_Parameters_t system_parameter, uart_inst_t *uart_to_print);

//Get RTC time and date from user, print rtc date time
int get_rtc_time_date_from_user(uart_inst_t *user_uart, datetime_t *date_time);
void print_date_time(uart_inst_t *uart_to_print, datetime_t *date_time);

//Get test header function
int get_test_header(Test_Header_t *test_header, Test_Type_t test_type, uint8_t number_of_tests_to_perform, datetime_t date_time);
int print_test_header(Test_Header_t test_header, uart_inst_t *uart_to_print);

//Printing test results from test returns with header
int print_uart_test_results(Test_Header_t header, Uart_Test_Return_t *test_return, uint8_t num_of_tests, Uart_Test_Output_Format_t output_format, uart_inst_t *uart_to_print);
int print_spi_test_results(Test_Header_t header, SPI_Test_Return_t *test_return, uint8_t num_of_tests, SPI_Test_Output_Format_t output_format, uart_inst_t *uart_to_print);
int print_adc_test_results(Test_Header_t header, ADC_Test_Return_t *test_return, uint8_t num_of_tests, ADC_Test_Output_Format_t output_format, uart_inst_t *uart_to_print);

//Flash test results functions:

//Saving uart test results to flash and load uart test results from flash
int save_uart_test_results_to_flash(Uart_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t *flash_read_address);
int read_uart_test_results_from_flash(Uart_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash);

int save_spi_test_results_to_flash(SPI_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t *flash_read_address);
int read_spi_test_results_from_flash(SPI_Test_Return_t *test_returns, uint8_t number_of_tests, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash);

int save_adc_test_results_to_flash(ADC_Test_Return_t *test_returns, uint32_t *flash_read_address);
int read_adc_test_results_from_flash(ADC_Test_Return_t *test_returns, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash);

int save_test_header_to_flash(Test_Header_t *test_header, uint8_t num_of_headers, uint32_t *flash_read_address);
int read_test_header_from_flash(Test_Header_t *test_header, uint8_t num_of_headers, uint32_t flash_read_address, uint16_t number_of_bytes_written_to_flash);

//Function to set visual controller evaluation LEDs
void set_up_evaluation_leds_gpio(uint *gpio_array);
void set_evaluation_leds(Program_State_t state, uint *gpio_array);

//Functions to save settings and test counter to flash
void store_settings_to_flash(Test_Suite_Settings_t settings);
void store_test_counter_to_flash(uint16_t test_counter);

//Functions to get settings and test counter to flash
void get_settings_from_flash(Test_Suite_Settings_t *settings);
uint16_t get_test_counter_from_flash(void);

//Functions to check for test counter and settings
bool are_settings_stored(void);
bool is_test_counter_stored(void);

//end file test_suit.h
