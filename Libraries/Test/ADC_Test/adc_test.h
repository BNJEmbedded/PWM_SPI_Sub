//File: adc_test.h
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

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:

//Own Libraries:

//NOTE: Control preprocessor to control which ADC setup is build and used
//#define USE_OPTIMIZED_SETUP true

//Preprocessor constants - if optimized setup is used more samples per ramp step are taken:
#ifdef USE_OPTIMIZED_SETUP
    #define MAX_ADC_SAMPLES_PER_RAMP 1024
    #define MAX_ADC_SAMPLES_PER_RAMP_STEP 500
#else
    #define MAX_ADC_SAMPLES_PER_RAMP 1024
    #define MAX_ADC_SAMPLES_PER_RAMP_STEP 5
#endif

//Type definitions:

typedef struct ADC_Test_Hardware_s {

    //ADC
    //GPIO pin and adc channel
    uint adc_gpio_pin;
    uint adc_channel;
    //DMA for ADC
    uint dma_adc_capture_channel;
    uint dma_adc_control_channel;
    //PWM-DAC
    uint pwm_dac_pin;
    //UART Instance for synchronisation
    uart_inst_t *uart_instance;
    uint uart_tx_pin;
    uint uart_rx_pin;
    uint uart_baudrate;

}ADC_Test_Hardware_t;

typedef struct ADC_Test_Parameter_s {

    //Parameters of the test

    //ADC
    //ADC sample rate
    float adc_sample_rate;
    //ADC number of samples per ramp  and per step of the ramp
    uint32_t number_of_samples_per_ramp_step;
    uint32_t number_of_samples_per_ramp;
    //PWM-DAC
    //Frequency of the pwm clk
    float dac_pwm_clk_frequency;
    //Frequency of the pwm signal
    float dac_pwm_frequency;
    //Resolution of the output ramp
    uint8_t dac_resolution_in_bits;


}ADC_Test_Parameter_t;

typedef struct ADC_Test_Structure_s {

    ADC_Test_Hardware_t hardware;
    ADC_Test_Parameter_t parameter;

}ADC_Test_Structure_t;

#ifdef USE_OPTIMIZED_SETUP
//New ADC test structure that stores only mean value and standard deviation and fft:
typedef struct ADC_Test_Return_s {
    float adc_sample_rate;
    uint32_t number_of_samples;

    uint8_t dac_resolution_in_bits;
    float dac_pwm_frequency;

    uint32_t number_of_samples_per_ramp_step;
    uint32_t number_of_samples_per_ramp;

    //Store mean_value, std_deviation 
    float mean_value[MAX_ADC_SAMPLES_PER_RAMP];
    float std_deviation[MAX_ADC_SAMPLES_PER_RAMP];

}ADC_Test_Return_t;
#endif

#ifndef USE_OPTIMIZED_SETUP
typedef struct ADC_Test_Return_s {

    float adc_sample_rate;
    uint32_t number_of_samples;

    uint8_t dac_resolution_in_bits;
    float dac_pwm_frequency;

    uint32_t number_of_samples_per_ramp_step;
    uint32_t number_of_samples_per_ramp;

    uint8_t adc_samples[MAX_ADC_SAMPLES_PER_RAMP*MAX_ADC_SAMPLES_PER_RAMP_STEP];

}ADC_Test_Return_t;
#endif

typedef struct ADC_Test_Output_Format_s {

    uint8_t values_per_line;
    uint8_t value_separator;
    bool print_raw_digital_values;

}ADC_Test_Output_Format_t;

//Function Prototypes:

int adc_test_main_read_ramp(ADC_Test_Structure_t adc_tests, ADC_Test_Return_t *return_of_test, bool use_watchdog);
void adc_test_sub_ramp_output(ADC_Test_Structure_t adc_tests, bool use_watchdog);
void adc_test_print_test_returns(ADC_Test_Return_t *test_return, uart_inst_t *uart_to_print, ADC_Test_Output_Format_t format);

//end file adc_test.h
