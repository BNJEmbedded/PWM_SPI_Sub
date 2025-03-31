//File: spi.c
//Project: Pico_MRI_Test_M

/* Description:

    Custom spi-wrapper around the Raspberry-Pi-Pico-SDK (hardware/spi.h). 
    Let user configure SPI as main (master).
    Let user configure SPI as sub (slave) - with rx-interrupt.
    Let tx and rx data as sub or main. 

    NOTE: As the communication in SPI always comes from main, the main have one special tx byte that is currently set to 0x00. When main tx 0x00 it means sub
          should write their tx data to the MISO line (Main polls data from sub).
    NOTE: This code was especially written to enable communication between two Raspberry-Pi-Pico-Boards, that beeing said some functions may not be suitable for other devices.
          For example the sub tx part is described in the above note, these polling techniques may not be necessary for other spi devices.
    NOTE: This module is not multi-core save.

    FUTURE_FEATURE: Make this module multi-core-save
    FUTURE_FEATURE: Let user set the various SPI-format
    FUTURE_FEATURE: Let user configure SPI sub with other tx techniques

*/

//Corresponding header-file:
#include "spi.h"

//Libraries:

//Standard-C:
#include <stdlib.h>
#include <stdio.h>

//Pico:

//Pico High-LvL-Libraries:

//Pico Hardware-Libraries:
#include "hardware/spi.h"

//Own Libraries:

//Preprocessor constants:

//Type definitions:

typedef struct Spi_Config_s {

    spi_inst_t *spi_instance;
    uint spi_miso_pin;
    uint spi_mosi_pin;
    uint spi_clk_pin;
    uint spi_cs_pin;
    bool spi_configured_as_main;
    bool spi_configured_as_sub;

    int spi_data_size;
    volatile bool spi_get_rx_data_complete_flag;
    volatile bool spi_busy_tx_flag;
    uint8_t spi_main_polling_byte;
    uint8_t spi_rx_data[MAX_SPI_DATA_SIZE];
    uint8_t spi_tx_data[MAX_SPI_DATA_SIZE];

}Spi_Config_t;

//File global (static) variables:

static Spi_Config_t spi_config_array[2];

//File global (static) function definitions

static inline void spi0_sub_rx_interrupt_handler(void) {

    //If main is writing data to sub, send data from tx_buffer to main
    if(spi_config_array[0].spi_busy_tx_flag == false) {
        spi_read_blocking(spi0, 0, spi_config_array[0].spi_rx_data, spi_config_array[0].spi_data_size);
        spi_config_array[0].spi_get_rx_data_complete_flag = true;
    }
    else {
        spi_write_blocking(spi0, spi_config_array[0].spi_tx_data, spi_config_array[0].spi_data_size);
        spi_config_array[0].spi_busy_tx_flag = false;
    }

    //Clear IRQ
    irq_clear(SPI0_IRQ);

}//end spi0_sub_rx_interrupt

static inline void spi1_sub_rx_interrupt_handler(void) {

    //If main is writing data to sub, send data from tx_buffer to main
    if(spi_config_array[1].spi_busy_tx_flag == false) {
        spi_read_blocking(spi1, 0, spi_config_array[1].spi_rx_data, spi_config_array[1].spi_data_size);
        spi_config_array[1].spi_get_rx_data_complete_flag = true;
    }
    else {
        spi_write_blocking(spi1, spi_config_array[1].spi_tx_data, spi_config_array[1].spi_data_size);
        spi_config_array[1].spi_busy_tx_flag = false;
    }

    //Clear IRQ
    irq_clear(SPI1_IRQ);

}//end spi1_sub_rx_interrupt

//Function definition:

//SPI hardware configuration
int32_t configure_spi_as_main(spi_inst_t *spi_instance, uint spi_miso_pin, uint spi_mosi_pin, uint spi_clk_pin, uint spi_cs_pin, uint spi_clk_frequency, uint spi_data_length) {
    
    uint8_t config_index = 0;
    uint spi_clk_return;

    if(spi_data_length > MAX_SPI_DATA_SIZE) {
        return -1; //Error: Wrong parameter - data size is to big
    }

    if(spi_instance == spi0) {
        config_index = 0;
        spi_config_array[config_index].spi_instance = spi0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
        spi_config_array[config_index].spi_instance = spi1;

    }
    else {
        return -1; //Error: Wrong parameter - given parameter does not represent real hardware
    }

    gpio_init(spi_mosi_pin);
    gpio_init(spi_miso_pin);
    gpio_init(spi_clk_pin);
    gpio_init(spi_cs_pin);

    gpio_set_function(spi_mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_cs_pin, GPIO_FUNC_SPI);

    //TODO: Check if hardware is available and claim hardware

    if(spi_config_array[config_index].spi_configured_as_main || spi_config_array[config_index].spi_configured_as_sub) {
        return -2; //Error: SPI instance is already configured
    }

    //Init spi as main with given clock frequency
    spi_clk_return = spi_init(spi_config_array[config_index].spi_instance, spi_clk_frequency);
    spi_set_slave(spi_config_array[config_index].spi_instance, false);

    //Write pin numbers to configuration array
    spi_config_array[config_index].spi_mosi_pin = spi_mosi_pin;
    spi_config_array[config_index].spi_miso_pin = spi_miso_pin;
    spi_config_array[config_index].spi_clk_pin = spi_clk_pin;
    spi_config_array[config_index].spi_cs_pin = spi_cs_pin;
    //Write data size
    spi_config_array[config_index].spi_data_size = spi_data_length;

    //Init SPI buffers
    clear_spi_buffer(spi_config_array[config_index].spi_rx_data);
    clear_spi_buffer(spi_config_array[config_index].spi_tx_data);

    //Set flags in configuration array
    spi_config_array[config_index].spi_configured_as_main = true;
    spi_config_array[config_index].spi_configured_as_sub = false;
    //Init these flags - these are never used when spi is configured as main
    spi_config_array[config_index].spi_get_rx_data_complete_flag = false;
    spi_config_array[config_index].spi_busy_tx_flag = false;
    spi_config_array[config_index].spi_get_rx_data_complete_flag = false;

    //TODO: Make polling byte settable 
    spi_config_array[config_index].spi_main_polling_byte = 0x00;

    return (int32_t) spi_clk_return;

}//end configure_spi_as_main

int32_t configure_spi_as_sub(spi_inst_t *spi_instance, uint spi_miso_pin, uint spi_mosi_pin, uint spi_clk_pin, uint spi_cs_pin, uint spi_clk_frequency, uint spi_data_length) {
    
    uint8_t config_index = 0;
    uint spi_clk_return;

    if(spi_data_length > MAX_SPI_DATA_SIZE) {
        return -1; //Error: Wrong parameter - data size is to big
    }

    if(spi_instance == spi0) {
        config_index = 0;
        spi_config_array[config_index].spi_instance = spi0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
        spi_config_array[config_index].spi_instance = spi1;

    }
    else {
        return -1; //Error: Wrong parameter - given parameter does not represent real hardware
    }

    gpio_init(spi_mosi_pin);
    gpio_init(spi_miso_pin);
    gpio_init(spi_clk_pin);
    gpio_init(spi_cs_pin);

    gpio_set_function(spi_mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_clk_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_cs_pin, GPIO_FUNC_SPI);

    //TODO: Check if hardware is available and claim hardware

    if(spi_config_array[config_index].spi_configured_as_main || spi_config_array[config_index].spi_configured_as_sub) {
        return -2; //Error: spi instance is already configured
    }

    //Init spi as sub with given clock frequency
    spi_clk_return = spi_init(spi_config_array[config_index].spi_instance, spi_clk_frequency);
    spi_set_slave(spi_config_array[config_index].spi_instance, true);

    //Activate rx interrupt on sub
    if(config_index == 0) {
        //Enable the RX FIFO interrupt
        spi0_hw->imsc = 1 << 2;
        //Enable the SPI interrupt
        irq_set_enabled(SPI0_IRQ, true);
        //Attach the interrupt handler
        irq_set_exclusive_handler(SPI0_IRQ, spi0_sub_rx_interrupt_handler);

    }
    else {
        //Enable the RX FIFO interrupt
        spi1_hw->imsc = 1 << 2;
        //Enable the SPI interrupt
        irq_set_enabled(SPI1_IRQ, true);
        //Attach the interrupt handler
        irq_set_exclusive_handler(SPI1_IRQ, spi1_sub_rx_interrupt_handler);
    }
        
    //Write pin numbers to configuration array
    spi_config_array[config_index].spi_mosi_pin = spi_mosi_pin;
    spi_config_array[config_index].spi_miso_pin = spi_miso_pin;
    spi_config_array[config_index].spi_clk_pin = spi_clk_pin;
    spi_config_array[config_index].spi_cs_pin = spi_cs_pin;

    //Init SPI buffers
    clear_spi_buffer(spi_config_array[config_index].spi_rx_data);
    clear_spi_buffer(spi_config_array[config_index].spi_tx_data);

    //Set data length
    spi_config_array[config_index].spi_data_size = spi_data_length;

    //Set flags in configuration array
    spi_config_array[config_index].spi_configured_as_main = false;
    spi_config_array[config_index].spi_configured_as_sub = true;
    spi_config_array[config_index].spi_busy_tx_flag = false;
    spi_config_array[config_index].spi_get_rx_data_complete_flag = false;

    return (int32_t)spi_clk_return;

}//end configure_spi_as_sub

int32_t reconfigure_spi(spi_inst_t *spi_instance, uint spi_clk_frequency, uint spi_data_length) {

    bool spi_configured_as_sub = false;
    bool spi_configured_as_main = false;
    uint8_t config_index = 0;
    int32_t return_val = 0;

    if(spi_data_length > MAX_SPI_DATA_SIZE) {
        return -2; //Error data length to big
    }

    if(spi_data_length > MAX_SPI_DATA_SIZE) {
        return -1; //Error: Wrong parameter - data size is to big
    }

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: Wrong parameter - given parameter does not represent real hardware
    }

    //Save configuration status bevor de-configuration
    spi_configured_as_main = spi_config_array[config_index].spi_configured_as_main;
    spi_configured_as_sub = spi_config_array[config_index].spi_configured_as_sub;

    return_val = deconfigure_spi(spi_config_array[config_index].spi_instance);

    if(return_val < 0) {
        return return_val; //Error: Error in de-configuration of spi instance
    }
    
    //If SPI was configured as main - apply reconfiguration as main
    if(spi_configured_as_main == true && spi_configured_as_sub == false) {
        return configure_spi_as_main(spi_config_array[config_index].spi_instance, spi_config_array[config_index].spi_miso_pin, spi_config_array[config_index].spi_mosi_pin, 
        spi_config_array[config_index].spi_clk_pin, spi_config_array[config_index].spi_cs_pin, spi_clk_frequency, spi_data_length);
    }
    //If SPI was configured as sub - apply reconfiguration as sub
    else if(spi_configured_as_main == false && spi_configured_as_sub == true) {

        return configure_spi_as_sub(spi_config_array[config_index].spi_instance, spi_config_array[config_index].spi_miso_pin, spi_config_array[config_index].spi_mosi_pin, 
        spi_config_array[config_index].spi_clk_pin, spi_config_array[config_index].spi_cs_pin, spi_clk_frequency, spi_data_length);

    }

    return -2; //Error: SPI was not configured

}//end reconfigure_spi

int deconfigure_spi(spi_inst_t *spi_instance) {

    uint8_t config_index = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: Given parameter does not represent real hardware
    }

    if(!(spi_config_array[config_index].spi_configured_as_main) && !(spi_config_array[config_index].spi_configured_as_sub)) {
        return -2; //Error: SPI was not configured, no need to de-configure
    }

    clear_spi_buffer(spi_config_array[config_index].spi_tx_data);
    clear_spi_buffer(spi_config_array[config_index].spi_rx_data);

    //Deinit GPIO
    gpio_deinit(spi_config_array[config_index].spi_mosi_pin);
    gpio_deinit(spi_config_array[config_index].spi_miso_pin);
    gpio_deinit(spi_config_array[config_index].spi_clk_pin);
    gpio_deinit(spi_config_array[config_index].spi_cs_pin);

    //Deinit SPI
    spi_deinit(spi_config_array[config_index].spi_instance);

    //TODO: unclaim SPI0 hardware

    spi_config_array[config_index].spi_get_rx_data_complete_flag = false;
    spi_config_array[config_index].spi_busy_tx_flag = false;
    

    //If sub deinit dma channel
    if(spi_config_array[config_index].spi_configured_as_sub) {

        if(config_index == 0) {
            //Disable the SPI interrupt
            irq_set_enabled(SPI0_IRQ, false);
            //Remove the interrupt handler
            irq_remove_handler(SPI0_IRQ, spi0_sub_rx_interrupt_handler);
        }
        else {
            //Disable the SPI interrupt
            irq_set_enabled(SPI1_IRQ, false);
            //Remove the interrupt handler
            irq_remove_handler(SPI1_IRQ, spi1_sub_rx_interrupt_handler);
        }
        spi_config_array[config_index].spi_configured_as_sub = false;
        return 1;
    }

    spi_config_array[config_index].spi_configured_as_main = false; 
    return 1;

}//end reconfigure_spi

//Main-SPI-functions
int inline spi_main_rx_data(spi_inst_t *spi_instance, int8_t *rx_data_from_sub) {

    uint8_t spi_rx_byte = 0x00;
    uint8_t config_index = 0;
    uint k = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(spi_config_array[config_index].spi_configured_as_main == true && spi_config_array[config_index].spi_configured_as_sub == false) {
        //Poll from sub till sub sends back data
        while(k < spi_config_array[config_index].spi_data_size) {     
            spi_read_blocking(spi_config_array[config_index].spi_instance, 0x00, &spi_rx_byte, 1);
            if(spi_rx_byte != 0 && k < spi_config_array[config_index].spi_data_size) {
                k = spi_read_blocking(spi_config_array[config_index].spi_instance, spi_config_array[config_index].spi_main_polling_byte, &rx_data_from_sub[1], 
                spi_config_array[config_index].spi_data_size-1);
                rx_data_from_sub[0] = spi_rx_byte;
                k = k + 1;
                return k;
            }   
        }
    }

    return -2; //Error: SPI is not configured as main
           
}//end spi_main_rx_data

int inline spi_main_tx_data(spi_inst_t *spi_instance, int8_t *tx_data_to_sub) {

    uint8_t config_index = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(spi_config_array[config_index].spi_configured_as_main && !(spi_config_array[config_index].spi_configured_as_sub)) {
        return spi_write_blocking(spi_config_array[config_index].spi_instance, tx_data_to_sub, spi_config_array[config_index].spi_data_size);
    }

    return -2; //Error: SPI not configured as main
    
}//end spi_main_tx_data

//Sub-SPI-functions
int spi_sub_set_tx_data(spi_inst_t *spi_instance, uint8_t *tx_data_to_main) {

    uint8_t config_index = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

     if(!(spi_config_array[config_index].spi_configured_as_main) && spi_config_array[config_index].spi_configured_as_sub) {     
        clear_spi_buffer(spi_config_array[config_index].spi_tx_data);
        for(uint i = 0; i  < spi_config_array[config_index].spi_data_size; i++) {
            spi_config_array[config_index].spi_tx_data[i] = tx_data_to_main[i];
        }
        return 1;
    }
    return -2; //Error: SPI is not configured as sub

}//spi_sub_set_tx_data

int spi_sub_set_tx_busy_flag(spi_inst_t *spi_instance, bool new_flag_status) {

    uint8_t config_index = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(!(spi_config_array[config_index].spi_configured_as_main) && spi_config_array[config_index].spi_configured_as_sub) {
        spi_config_array[config_index].spi_busy_tx_flag = new_flag_status;
        return 1;
    }

    return -2; //Error: SPI instance is not configured as sub

}//end

bool spi_get_tx_busy_flag_status(spi_inst_t *spi_instance) {
   if(spi_instance == spi0) {
        return spi_config_array[0].spi_busy_tx_flag;
   }

   return spi_config_array[1].spi_busy_tx_flag;
}//end spi_get_tx_busy_flag_status

bool spi_sub_get_rx_data_flag_status(spi_inst_t *spi_instance) {

    if(spi_instance == spi0) {
        return spi_config_array[0].spi_get_rx_data_complete_flag;
    }

    return spi_config_array[1].spi_get_rx_data_complete_flag;

}//spi_sub_get_rx_data_flag_status

int spi_sub_get_rx_data(spi_inst_t *spi_instance ,uint8_t *rx_data_from_main) {

    uint8_t config_index = 0;

    if(spi_instance == spi0) {
        config_index = 0;
    }
    else if(spi_instance == spi1) {
        config_index = 1;
    }
    else {
        return -1; //Error: given parameter does not represent real hardware
    }

    if(spi_config_array[config_index].spi_configured_as_main && !(spi_config_array[config_index].spi_configured_as_sub)) {
       return -2; //Error: SPI was not configured as sub
    }

    if(spi_config_array[config_index].spi_get_rx_data_complete_flag) {

        for(uint j = 0; j < spi_config_array[config_index].spi_data_size; j++) {
            rx_data_from_main[j] = spi_config_array[config_index].spi_rx_data[j];
            spi_config_array[config_index].spi_rx_data[j] = '\0';
        }

        spi_config_array[config_index].spi_get_rx_data_complete_flag = false;
        return 1; //Return no error
    }

    return -3; //Error: SPI have no data received, check first with the spi_sub_get_rx_data_flag_status() function

}//spi_sub_get_rx_data

void clear_spi_buffer(uint8_t *spi_buffer) {
     for(uint16_t k = 0; k < MAX_SPI_DATA_SIZE; k++) {
        spi_buffer[k] = '\0';
    }
}

//end file spi.c
