//File: pwm.c
//Project: Pico_MRI_Test_M

/* Description:
    Custom pwm-wrapper around the Raspberry-Pi-Pico-SDK (hardware/pwm.h). 
    Let user configure pwm output channels with given frequency and duty cycle.
    Let user configure pwm output channel for a PWM-DAC. User could set how many bits the DAC resolution is, if the DAC should ramp up and down or only up.
    NOTE: This module is not multi-core-save

    FUTURE_FEATURE: Make this module multi-core-save
    FUTURE_FEATURE: PWM input - lets user measure frequency/duty-cycle of PWM input
    FUTURE_FEATURE: Let user control DAC hold time (at the moment this is fixed)
*/

//Corresponding header-file:
#include "pwm.h"

//Libraries:

//Standard-C:
#include <math.h>

//Pico:

//Pico High-LvL-Libraries:

//Pico Hardware-Libraries:
#include "hardware/pwm.h"
#include "hardware/clocks.h"

//Own Libraries:

//Preprocessor constants:
#define MAX_PWM_CHANNELS 16

//File global (static) variables:

//Array with pwm_instances (pwm-channels)
static PWM_Instance_t pwm_instances[MAX_PWM_CHANNELS];

//PWM-DAC-Instance:
static uint8_t dac_pwm_instance_index = 0;

//Ramp up and down
static bool dac_instance_up_and_down_counting = false;
static volatile bool dac_is_ramping_up = false;

//Resolution:
static uint8_t dac_instance_resolution = 8; //Standard value is 8-Bit-Resolution
static uint16_t dac_instance_resolution_as_integer = 256; //Standard value is 8-Bit-Resolution <-> 256 as integer
static uint16_t dac_resolution_counter = 0; //Gives the number of parts of the duty-cycle 

//Level and Wrap
static uint16_t dac_pwm_wrap = 0;
static uint16_t dac_pwm_lvl = 0;

//Hold time:
static uint32_t dac_cycle_counter = 1; //Counts how many pwm-cycles the duty-cycle is hold
static const float dac_hold_time = 10e-3; //Hold the signal for 10ms
static uint32_t dac_hold_time_in_cycles = 0; //This value will be calculated in number in cycle for a time span of 10ms

//Function definition:

//File global (static) function definition:

static uint8_t get_array_index(uint gpio_pin) {
    //Write configuration to static variable
    if(gpio_pin < 16) {
        return gpio_pin;
    }
    else {
        return gpio_pin-16;
    }
}//end get_array_index

static bool is_slice_channel_used(uint gpio_pin) {

   uint8_t array_index = 0;
   array_index = get_array_index(gpio_pin);
   return pwm_instances[array_index].pwm_is_configured;  

}//end is_slice_channel_used

static bool dac_already_used(void) {
    for(uint8_t k = 0; k < MAX_PWM_CHANNELS; k++) {
        if(pwm_instances[k].is_dac_output) {
            return true;
        }
    }

    return false;
}

static void pwm_wrap_interrupt_handler(void) {

    pwm_clear_irq(pwm_instances[dac_pwm_instance_index].pwm_slice);

    //The MCU will hold the duty-cycle till x-amount of cycles are passed (step length), if this number is reached duty-cycle will be changed
    if(dac_instance_up_and_down_counting == false) {
        if(dac_cycle_counter < dac_hold_time_in_cycles) {
            dac_cycle_counter++;
        }
        else {
            if(dac_resolution_counter < dac_instance_resolution_as_integer) {
                dac_resolution_counter++;
            }
            else {      
                dac_resolution_counter = 0; 
            }

            //Set new duty-cycle 
            dac_pwm_lvl = dac_resolution_counter*(dac_pwm_wrap/dac_instance_resolution_as_integer);
            pwm_set_gpio_level(pwm_instances[dac_pwm_instance_index].gpio_pin, dac_pwm_lvl);
            dac_cycle_counter = 1;  
        }
    }
    else {
        if(dac_cycle_counter < dac_hold_time_in_cycles) {
            dac_cycle_counter++;
        }
        else {
            if(dac_is_ramping_up == true) {

                if(dac_resolution_counter < dac_instance_resolution_as_integer) {
                    dac_resolution_counter++;
                }
                else if(dac_resolution_counter == 0) {
                    dac_is_ramping_up = true;
                }
                else if(dac_resolution_counter == dac_instance_resolution_as_integer) {
                    dac_is_ramping_up = false;

                }
            }
            else {
                if(dac_resolution_counter > 0) {
                    dac_resolution_counter--;
                }
                else {
                    dac_is_ramping_up = true;
                }
            }

            //Set new duty-cycle 
            dac_pwm_lvl = dac_resolution_counter*(dac_pwm_wrap/dac_instance_resolution_as_integer);
            pwm_set_gpio_level(pwm_instances[dac_pwm_instance_index].gpio_pin, dac_pwm_lvl);
            dac_cycle_counter = 1;
        }

    }
    

}//end pwm_wrap_interrupt_handler

//Function definition:

float configure_pwm_output(uint pwm_gpio_pin, float pwm_clk_frequency, float pwm_frequency, float duty_cycle) {

    float pwm_clk_div = 0;
    uint8_t pwm_slice = 0;
    uint8_t pwm_channel = 0;
    uint16_t pwm_lvl = 0;
    uint16_t pwm_wrap = 0;
    uint8_t array_index = 0;

    if(is_slice_channel_used(pwm_gpio_pin)) {
        return -1; //Error pwm channel is already used
    }

    //Init gpio
    gpio_init(pwm_gpio_pin);
    gpio_set_function(pwm_gpio_pin, GPIO_FUNC_PWM);

    //Get pwm slice from gpio
    pwm_slice = pwm_gpio_to_slice_num(pwm_gpio_pin);
    //Get pwm channel
    pwm_channel = pwm_gpio_to_channel(pwm_gpio_pin);

    //Get prescaler value
    pwm_clk_div = (float)(clock_get_hz(clk_sys)/pwm_clk_frequency);
    //Set pwm prescaler
    pwm_set_clkdiv(pwm_slice, pwm_clk_div);

    //If the counter reaches pwm_wrap the output toggles  period
    pwm_wrap = (pwm_clk_frequency/pwm_frequency);
    pwm_set_wrap(pwm_slice, pwm_wrap);

    //If the counter reaches pwm_lvl the output switches from high to low - duty-cycle
    pwm_lvl = (uint16_t)(pwm_wrap*duty_cycle);
    pwm_set_chan_level(pwm_slice, pwm_channel, pwm_lvl);

    //Write configuration to static variable
    array_index = get_array_index(pwm_gpio_pin);

    pwm_instances[array_index].gpio_pin = pwm_gpio_pin;
    pwm_instances[array_index].pwm_channel = pwm_channel;
    pwm_instances[array_index].pwm_slice = pwm_slice;
    pwm_instances[array_index].pwm_wrap = pwm_wrap;
    pwm_instances[array_index].pwm_lvl = pwm_lvl;
    pwm_instances[array_index].pwm_is_output = true;
    pwm_instances[array_index].pwm_is_configured = true;
    pwm_instances[array_index].is_dac_output = false;

    return pwm_frequency;

}//end configure_pwm_output

int configure_pwm_DAC(uint dac_gpio_pin, float pwm_clk_frequency, float pwm_frequency, uint8_t dac_resolution, bool ramp_up_down) {

    float dac_pwm_frequency = 0;

    if(dac_already_used()) {
        return -2; //Only one DAC instance allowed
    }

    if(is_slice_channel_used(dac_gpio_pin)) {
        return -1; //Error pwm is already configured
    }

    //Configure pwm as output
    dac_pwm_frequency = configure_pwm_output(dac_gpio_pin, pwm_clk_frequency, pwm_frequency, 0);
    if(dac_pwm_frequency < 0) {
        return dac_pwm_frequency; //Error in configuration of pwm-instance
    }
    //Calculate the hold time in pwm cycles
    dac_hold_time_in_cycles = (uint32_t)(pwm_frequency*dac_hold_time); 

    //Get array index of pwm-instance of pwm for dac
    dac_pwm_instance_index = get_array_index(dac_gpio_pin);

    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_wrap_interrupt_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_clear_irq(pwm_instances[dac_pwm_instance_index].pwm_slice);
    pwm_set_irq_enabled(pwm_instances[dac_pwm_instance_index].pwm_slice, true);
    
    pwm_instances[dac_pwm_instance_index].is_dac_output = true;
    dac_pwm_wrap = pwm_instances[dac_pwm_instance_index].pwm_wrap;
    dac_pwm_lvl = pwm_instances[dac_pwm_instance_index].pwm_lvl;
    dac_instance_resolution = dac_resolution;
    dac_instance_resolution_as_integer = (uint32_t) (pow(2, dac_resolution));
    dac_instance_up_and_down_counting = ramp_up_down;
    dac_cycle_counter = 1;
    dac_resolution_counter = 0;

    return 1; //Return no error
    
}//end configure_pwm_DAC

int start_stop_pwm(uint gpio_pin, bool new_pwm_state) {

    uint8_t array_index = 0;
    if(is_slice_channel_used(gpio_pin) == false) {
        return -1; //Error PWM is not configured 
    }
    array_index = get_array_index(gpio_pin);
    pwm_set_enabled(pwm_instances[array_index].pwm_slice, new_pwm_state);

    //If pwm instance is stopped and used pwm instance is dac output clear interrupt, reset cycle counter, resolution counter and lvl.
    if(pwm_instances[array_index].is_dac_output && new_pwm_state == false) {
        pwm_clear_irq(pwm_instances[array_index].pwm_slice);
        dac_cycle_counter = 1;
        dac_resolution_counter = 0;
        dac_pwm_lvl = 0;
    }
    return 1;

}//end start_stop_pwm

int deconfigure_pwm(uint pwm_gpio_pin) {

    uint8_t array_index = 0;

    if(is_slice_channel_used(pwm_gpio_pin) == false) {
        return -1; //Error pwm channel is not used
    }

    //Deinit pin
    gpio_deinit(pwm_gpio_pin);

    array_index = get_array_index(pwm_gpio_pin);
    pwm_instances[array_index].pwm_is_configured = false;
    pwm_instances[array_index].is_dac_output = false;

    if(pwm_instances[array_index].is_dac_output) {
        dac_pwm_instance_index = 0;
        pwm_clear_irq(pwm_instances[array_index].pwm_slice);
        pwm_set_irq_enabled(pwm_instances[array_index].pwm_slice, false);
        irq_set_enabled(PWM_IRQ_WRAP, false);
        irq_remove_handler(PWM_IRQ_WRAP, pwm_wrap_interrupt_handler);
        pwm_instances[array_index].is_dac_output = false;
        dac_cycle_counter = 1;
        dac_resolution_counter = 0;
        dac_hold_time_in_cycles = 0;
        dac_pwm_wrap = 0;
        dac_pwm_lvl = 0;
    }

    return 1;

}//end deconfigure_pwm

//end file pwm.c
