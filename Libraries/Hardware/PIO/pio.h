//File: pio.h
//Project: Pico_MRI_Test_M

/* Description:

*/

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:

#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:
#include "hardware/pio.h"

//Own Libraries:

//Preprocessor constants:

//Type definitions:

//Function Prototypes:

//PWM:

int configure_pio_pwm_output(PIO pio, uint sm, float pio_sm_clk_div, uint pwm_gpio_pin);

int set_pio_pwm_period(PIO pio, uint sm, uint32_t pwm_period);
int set_pio_pwm_lvl(PIO pio, uint sm, uint32_t pwm_lvl);

int configure_pio_pwm_intput(uint pwm_gpio_pin, float sample_frequency);


//Square-Wave:

int configure_pio_square_wave_output(PIO pio, uint sm, float frequency, uint square_wave_gpio_pin);

int set_pio_square_wave_frequency(PIO pio, uint sm, float frequency);

//Start stop pio state-machine:

int start_pio_sm(PIO pio, uint sm, bool new_pio_sm_status);

//De-configuration:

int deconfigure_pio_sm(PIO pio, uint sm);


//end file pio.h
