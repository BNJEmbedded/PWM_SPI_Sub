//File: pwm.h
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

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:

//Own Libraries:

//Preprocessor constants:

//Type definitions:
typedef struct PWM_Instance_s {

    bool pwm_is_configured;
    bool pwm_is_output;
    bool is_dac_output;
    uint gpio_pin;
    uint pwm_slice;
    uint8_t pwm_channel;
    uint16_t pwm_wrap;
    uint16_t pwm_lvl;

}PWM_Instance_t;

//Function prototypes:

/**
 * @brief Configures PWM output.
 *
 * This function configures the specified GPIO pin as a PWM output with the given parameters.
 *
 * @param pwm_gpio_pin The GPIO pin to configure as PWM output.
 * @param pwm_clk_frequency The desired PWM clock frequency.
 * @param pwm_frequency The desired PWM frequency.
 * @param duty_cycle The desired duty cycle of the PWM signal.
 *
 * @return Returns the configured PWM frequency on successful configuration, -1 if the PWM channel is already in use.
 */
float configure_pwm_output(uint pwm_gpio_pin, float pwm_clk_frequency, float pwm_frequency, float duty_cycle);

/**
 * @brief Configures PWM as a DAC (Digital-to-Analog Converter) output.
 *
 * This function configures the specified GPIO pin as a PWM output and sets it up to function as a DAC output.
 *
 * @param dac_gpio_pin The GPIO pin to configure as DAC output.
 * @param pwm_clk_frequency The desired PWM clock frequency.
 * @param pwm_frequency The desired PWM frequency.
 * @param dac_resolution The resolution of the DAC.
 * @param ramp_up_down Whether the DAC output should ramp up and down between values (true) or hold values until changed (false).
 *
 * @return Returns 1 on successful configuration, -1 if the PWM channel is already in use, -2 if another DAC instance is already configured.
 */
/*NOTE: 
    To use this driver the right way one needs to add a LPF (low-pass-filter) at the output of the corresponding pin - this driver was tested with a passiv LPF, with order 0.
    The time constant of the filter needs to be bigger than the pwm frequency - for more details there are good blog posts on the internet.
*/
int configure_pwm_DAC(uint dac_gpio_pin, float pwm_clk_frequency, float pwm_frequency, uint8_t dac_resolution, bool ramp_up_down);

/**
 * @brief Starts or stops PWM output on the specified GPIO pin.
 *
 * This function starts or stops PWM output on the specified GPIO pin based on the provided state.
 *
 * @param gpio_pin The GPIO pin associated with the PWM output.
 * @param new_pwm_state The new state of the PWM output (true for start, false for stop).
 *
 * @return Returns 1 on successful operation, -1 if the PWM is not configured on the specified GPIO pin.
 */
int start_stop_pwm(uint gpio_pin, bool new_pwm_state);

/**
 * @brief De-configures PWM output on the specified GPIO pin.
 *
 * This function de-configures PWM output on the specified GPIO pin by de-initializing the pin and clearing
 * the PWM configuration settings associated with it.
 *
 * @param pwm_gpio_pin The GPIO pin associated with the PWM output to de-configure.
 *
 * @return Returns 1 on successful de-configuration, -1 if the PWM channel is not used.
 */
int deconfigure_pwm(uint pwm_gpio_pin);
//end file pwm.h