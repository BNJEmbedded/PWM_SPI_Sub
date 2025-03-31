//File: pio.c
//Project: Pico_MRI_Test_M

/* Description:

*/

//Libraries:

//Corresponding header-file:
#include "pio.h"

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:

//Pico Hardware-Libraries:
#include "hardware/clocks.h"

//PIO header from pio assembly
#include "pio_pwm_out.pio.h"
#include "pio_square_wave_out.pio.h"


//Own Libraries:

//Preprocessor constants:

#define NUM_OF_PIO 2
#define NUM_OF_SM 4

//File global (static) typedefs:

typedef enum pio_sm_configuration_type_e {

    PWM,
    SQUAREWAVE,

}pio_sm_configuration_type_t;

typedef struct pio_sm_configuration_s {

    PIO pio;
    uint gpio_pin;
    bool pio_is_configured;
    bool pio_gpio_is_output;
    pio_sm_configuration_type_t type;


}pio_sm_configuration_t;

//File global (static) variables:
pio_sm_configuration_t configuration_array[NUM_OF_PIO][NUM_OF_SM];

//File global (static) functions

int get_pio_index_from_pio(PIO pio) {

    if(pio == pio0) {
        return 0;
    }
    else if(pio == pio1) {
        return 1;
    }
    
    return -1;

}//end get_pio_index_from_pio

bool get_configuration_status(PIO pio, uint sm) {
    int pio_index = get_pio_index_from_pio(pio);
    if(pio_index < 0 || sm >= NUM_OF_SM) {
        return false;
    }

    return configuration_array[pio_index][sm].pio_is_configured;

}//end get_configuration_status

//Function definition:

//PWM:

int configure_pio_pwm_output(PIO pio, uint sm, float pio_sm_clk_div, uint pwm_gpio_pin) {

    uint offset = 0;   
    int pio_index = 0;

    pio_index = get_pio_index_from_pio(pio);

    if(pio_index < 0 || sm >= NUM_OF_SM) {
        return -1; //Error: the given parameter do not represent an actual hardware
    }

    if(get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine already used for other functionality
    }

    if(pio_sm_is_claimed(pio, sm)) {
        return -1; //Error: state machine already claimed
    }

    //Claim state machine hardware
    pio_sm_claim(pio, sm);

    //Add program to state-machine and get offset
    offset = pio_add_program(pio, &pwm_out_program);
    //Init program: its the c program written inside the pio assembly file
    pwm_out_init(pio, sm, offset, pio_sm_clk_div, pwm_gpio_pin);

    //Set static pio and gpio_pin
    configuration_array[pio_index][sm].pio = pio;
    configuration_array[pio_index][sm].gpio_pin = pwm_gpio_pin;

    //Set flags
    configuration_array[pio_index][sm].pio_is_configured = true;
    configuration_array[pio_index][sm].pio_gpio_is_output = true;

    //Set type of configuration (the functionality of the state-machine)
    configuration_array[pio_index][sm].type = PWM;

    return 1;

}//end configure_pio_pwm_output

int set_pio_pwm_period(PIO pio, uint sm, uint32_t pwm_period) {

    int pio_index = get_pio_index_from_pio(pio);

    if(!get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine is not configured, or the given parameters do not represent an actual hardware
    }

    if(configuration_array[pio_index][sm].type != PWM || !configuration_array[pio_index][sm].pio_gpio_is_output) {
        return -1; //Error: state-machine is not configured as PWM ouput
    }

    //Disable state-machine
    pio_sm_set_enabled(pio, sm, false);
    //Push period
    pio_sm_put_blocking(pio, sm, pwm_period);
    //Immediately execute instruction: encoded_pull: (false, false) <-> pull if not empty, pull no blocking - the state machine pulls the duty cycle from FIFO-TX
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    //Immediately execute instruction: Sets all 32 bit in the instruction register (period)
    pio_sm_exec(pio, sm, pio_encode_out(pio_isr, 32));
    //Enable state-machine
    pio_sm_set_enabled(pio, sm, true);

    return 1;

}//end set_pio_pwm_period

int set_pio_pwm_lvl(PIO pio, uint sm, uint32_t pwm_lvl) {

    int pio_index = get_pio_index_from_pio(pio);

    if(!get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine is not configured, or the given parameters do not represent an actual hardware
    }

    if(configuration_array[pio_index][sm].type != PWM || !configuration_array[pio_index][sm].pio_gpio_is_output) {
        return -1; //Error: state-machine is not configured as PWM ouput
    }
    pio_sm_put_blocking(pio, sm, pwm_lvl);
    return 1;

}//end set_new_pio_pwm_lvl

//Square-wave:
//TODO: Maybe check for max frequency
int configure_pio_square_wave_output(PIO pio, uint sm, float frequency, uint square_wave_gpio_pin) {

    float clk_div = 0;
    uint offset = 0;   
    int pio_index = 0;

    pio_index = get_pio_index_from_pio(pio);

    if(pio_index < 0 || sm >= NUM_OF_SM) {
        return -1; //Error: the given parameter do not represent an actual hardware
    }

    if(get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine already used for other functionality
    }

    if(pio_sm_is_claimed(pio, sm)) {
        return -1; //Error: state machine already claimed
    }

    //Claim state machine hardware
    pio_sm_claim(pio, sm);

    //Get clock divider the frequency gets actually doubled as the program uses two instructions
    clk_div = (float)(clock_get_hz(clk_sys)/(2*frequency));

    //Add program and get offset
    offset = pio_add_program(pio, &squarewave_out_program);
    //Init program
    square_wave_out_init(pio, sm, offset, clk_div, square_wave_gpio_pin);
    
    //Set static pio and gpio_pin
    configuration_array[pio_index][sm].gpio_pin = square_wave_gpio_pin;
    configuration_array[pio_index][sm].pio = pio;

    //Set flags
    configuration_array[pio_index][sm].pio_gpio_is_output = true;
    configuration_array[pio_index][sm].pio_is_configured = true;

    //Set type of configuration (the functionality of the state-machine)
    configuration_array[pio_index][sm].type = SQUAREWAVE;

    return 1;

}//end int configure_pio_square_wave_output

int set_pio_square_wave_frequency(PIO pio, uint sm, float frequency) {

    int return_value = 0;
    int pio_index = 0;
    float clk_div = 0;

    //Stop state-machine
    return_value = start_pio_sm(pio, sm, false);

    if(return_value < 0) {
        return return_value; //Error: state-machine is not configured or parameters given do not represent actual hardware
    }

    pio_index = get_pio_index_from_pio(pio);

    if(configuration_array[pio_index][sm].type != SQUAREWAVE || !configuration_array[pio_index][sm].pio_gpio_is_output) {
        return -1; //Error: state-machine is not configured as square wave output
    }

    //Get clock divider - the given frequency is actually doubled because the pio program uses two instructions so it uses two cycles
    clk_div = clock_get_hz(clk_sys)/(2*frequency);
    pio_sm_set_clkdiv(pio, sm, clk_div);

    start_pio_sm(pio, sm, true);

    return 1;

}//end set_pio_square_wave_frequency

//Start stop pio state-machine:

int start_pio_sm(PIO pio, uint sm, bool new_pio_sm_status) {

    int pio_index = get_pio_index_from_pio(pio);

    if(!get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine not configured, or the given parameters do not represent an actual hardware
    }

    pio_sm_set_enabled(pio, sm, new_pio_sm_status);
    return 1;

}//end start_pio_sm

//De-configuration:

int deconfigure_pio_sm(PIO pio, uint sm) {

    int pio_index = 0;

    pio_index = get_pio_index_from_pio(pio);

    if(!get_configuration_status(pio, sm)) {
        return -1; //Error: state-machine is not configured, or the given parameters do not represent an actual hardware, no need to de-configure
    }

    //Disable pio state-machine
    pio_sm_set_enabled(pio, sm, false);

    //Deinit pin
    gpio_deinit(configuration_array[pio_index][sm].gpio_pin);

    //Unclaim hardware
    pio_sm_unclaim(pio, sm);

    //Mark in configuration array that this pio-state-machine is not in use and can be used for other functionality
    configuration_array[pio_index][sm].pio = 0;
    configuration_array[pio_index][sm].pio_gpio_is_output = false;
    configuration_array[pio_index][sm].pio_is_configured = false;

    return 1;

}//end deconfigure_pio_sm

//end file pio.c
