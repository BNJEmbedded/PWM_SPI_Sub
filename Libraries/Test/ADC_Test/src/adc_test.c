//File: adc_test.c
//Project: Pico_MRI_Test_M

/* Description:

    This is a handler for testing the adc-peripheral of RP2040 inside a MRI-Scanner.

        The test setup for ADC is:

        -Two Raspberry-Pi-Pico-Boards (Main and Sub)
        -Sub creates a ramp output via PWM-DAC (digital to analog converter) and a passive LPF (low pass filter) of the order one at the output.
        -Main measures this ramp with the adc.
        -Per ramp step (the number of steps depends on the resolution of the DAC) there are a configure-able amount of samples taken.
        -Out of these samples per step the mean and standard deviations are calculated
        -All steps are printed out as mean-value +/- standard-deviation or as raw digital values

*/

//Corresponding header-file:
#include "adc_test.h"

//Libraries:

//Standard-C:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h"

//Pico Hardware-Libraries:
#include "hardware/watchdog.h"

//Own Libraries:
#include "adc.h"
#include "uart.h"
#include "pwm.h"
#include "statistic.h"

//Preprocessor constants:

//File global (static) variables:

//Functions:

//File global (static) function definitions:

//Function definition:

int adc_test_main_read_ramp(ADC_Test_Structure_t adc_tests, ADC_Test_Return_t *return_of_test, bool use_watchdog) {

    uint32_t wait_between_samples_ms = 0;
    uint32_t wait_between_samples_us = 0;
    float precise_wait_between_samples_ms = 0;
    bool ms_waittime = true;

    #ifdef USE_OPTIMIZED_SETUP
        uint8_t adc_sample_buffer_byte[MAX_ADC_SAMPLES_PER_RAMP_STEP];
        float adc_sample_buffer[MAX_ADC_SAMPLES_PER_RAMP_STEP];
    #endif
   
    //Get precise wait time between samples in ms - one step takes 10ms and this program will sample number_of_samples_per_ramp_step times on every step
    //To get the waittime in ms it the step time is divided by the number of samples per step
    precise_wait_between_samples_ms = (10/adc_tests.parameter.number_of_samples_per_ramp_step);

    if(precise_wait_between_samples_ms < 1) {
        ms_waittime = false;
    }
    else {
        ms_waittime = true;
    }

    #ifdef USE_OPTIMIZED_SETUP
    if(ms_waittime) {
        wait_between_samples_ms = (uint32_t)(10/adc_tests.parameter.number_of_samples_per_ramp_step);
    }
    else {
        //Else get µs wait time
        wait_between_samples_us = (uint32_t)(10000/adc_tests.parameter.number_of_samples_per_ramp_step);
        //HACK: The wait time needs to be changed because calculating mean value and standard deviation every 500 samples also takes time..., getting all the samples is to big of a array
        wait_between_samples_us = wait_between_samples_us - 10;
    }
    #else
        wait_between_samples_ms = (10/adc_tests.parameter.number_of_samples_per_ramp_step);
    #endif
    
    //Configure UART for communication between main and sub
    configure_uart_hardware(adc_tests.hardware.uart_instance, adc_tests.hardware.uart_rx_pin, adc_tests.hardware.uart_tx_pin, 
    adc_tests.hardware.uart_baudrate, 8, 1, UART_PARITY_NONE, false, false, '\n');

    //Configure ADC for single channel multi sample conversion mode
    configure_adc_single_channel(adc_tests.hardware.adc_channel, adc_tests.parameter.adc_sample_rate, adc_tests.hardware.dma_adc_capture_channel,
    adc_tests.hardware.dma_adc_control_channel);

    uart_tx_data(adc_tests.hardware.uart_instance, "READY TO MEASURE!");

    //Wait till sub starts ramp and start sampling
    while(1) {
        if(uart_get_rx_complete_flag(adc_tests.hardware.uart_instance)) {

            if(use_watchdog) {
                watchdog_update();
            }
            break;
        }
        tight_loop_contents();
    }
    adc_start_measurement(true);

    #ifndef USE_OPTIMIZED_SETUP     

    //Get samples and store them directly in return structure
    for(uint32_t s = 0; s < adc_tests.parameter.number_of_samples_per_ramp*adc_tests.parameter.number_of_samples_per_ramp_step; s++) {
        adc_get_gpio_measurement(adc_tests.hardware.adc_gpio_pin, &return_of_test->adc_samples[s]);
        if(use_watchdog) {
            watchdog_update();
        }
        busy_wait_ms(wait_between_samples_ms); 
    }
    //HACK: Wait for another 20ms
    busy_wait_us(20*1000);
    #endif

    #ifdef USE_OPTIMIZED_SETUP
    for(uint32_t k = 0; k < adc_tests.parameter.number_of_samples_per_ramp; k++) {

        for(uint32_t s = 0; s < adc_tests.parameter.number_of_samples_per_ramp_step; s++) {
            //Get samples
            adc_get_gpio_measurement(adc_tests.hardware.adc_gpio_pin, &adc_sample_buffer_byte[s]);
            adc_sample_buffer[s] = ((float)adc_sample_buffer_byte[s]*3.261)/255;

            if(use_watchdog) {
                watchdog_update();
            }

            if(ms_waittime) {
                busy_wait_ms(wait_between_samples_ms);
            }
            else {
                busy_wait_us(wait_between_samples_us);
            }
             
        }
        //Calculate mean value and standard deviation;
        return_of_test->mean_value[k] = get_mean_value(adc_sample_buffer, MAX_ADC_SAMPLES_PER_RAMP_STEP);
        return_of_test->std_deviation[k] = get_std_deviation(adc_sample_buffer, MAX_ADC_SAMPLES_PER_RAMP_STEP);
    }
    //HACK: Wait for another 250ms
    busy_wait_us(250*1000);
   
    #endif

    //TX back to sub that measurement is finished
    uart_tx_data(adc_tests.hardware.uart_instance, "FINISHED_MEASUREMENT");
    adc_start_measurement(false);
    //Deconfigure ADC
    deconfigure_adc();
    //Deconfigure UART after test
    deconfigure_uart_hardware(adc_tests.hardware.uart_instance);

    return_of_test->adc_sample_rate = adc_tests.parameter.adc_sample_rate;
    return_of_test->number_of_samples = adc_tests.parameter.number_of_samples_per_ramp*adc_tests.parameter.number_of_samples_per_ramp_step;
    return_of_test->number_of_samples_per_ramp_step = adc_tests.parameter.number_of_samples_per_ramp_step;
    return_of_test->number_of_samples_per_ramp = adc_tests.parameter.number_of_samples_per_ramp;
    return_of_test->dac_pwm_frequency = adc_tests.parameter.dac_pwm_frequency;
    return_of_test->dac_resolution_in_bits = adc_tests.parameter.dac_resolution_in_bits;

    return 1;

}//end adc_test_main_read_ramp

void adc_test_sub_ramp_output(ADC_Test_Structure_t adc_tests, bool use_watchdog) {

    //TODO: Check for wrong parameters and discard test
    uint8_t uart_buff[MAX_UART_DATA_SIZE];
    uint8_t p = 0;
    volatile uint32_t watchdog_count = 0;

    //Configure UART for communication between main and sub
    configure_uart_hardware(adc_tests.hardware.uart_instance, adc_tests.hardware.uart_rx_pin, adc_tests.hardware.uart_tx_pin, 
    adc_tests.hardware.uart_baudrate, 8, 1, UART_PARITY_NONE, false, false, '\n');

    //Set ramp till main is done with measurement that
    configure_pwm_DAC(adc_tests.hardware.pwm_dac_pin, adc_tests.parameter.dac_pwm_clk_frequency, 
    adc_tests.parameter.dac_pwm_frequency, adc_tests.parameter.dac_resolution_in_bits, false);

    //Wait till main is ready
    while(1) {
        if(uart_get_rx_complete_flag(adc_tests.hardware.uart_instance)) {
            if(use_watchdog) {
                watchdog_update();
            }
            uart_get_rx_data(adc_tests.hardware.uart_instance, uart_buff);
            break;
        }
    }

    start_stop_pwm(adc_tests.hardware.pwm_dac_pin, true);
    uart_tx_data(adc_tests.hardware.uart_instance, "START_RAMP");

    /*NOTE:
        Wait for half the ramp time - note: this is a workaround for the watchdog timer hardware is not able to set times over 8.3s (caused by a hardware design error - counter is
        incremented two times every tick)
        Also its is not possible to get the watchdog count with the function watchdog_get_count() so it is not possible to get the current time left till reboot, with which it would
        be possible to update the watchdog for x-times expanding the watchdog time to x*watchdog_delay_time
    */
    if(use_watchdog) {
        watchdog_update();
        uint32_t delay_ms = (adc_tests.parameter.number_of_samples_per_ramp*10)/2;
        busy_wait_ms(delay_ms);
        watchdog_update();
    }
   
    //Wait till main is finished with measurement of waveform
    while(1) {
        if(uart_get_rx_complete_flag(adc_tests.hardware.uart_instance)) {
            if(use_watchdog) {
                watchdog_update();
            }
            break;
        }
        tight_loop_contents();
    }

    start_stop_pwm(adc_tests.hardware.pwm_dac_pin, false);
    deconfigure_pwm(adc_tests.hardware.pwm_dac_pin);
    //Deconfigure UART after test
    deconfigure_uart_hardware(adc_tests.hardware.uart_instance);

}//end adc_test_sub_ramp_output

void adc_test_print_test_returns(ADC_Test_Return_t *test_return, uart_inst_t *uart_to_print, ADC_Test_Output_Format_t format) {
    
    uint8_t uart_tx_string[MAX_UART_DATA_SIZE];
    uint8_t j = 0;

    uart_tx_data(uart_to_print, "#######################ADC-Test########################");
    uart_tx_data(uart_to_print, "");
    uart_tx_data(uart_to_print, "#################ADC-Test-Parameter:########################");
    sprintf(uart_tx_string, "DAC-Parameter: Resolution: %ld-Bit, PWM-Frequency: %f Hz, Hold-time: 10 ms", 
    test_return->dac_resolution_in_bits, test_return->dac_pwm_frequency);
    uart_tx_data(uart_to_print, uart_tx_string);
    clear_uart_buffer(uart_tx_string);
    sprintf(uart_tx_string, "ADC-Parameter: Resolution: 8-Bit, ADC-Sample-Rate: %f S/s, Samples-per-ramp-step: %ld, Samples-per-ramp: %ld", 
    test_return->adc_sample_rate, test_return->number_of_samples_per_ramp_step, test_return->number_of_samples_per_ramp);
    uart_tx_data(uart_to_print, uart_tx_string);
    clear_uart_buffer(uart_tx_string);
    uart_tx_data(uart_to_print, "");
    uart_tx_data(uart_to_print, "##############Start-ADC-measurement-file###############");
    uart_tx_data(uart_to_print, "");

    #ifdef USE_OPTIMIZED_SETUP
    float mean_value = 0;
    float std_deviation = 0;
    uint8_t m = 0;

    for(uint32_t k = 0; k < test_return->number_of_samples_per_ramp; k++) {

        //Print the values
        if(m < format.values_per_line) {
            sprintf(uart_tx_string, "%f +/- %f%c", test_return->mean_value[k], 2*(test_return->std_deviation[k]), format.value_separator);
            uart_tx_data_unterminated(uart_to_print, uart_tx_string);
            clear_uart_buffer(uart_tx_string);
            m++;
        }
        else {
            uart_tx_data(uart_to_print, "");
            sprintf(uart_tx_string, "%f +/- %f%c", test_return->mean_value[k], 2*(test_return->std_deviation[k]), format.value_separator);
            uart_tx_data_unterminated(uart_to_print, uart_tx_string);
            clear_uart_buffer(uart_tx_string);
            m = 1;
        }

    }
                
    #endif

    #ifndef USE_OPTIMIZED_SETUP
    if(format.print_raw_digital_values == true) {

        for(uint32_t k = 0; k < test_return->number_of_samples; k++) {
            if(j < format.values_per_line) {  
                sprintf(uart_tx_string, "%ld%c", test_return->adc_samples[k], format.value_separator);
                uart_tx_data_unterminated(uart_to_print, uart_tx_string);
                clear_uart_buffer(uart_tx_string);
                j++;
            }
            else {
                uart_tx_data(uart_to_print, "");
                sprintf(uart_tx_string, "%ld%c", test_return->adc_samples[k], format.value_separator);
                uart_tx_data_unterminated(uart_to_print, uart_tx_string);
                clear_uart_buffer(uart_tx_string);
                j = 0;
            } 
        }

    }
    else {

        float voltage = 0;
        float voltage_samples_per_step[MAX_ADC_SAMPLES_PER_RAMP_STEP];
        float mean_value = 0;
        float std_deviation = 0;
        uint8_t m = 0;
        
        for(uint32_t k = 0; k < test_return->number_of_samples; k++) {
            //Calculate voltage for every number of samples per ramp step and save them to buffer
            if(j < test_return->number_of_samples_per_ramp_step) {
                voltage = ((float)test_return->adc_samples[k]*3.261)/255;
                voltage_samples_per_step[j] = voltage;
                j++;
            }
            else {
                
                //Get mean and standard deviation for every ramp step
                mean_value = get_mean_value(voltage_samples_per_step, test_return->number_of_samples_per_ramp_step);
                std_deviation = get_std_deviation(voltage_samples_per_step, test_return->number_of_samples_per_ramp_step);

                //Print the values
                if(m < format.values_per_line) {
                    sprintf(uart_tx_string, "%f +/- %f%c", mean_value, 2*std_deviation, format.value_separator);
                    uart_tx_data_unterminated(uart_to_print, uart_tx_string);
                    clear_uart_buffer(uart_tx_string);
                    m++;
                }
                else {
                    uart_tx_data(uart_to_print, "");
                    sprintf(uart_tx_string, "%f +/- %f%c", mean_value, 2*std_deviation, format.value_separator);
                    uart_tx_data_unterminated(uart_to_print, uart_tx_string);
                    clear_uart_buffer(uart_tx_string);
                    m = 1;
                }

                //Get  voltage from next sample
                j = 0;
                voltage = ((float)test_return->adc_samples[k]*3.261)/255;
                voltage_samples_per_step[j] = voltage;
                j++;
            }
        }
    }
    #endif
    
    uart_tx_data(uart_to_print, "");
    uart_tx_data(uart_to_print, "##############End-ADC-measurement-file#################");
    uart_tx_data(uart_to_print, "");
    

}//end adc_test_print_test_returns

//end file adc_test.c