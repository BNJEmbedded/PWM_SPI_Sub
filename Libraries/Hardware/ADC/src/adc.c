//File: adc.c
//Project: Pico_MRI_Test_M

/* Description:

    Custom adc-wrapper around the Raspberry-Pi-Pico-SDK. Lets user configure single channel and multi channel measurement with RP-Pi-Pico ADC.
    This module uses DMA-Channels to get the data from ADC to RAM.

    NOTE: This module is not multi-core-save
    NOTE: Except the temperature sensor all measurements are with fixed 8-Bit resolution, as the ENOB of the ADC is 8.9 (see RP2040 Data-sheet)
    NOTE: Multichannel function was not properly tested - as it may needs changes
    NOTE: Temperature Sensor in multi-channel measurement makes no sense as in multi-channel mode the resolution is set to 8-Bit, the temperature sensor needs a resolution
          of 12-Bits (because the change in voltage with temperature is to small to measure properly with 8-Bit)

    //TODO: Test Multi-Channel measurement

    FUTURE_FEATURE: New and improved multichannel functions
    FUTURE_FEATURE: New and improved temperature measurement
    FUTURE_FEATURE: Multi-core

*/


//Corresponding header-file:
#include "adc.h"

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:

//Pico Hardware-Libraries:
#include "hardware/adc.h"
#include "hardware/dma.h"

//Own Libraries:
#include "uart.h"

//Preprocessor constants:
#define MAX_ADC_CHANNELS 5
#define MAX_ADC_SAMPLES 10240

//File global (static) variables:

//Data:

//Multi-channel-mode data and count of channels
static uint8_t adc_number_of_channels = 0;
//Data of each channel in single or multi-channel mode when using all channels
static uint8_t adc_data[MAX_ADC_CHANNELS];

//Data if using multi-channel-mode without using all channels: 
static uint8_t channel_list[MAX_ADC_CHANNELS];
static uint8_t adc_data_2_channels[2];
static uint8_t adc_data_3_channels[3];
static uint8_t adc_data_4_channels[4];

//Start address for DMA
static uint8_t* start_address[1] = {NULL};

//DMA Channels
static uint dma_adc_capture_channel = 0;
static uint dma_adc_control_channel = 0;

//Configuration:

//ADC mode (singe or multi channel)
static bool adc_channel_is_configured[MAX_ADC_CHANNELS];
//Flag of each channel wich indicates if the channel is configured
static bool adc_mode_single_channel = false;

//Temperature sensor flag
static bool temperature_sensor_is_configured = false;

//Function definition:

//File global (static) function definitions:
static int8_t get_channel(uint adc_gpio_pin) {
    switch(adc_gpio_pin) {
        case 26: return 0;
        case 27: return 1;
        case 28: return 2;
        case 29: return 3;
        default: return -1; //Error other gpio pins could not be used as adc pins
    }
}//end get_channel

static int8_t get_gpio_from_channel(uint adc_channel) {
    switch(adc_channel) {
        case 0: return 26;
        case 1: return 27;
        case 2: return 28;
        case 3: return 29;
        default: return -1; //Error other adc channels are not connected to gpio pins
    }
}//end get_gpio_from_channel

//Function definitions:

//Configuration:
int configure_adc_single_channel(uint adc_channel, float adc_sample_rate, uint dma_capture_channel, uint dma_control_channel) {

    float adc_clk_div = 0;
    int8_t adc_gpio_pin = 0;
    int channel_mask = 0;

    if(adc_channel > 4) {
        return -1; //Error wrong parameter, there are only 5 ADC-channels
    }
    channel_mask += 1 << adc_channel;
    uint8_t* new_start_address[1] = {&adc_data[adc_channel]};
    start_address[0] = new_start_address[0];

    adc_gpio_pin = get_gpio_from_channel(adc_channel);

    //If ADC is connected to gpio pin init gpio pin
    if(adc_gpio_pin != -1) {    
        gpio_init(adc_gpio_pin);
        adc_gpio_init(adc_gpio_pin);
    } 
    else {
        if(adc_channel == 4) {
            adc_set_temp_sensor_enabled(true);
            temperature_sensor_is_configured = true;
        }
    }
    //TODO: check if adc is already claimed if not claim adc hardware
    //Init adc
    adc_init();
    adc_set_round_robin(channel_mask);
    //Select input channel
    adc_select_input(adc_channel);

    //FIFO setup of adc
    adc_fifo_setup(
        true, //Enable FIFO
        true, //Set DMA-DREQ
        1, //One transfer
        false, //Do not use error bit
        true  //8-bit transfer
    );
    
    adc_fifo_drain();

    if(adc_sample_rate > 500*1000) {
        return -1; //Error: wrong parameter to high sample rate
    }
    //Set sample rate with the adc clk divisor
    adc_clk_div = ((48*1000*1000)/(adc_sample_rate*96))-1;
    adc_set_clkdiv(adc_clk_div);

    //DMA-Setup:

    //Check if one of the used dma channels are already claimed
    if(dma_channel_is_claimed(dma_capture_channel) || dma_channel_is_claimed(dma_control_channel)) {
        return -3; //Error dma channel is already claimed
    }
    //Claim dma channels
    dma_channel_claim(dma_capture_channel);
    dma_channel_claim(dma_control_channel);
    dma_adc_capture_channel = dma_capture_channel;
    dma_adc_control_channel = dma_control_channel;

    //Get configuration for dma control channel
    dma_channel_config dma_ctrl_conf = dma_channel_get_default_config(dma_control_channel);
    //Get configuration for capture dma channel
    dma_channel_config dma_capture_conf = dma_channel_get_default_config(dma_capture_channel);

   //Configure dma capture channel
    channel_config_set_transfer_data_size(&dma_capture_conf, DMA_SIZE_8); 
    channel_config_set_read_increment(&dma_capture_conf, false); //Read not incrementing (adc FIFO register is a single register)
    channel_config_set_write_increment(&dma_capture_conf, false); //In single channel mode do not increment write address
    channel_config_set_irq_quiet(&dma_capture_conf, true); 
    channel_config_set_dreq(&dma_capture_conf, DREQ_ADC); //If a adc-sample is taken start DMA-transfer
    channel_config_set_chain_to(&dma_capture_conf, dma_control_channel); //Set chain to the dma control channel
    channel_config_set_enable(&dma_capture_conf, true);
    
    dma_channel_configure(
    dma_capture_channel,//Channel to be configured
    &dma_capture_conf, //Channel configuration
    NULL,            // write (destination) address will be controlled by the dma control channel.
    &adc_hw->fifo,      // read (source) address. Does not change.
    1, // Number of word transfers.
    false //Don't Start immediately.
    );

    //Configure dma capture channel
    channel_config_set_transfer_data_size(&dma_ctrl_conf, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_ctrl_conf, false); //Read a single uint32.
    channel_config_set_write_increment(&dma_ctrl_conf, false);//No write increment write a single uint32
    channel_config_set_irq_quiet(&dma_ctrl_conf, true);
    channel_config_set_dreq(&dma_ctrl_conf, DREQ_FORCE); //Go as fast as possible.
    channel_config_set_enable(&dma_ctrl_conf, true);

    // Apply reconfig channel configuration.
    dma_channel_configure(
        dma_control_channel,  // Channel to be configured
        &dma_ctrl_conf,
        &dma_hw->ch[dma_capture_channel].al2_write_addr_trig, //Write (destination) address, writing at this address re-triggers the capture channel.
        start_address,  //Read (source) address is a single array with the starting address.
        1,          // Number of word transfers.
        false       // Don't Start immediately.
    );

    //Set configuration flag:
    adc_channel_is_configured[adc_channel] = true;
    //ADC is setup for single channel conversion
    adc_mode_single_channel = true;
    adc_number_of_channels = 1;
    
    return 1;

}//end configure_adc_single_channel

//TODO: Multichannel needs proper testing
int configure_adc_multi_channel(uint *adc_channels, uint8_t number_of_adc_channels, float adc_sample_rate, uint dma_capture_channel, uint dma_control_channel) {
    
    float adc_clk_div = 0;
    int channel_mask = 0;

    if(number_of_adc_channels > MAX_ADC_CHANNELS) {
        return -1; //Error: there are only five adc channels
    }

    if(configure_adc_single_channel) {
        return -2; //Error: adc is configured in single channel already
    }

    //TODO: Implement function further

    //Configure channel gpio/temperature sensor
    for(uint8_t k = 0; k < number_of_adc_channels; k++) {

        int8_t adc_gpio_pin = get_gpio_from_channel(adc_channels[k]);

        if(adc_gpio_pin != -1) {
            gpio_init(adc_gpio_pin);
            adc_gpio_init(adc_gpio_pin);
            channel_list[number_of_adc_channels] = adc_channels[k];
            number_of_adc_channels++;
        }
        else if(adc_channels[k] == 4) {
            adc_set_temp_sensor_enabled(true);
            temperature_sensor_is_configured = true;
            channel_list[number_of_adc_channels] = adc_channels[k];
            number_of_adc_channels++;
        }
        else {
            number_of_adc_channels = 0;
            return -1; //Error there are no other ADC-channels
        }
        //Set channel mask
        channel_mask |= (1 << adc_channels[k]);
    }
    uint8_t* new_start_address[1] = {NULL};
    //Set start address of the right array
    switch(number_of_adc_channels) {

        case 1: return -1; //Error: there is another function for single channel setup    
        case 2: new_start_address[0] = &adc_data_2_channels[0];
                start_address[0] = new_start_address[0];
                break;
        case 3: new_start_address[0] = &adc_data_3_channels[0];
                start_address[0] = new_start_address[0];
                break;
        case 4: new_start_address[0] = &adc_data_4_channels[0];
                start_address[0] = new_start_address[0];
                break;
        case 5: new_start_address[0] = &adc_data[0];
                start_address[0] = new_start_address[0];
                break;
        default: return -1; //Error: there are only five adc channels;
    }

    //TODO: check if adc is already claimed if not claim adc hardware
    //Init adc
    adc_init();
    adc_set_round_robin(channel_mask);
    //Select input channel, first channel in array
    adc_select_input(adc_channels[0]);

     //FIFO setup of adc
    adc_fifo_setup(
        true, //Enable FIFO
        true, //Set DMA-DREQ
        1, //One transfer
        false, //Do not use error bit
        true  //8-bit transfer
    );
    
    adc_fifo_drain();

    if(adc_sample_rate > 500*1000) {
        return -1; //Error: wrong parameter to high sample rate
    }
    //Set sample rate with the adc clk divisor
    adc_clk_div = ((48*1000*1000)/(adc_sample_rate*96))-1;
    adc_set_clkdiv(adc_clk_div);

    //DMA-Setup:

    //Check if one of the used dma channels are already claimed
    if(dma_channel_is_claimed(dma_capture_channel) || dma_channel_is_claimed(dma_control_channel)) {
        return -3; //Error dma channel is already claimed
    }
    //Claim dma channels
    dma_channel_claim(dma_capture_channel);
    dma_channel_claim(dma_control_channel);
    dma_adc_capture_channel = dma_capture_channel;
    dma_adc_control_channel = dma_control_channel;

    //Get configuration for dma control channel
    dma_channel_config dma_ctrl_conf = dma_channel_get_default_config(dma_control_channel);
    //Get configuration for capture dma channel
    dma_channel_config dma_capture_conf = dma_channel_get_default_config(dma_capture_channel);

   //Configure dma capture channel
    channel_config_set_transfer_data_size(&dma_capture_conf, DMA_SIZE_8); 
    channel_config_set_read_increment(&dma_capture_conf, false); //Read not incrementing (adc FIFO register is a single register)
    channel_config_set_write_increment(&dma_capture_conf, false); //In single channel mode do not increment write address
    channel_config_set_irq_quiet(&dma_capture_conf, true); 
    channel_config_set_dreq(&dma_capture_conf, DREQ_ADC); //If a adc-sample is taken start DMA-transfer
    channel_config_set_chain_to(&dma_capture_conf, dma_control_channel); //Set chain to the dma control channel
    channel_config_set_enable(&dma_capture_conf, true);
    
    dma_channel_configure(
    dma_capture_channel,//Channel to be configured
    &dma_capture_conf, //Channel configuration
    NULL,            // write (destination) address will be controlled by the dma control channel.
    &adc_hw->fifo,      // read (source) address. Does not change.
    number_of_adc_channels, // Number of word transfers.
    false //Don't Start immediately.
    );

    //Configure dma capture channel
    channel_config_set_transfer_data_size(&dma_ctrl_conf, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_ctrl_conf, false); //Read a single uint32.
    channel_config_set_write_increment(&dma_ctrl_conf, false);//No write increment write a single uint32
    channel_config_set_irq_quiet(&dma_ctrl_conf, true);
    channel_config_set_dreq(&dma_ctrl_conf, DREQ_FORCE); //Go as fast as possible.
    channel_config_set_enable(&dma_ctrl_conf, true);

    // Apply reconfig channel configuration.
    dma_channel_configure(
        dma_control_channel,  // Channel to be configured
        &dma_ctrl_conf,
        &dma_hw->ch[dma_capture_channel].al2_write_addr_trig, //Write (destination) address, writing at this address re-triggers the capture channel.
        start_address,  //Read (source) address is a single array with the starting address.
        1,          // Number of word transfers.
        false       // Don't Start immediately.
    );

    //Set configuration flag
    for(uint8_t k = 0; k < adc_number_of_channels; k++) {
        adc_channel_is_configured[adc_channels[k]] = true;
    }
    //ADC is setup for multi channel conversion
    adc_mode_single_channel = false;

    return 1;
    
}//end configure_adc_multi_channel

int deconfigure_adc(void) {

    //Stop hardware
    adc_run(false);
    dma_channel_abort(dma_adc_control_channel);

    //Deinit hardware:
    adc_fifo_drain();
       
    dma_channel_cleanup(dma_adc_capture_channel);
    dma_channel_cleanup(dma_adc_control_channel);

    //Unclaim dma channels
    dma_channel_unclaim(dma_adc_capture_channel);
    dma_channel_unclaim(dma_adc_control_channel);

    start_address[0] = NULL;

    if(adc_mode_single_channel) {

        uint8_t adc_channel = 0;
        //Deinit GPIO pins
        for(uint j = 0; j < MAX_ADC_CHANNELS; j++) {
            uint adc_gpio = 0;
            //If one channel is configured deinit gpio/temperature-sensor and return (single channel mode)
            if(adc_channel_is_configured[j]) {
                if(j != 4) {
                    adc_gpio = get_gpio_from_channel(j);
                    gpio_deinit(adc_gpio);
                    adc_channel = j;
                    break;
                }
                else {
                    adc_set_temp_sensor_enabled(false);
                    temperature_sensor_is_configured = false;
                    adc_channel = j;
                    break;
                }
               
            }   
        }

       //Reset flags, static variables and data array
       adc_mode_single_channel = false;
       adc_channel_is_configured[adc_channel] = false;
       dma_adc_capture_channel = 0;
       dma_adc_control_channel = 0;
       adc_data[adc_channel] = 0;

       return 1;

    } 
    
    //Multi channel
    //Deinit data array
    switch(adc_number_of_channels) {
        case 1: return -1; //Error: there is another function for single channel setup    
        case 2: adc_data_2_channels[0] = 0;
                adc_data_2_channels[1] = 0;
                break;
        case 3: adc_data_3_channels[0] = 0;
                adc_data_3_channels[1] = 0;
                adc_data_3_channels[2] = 0;
                break;
        case 4: adc_data_4_channels[0] = 0;
                adc_data_4_channels[1] = 0;
                adc_data_4_channels[2] = 0;
                adc_data_4_channels[3] = 0;
                break;
        case 5: adc_data[0] = 0;
                adc_data[1] = 0;
                adc_data[2] = 0;
                adc_data[3] = 0;
                adc_data[4] = 0;
                break;
        default: return -1; //Error: there are only five adc channels;
    }

    //Deinit GPIO pins
    for(uint j = 0; j < MAX_ADC_CHANNELS; j++) {
    uint adc_gpio = 0;
    //If a channel is configured deinit gpio/temperature-sensor
        if(adc_channel_is_configured[j]) {
            if(j != 4) {
                adc_gpio = get_gpio_from_channel(j);
                gpio_deinit(adc_gpio);
            }
            else {
                adc_set_temp_sensor_enabled(false);
                temperature_sensor_is_configured = false;
                }
            adc_channel_is_configured[j] = false;
            adc_number_of_channels--;  
        }   
        channel_list[j] = 0;
    }
    
    return 1;
    
}//end deconfigure_adc

//Measurement:
int adc_start_measurement(bool start) {

    if(adc_number_of_channels == 0) {
        return -1; //Error no channel was configured
    }

    adc_run(start);
    if(start) {
        dma_channel_start(dma_adc_control_channel);
        return 1;
    }
    dma_channel_abort(dma_adc_control_channel);
    return 1;
    
}//end adc_start_measurment

int adc_get_gpio_measurement(uint adc_gpio_pin, uint8_t *measured_value) {
    //TODO: implement for multi-channel-measurement
    int adc_channel = get_channel(adc_gpio_pin);
    if(adc_channel == -1) {
        return -1; //Error: configured ADC channel is not connected to gpio_pin, or gpio_pin doesn't have analog channel function
    }

    if(adc_channel_is_configured[adc_channel] == false) {
        return -2; //Error: adc channel is not configured
    }

    if(adc_mode_single_channel == true) {
        *measured_value = adc_data[adc_channel];
        return 1;
    }
    else {
        for(uint8_t c = 0; c < adc_number_of_channels; c++) {
            //If the adc_channel of gpio pin matches the array entry in channels list get the value from index c
            if(adc_channel == channel_list[c]) {
                //Read from the right array depending on number of channels configured
                switch(adc_number_of_channels) {
                    case 1: return -1; //Error: there is another function for single channel setup    
                    case 2: *measured_value = adc_data_2_channels[c];
                            return 1;
                    case 3: *measured_value = adc_data_3_channels[c];
                            return 1;
                    case 4: *measured_value = adc_data_4_channels[c];
                            return 1;
                    case 5: *measured_value = adc_data[c];
                            return 1;
                    default: return -1; //Error: there are only five adc channels;
                }
            }
        }
    }

    return -1; //Error: Channel was not found

}//end adc_get_gpio_measurement

int adc_get_temperature_measurement(uint8_t *temperature_value) {

    if(temperature_sensor_is_configured) {

       if(adc_mode_single_channel) {
            *temperature_value = adc_data[4];
            return 1;
       }
       for(uint8_t c = 0; c < adc_number_of_channels; c++) {
            //If the adc_channel of gpio pin matches the array entry in channels list get the value from index c
            if(channel_list[c] == 4) {
                //Read from the right array depending on number of channels configured
                switch(adc_number_of_channels) {
                    case 1: return -1; //Error: there is another function for single channel setup    
                    case 2: *temperature_value = adc_data_2_channels[c];
                            return 1;
                    case 3: *temperature_value = adc_data_3_channels[c];
                            return 1;
                    case 4: *temperature_value = adc_data_4_channels[c];
                            return 1;
                    case 5: *temperature_value = adc_data[c];
                            return 1;
                    default: return -1; //Error: there are only five adc channels;
                }
            }
        }
    }

    return -1; //Error: temperature sensor not configured

}//end adc_get_temperature_measurement

//end file adc.c
