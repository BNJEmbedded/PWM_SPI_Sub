//File: flash.c
//Project: Pico_MRI_Test_M

/* Description:

    Custom flash-wrapper around the Raspberry-Pi-Pico-SDK (hardware/flash.h). 
    Lets user set a offset to the area where user data / utility data is stored.
    User could write/read data from flash, and erase all data.
    For more information about the need for the utility data see the NOTES below. 

    NOTE: This module is not multi-core-save

*/

//Corresponding header-file:
#include "flash.h"

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
#include "hardware/flash.h"
#include "hardware/sync.h"

//Own Libraries:
#include "data_to_byte.h"

//Preprocessor constants:

//File global (static) variables:

//Flash section for flash utility data
static uint32_t flash_utility_section_offset = 0x00;
//Flag that indicates that the utility section offset was set:
static bool flash_utility_section_offset_set_flag = false;
//Buffer to read utility section
static volatile uint8_t flash_utility_data[FLASH_SECTOR_SIZE];

//Flag that indicates if there is user data written to flash
static bool user_data_to_flash_written = false;

//Flash section for user data
//Offset:
static uint32_t flash_target_offset = 0x00;
//Configuration flag:
static bool flash_target_offset_set_flag = false;
//Actual flash address for read:
static uint32_t current_flash_address = 0x00;
//Actual flash page counter and sector counter, for writing and erasing data
static uint16_t flash_page_counter = 0;
static uint16_t flash_sector_counter = 0;

//Functions:

//File global (static) function definitions and implementations:

static int flash_write_flash_utility_section(void) {

    uint32_t interrupts;

    if(!flash_utility_section_offset_set_flag) {
        return -1; //Error: the utility section offset was never set
    }

    interrupts = save_and_disable_interrupts();
    flash_range_program(flash_utility_section_offset, (uint8_t*)flash_utility_data, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);
    return 1;

}//end flash_write_flash_utility_section_offset

static int flash_read_flash_utility_section(void) {

    if(!flash_utility_section_offset_set_flag) {
        return -1; //Error: the utility section offset was never set
    }

    uint8_t *data = (uint8_t*)(flash_utility_section_offset+XIP_BASE);

    for(uint16_t i = 0; i < FLASH_SECTOR_SIZE; i++) {
        flash_utility_data[i] = data[i];  
    }
    return 1;

}//end flash_read_flash_utility_section

static int flash_delete_flash_utility_section() {
    uint32_t interrupts;

    if(!flash_utility_section_offset_set_flag) {
        return -1; //Error: the utility section offset was never set
    }
    if(flash_utility_section_offset >= FLASH_END_OFFSET || flash_utility_section_offset == 0x00000000) {
        return -1; //Error: no physically flash attached at this offset, or trying to delete contents of ROM
    }

    interrupts = save_and_disable_interrupts();
    flash_range_erase(flash_utility_section_offset, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);
    return 1;

}//end flash_delete_flash_utility_section

static int save_counters_and_offset_to_utility_section(void) {

    uint8_t byte_buffer[4] = {0,0,0,0};

    uint16_t_to_byte_array(flash_page_counter, byte_buffer);
    flash_utility_data[1] = byte_buffer[0];
    flash_utility_data[2] = byte_buffer[1];

    uint16_t_to_byte_array(flash_sector_counter, byte_buffer);
    flash_utility_data[3] = byte_buffer[0];
    flash_utility_data[4] = byte_buffer[1];

    uint32_t_to_byte_array(flash_target_offset, byte_buffer);
    flash_utility_data[5] = byte_buffer[0];
    flash_utility_data[6] = byte_buffer[1];
    flash_utility_data[7] = byte_buffer[2];
    flash_utility_data[8] = byte_buffer[3];

    return flash_write_flash_utility_section();

}//end save_counters_and_offset_to_utility_data

//Function definition:

bool is_user_data_stored_to_flash(void) {
    return user_data_to_flash_written;
}

int flash_set_flash_target_offset_for_user_data(uint32_t offset, bool erase_first_sector) {

    uint8_t byte_buffer[4];

    if(!flash_utility_section_offset_set_flag) {
        return -2; //Error: the utility section offset was not set, as for that this flash driver wont work
    }
    
    //Get utility data and check if there is data stored in flash. If there is no data stored in flash set given offset

    if(!user_data_to_flash_written) {

        if(flash_target_offset >= FLASH_END_OFFSET) {
            return -1; //Error: to big offset, its over the maximum address of the flash on the Raspberry-Pi-Pico-Board
        }

        flash_target_offset = offset;
        current_flash_address = (XIP_BASE + flash_target_offset);

        //Delete Data in the first sector
        if(erase_first_sector) {
            flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE*1);
            flash_sector_counter = 1;
        }
    
        flash_target_offset_set_flag = true;
        flash_page_counter = 0;

        return 1; //Success: The given offset was 

    }

    //If there is data stored in flash get page counter, sector counter and current offset

    //Set flag
    user_data_to_flash_written = true;

    byte_buffer[0] = flash_utility_data[1];
    byte_buffer[1] = flash_utility_data[2];

    flash_page_counter = byte_array_to_uint16_t(byte_buffer);

    byte_buffer[0] = flash_utility_data[3];
    byte_buffer[1] = flash_utility_data[4];

    flash_sector_counter = byte_array_to_uint16_t(byte_buffer);

    byte_buffer[0] = flash_utility_data[5];
    byte_buffer[1] = flash_utility_data[6];
    byte_buffer[2] = flash_utility_data[7];
    byte_buffer[3] = flash_utility_data[8];

    flash_target_offset = byte_array_to_uint32_t(byte_buffer);

    if(flash_target_offset >= FLASH_END_OFFSET) {
        flash_target_offset_set_flag = false;
        return -1; //Error: the flash is already full
    }
    if(flash_target_offset == 0) {
        flash_target_offset_set_flag = false;
        return -1; //Error: flash offset in ROM, there may be an error with the flash itself
    }

    current_flash_address = flash_target_offset+XIP_BASE;
    flash_target_offset_set_flag = true;

    return 2; //Success: There was found that there was user data stored in flash, flash settings successfully restored

}//end flash_set_flash_target_offset_for_user_data

uint32_t flash_get_current_user_data_offset(void) {

    return flash_target_offset;

}//end flash_get_current_user_data_offset

int write_bytes_to_flash(uint32_t *data_start_address, uint8_t *bytes, uint16_t num_of_bytes) {

    //Temp variable to store offset
    uint32_t flash_target_offset_temp = 0x000;
    //Write size is the number of pages to be written. If data size is smaller then 256 byte (size of a flash page) it still needs to be written one whole page, because
    //it is only possible to write a full page to flash
    int16_t write_size = (num_of_bytes/FLASH_PAGE_SIZE) + 1;

    //Variable to store the offset for erase
    uint32_t erase_offset = 0;
    //Variable to store the number of sectors to be erased
    uint16_t num_of_sectors = 0;
    //Flag that indicates if the erase offset is set
    bool erase_offset_set = false;
    //Variable to store the interrupts during flash writing and erasing
    uint32_t interrupts;

    if(flash_target_offset_set_flag == false) {
        return -2; //Error: the target offset is not set
    }

    //Counting how many flash pages are needed, if there are over 16 pages used (that means more then one sector)
    //Set the offset to next sector and erase the sectors needed
    for(uint8_t k = 0; k < write_size; k++) {
        flash_page_counter = flash_page_counter + 1;
        if(flash_page_counter == 17) {
            flash_sector_counter++;
            num_of_sectors++;
            if(erase_offset_set == false) {
                erase_offset = flash_target_offset+k*FLASH_PAGE_SIZE;
                if(erase_offset >= FLASH_END_OFFSET) {
                    return -1; //Error: this section is out of range for the standard 2MB flash on the Raspberry-Pi-Pico
                }
                erase_offset_set = true;
            }
            flash_page_counter = 1;
        }
    }

    //If there is an erase offset set erase the number of sectors needed to store the number of given bytes
    if(erase_offset_set == true) {
        //Disable Interrupts during eras flash sectors
        interrupts = save_and_disable_interrupts();
        //Delete Data in the sectors
        flash_range_erase(erase_offset, FLASH_SECTOR_SIZE*num_of_sectors);
        //Restore the interrupts
        restore_interrupts(interrupts);
    }

    //Disable Interrupts during writing to flash
    interrupts = save_and_disable_interrupts();
    //Write Data
    flash_range_program(flash_target_offset, bytes, FLASH_PAGE_SIZE*write_size);
    //Restore the interrupts
    restore_interrupts(interrupts);

    //The current flash address is where the data to read is located
    *data_start_address = current_flash_address;

    flash_target_offset_temp = flash_target_offset + FLASH_PAGE_SIZE*write_size;

    if(flash_target_offset_temp > 0x200000) {
        return -1; //Error: the target offset is to big, there is only 2MB flash on Raspberry-Pi-Pico-Board
    }
    flash_target_offset = flash_target_offset_temp;
    current_flash_address = flash_target_offset+XIP_BASE;

    //There is now data written to flash -> write a one to the first byte in utility section
    flash_utility_data[0] = (uint8_t)1;
    //Set flag that indicates if user data is written to flash
    if(user_data_to_flash_written) {
        //If flag already set do nothing
    }
    else {
        //If first data is stored to flash set flag
        user_data_to_flash_written = true;
    }
    //Save page and sector counter, and current offset to utility section
    save_counters_and_offset_to_utility_section();

    return 1; //Success: User data was successfully written to flash

}//end write_bytes_to_flash

int read_bytes_from_flash(uint32_t start_read_address, uint16_t num_of_bytes_to_read ,uint8_t *bytes) {
    
    if((start_read_address+(uint32_t)num_of_bytes_to_read) >= (FLASH_END_OFFSET+XIP_BASE)) {
        return -1; //Error: trying to read outside of physical flash address space.
    }

    uint8_t *data = (uint8_t*)start_read_address;

    for(uint16_t i = 0; i < num_of_bytes_to_read; i++) {
        bytes[i] = data[i];  
    }
    return 1;

}//end read_bytes_from_flash

int flash_set_flash_utility_section_offset(uint32_t flash_utility_offset) {

    if(flash_utility_offset >= FLASH_END_OFFSET) {
        return -1; //Error: given offset is pointing to not physically available flash space
    }

    flash_utility_section_offset = flash_utility_offset;
    //Set flag
    flash_utility_section_offset_set_flag = true;

}//end flash_set_utility_section_offset

int init_flash() {
    
    if(!flash_utility_section_offset_set_flag) {
        return -1;
    }

    //Init utility data array
    for(uint32_t p = 0; p < FLASH_SECTOR_SIZE; p++) {
        flash_utility_data[p] = 0;
    }

    //Read from utility section
    flash_read_flash_utility_section();

    if(flash_utility_data[0] != 1) {
        user_data_to_flash_written = false;
        flash_delete_flash_utility_section();
        return 1;
    }
    else {
        user_data_to_flash_written = true;
        flash_utility_data[0] = 1;
        return 2;
    }

}

int flash_write_to_utility_user_space(uint8_t *data_to_be_written, uint16_t num_of_bytes) {
    
    int return_value = 0;

    if(num_of_bytes >= FLASH_PAGE_SIZE*15) {
        return -1; //Error: data is to large
    }

    uint32_t j = 0;

    flash_read_flash_utility_section();
    flash_delete_flash_utility_section();

    for(uint32_t k = FLASH_UTILITY_USER_DATA_START_INDEX; j < num_of_bytes; k++) {
        flash_utility_data[k] = data_to_be_written[j];
        j++;
    }

    return_value = save_counters_and_offset_to_utility_section();
    return return_value;
    
}//end flash_write_to_utility_user_space

int flash_read_from_utility_user_space(uint8_t *read_data, uint16_t num_of_bytes) {

    int return_value = 0;

    if(num_of_bytes >= FLASH_PAGE_SIZE*15) {
        return -1; //Error: data is to large
    }

    return_value = flash_read_flash_utility_section();

    if(return_value < 0) {
        return return_value;
    }

    uint32_t j = 0;

    for(uint32_t k = FLASH_UTILITY_USER_DATA_START_INDEX; j < num_of_bytes; k++) {
        read_data[j] = flash_utility_data[k];
        j++;   
    }

    return return_value;

}//end flash_read_from_utility_user_space

int flash_erase_sector(uint32_t flash_user_data_offset) {

    if(flash_user_data_offset >= FLASH_END_OFFSET) {
        return -1; //Error: trying to erase data at an offset that is greater then the physically available flash offset
    }

    if(flash_sector_counter <= 0) {
        return -2; //Error: there are no sectors to erase
    }

    flash_range_erase(flash_user_data_offset, FLASH_SECTOR_SIZE*flash_sector_counter);
    flash_sector_counter = 0;
    flash_target_offset = flash_user_data_offset;

    //Indicate that there is no more user data stored in flash
    flash_utility_data[0] = (uint8_t)0;
    user_data_to_flash_written = false;
    flash_delete_flash_utility_section();
    flash_write_flash_utility_section();
    

    return 1;

}//end flash_erase_sector

//end file flash.c
