//File: flash.h
//Project: Pico_MRI_Test_M

/* Description:

    Custom flash-wrapper around the Raspberry-Pi-Pico-SDK (hardware/flash.h). 
    Lets user set a offset to the area where user data / utility data is stored.
    User could write/read data from flash, and erase all data.
    For more information about the need for the utility data see the NOTES below. 

    NOTE: This module is not multi-core-save

*/

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:

//Own Libraries:

//Preprocessor constants:

//NOTE: This Preprocessor constant defines the end of flash. Change if using custom chip with a bigger flash. Note that the maximum flash size for the RP2040 is 16MB. 
#define FLASH_END_OFFSET 0x200000

//NOTE: If this driver is changed and the utility section needs more space wich is not used by the user this constant could be changed to match the needs
#define FLASH_UTILITY_USER_DATA_START_INDEX 9

//Type definitions:

//Defines commands received from user in test suite

//Function Prototypes:

//Flash Utility section Functions:

/*NOTE:

    The flash utility section is used for automatic storing of user data to flash. That means so the user of this driver only needs to call the user data functions
    and all the erasing, writing and noting where the user is in flash is handled by this driver. So to use this driver the offset for the utility section always needs
    to be set first, otherwise this driver wont function.

    The flash utility section is one sector long - that means 4096 bytes in size.
    The first page - 256 bytes are used by this driver, and could not be used by the user.

    The remaining 15 pages could be also used by the user - means there is 3840 bytes of user data space inside the utility section.
    The only difference to the other flash user space is that one is not able to erase this sector with the normal means.

    An example of the use of user data in this section would be if the user of this driver wants to enable the user of the product to use the flash to store data,
    like in a GUI. The user of this driver then could save program data inside this section, wich could not be erased by the user of the product.

*/

/**
 * @brief Sets the offset of the flash utility section.
 *
 * This function sets the offset of the flash utility section and initializes the utility data array.
 *
 * @param flash_utility_offset The offset of the flash utility section.
 *
 * @return Returns 1 if the offset is successfully set and the utility section is initialized,
 * 2 if user data is found in the utility section, and -1 if the given offset points to unavailable flash space.
 */
int flash_set_flash_utility_section_offset(uint32_t flash_utility_offset);

int init_flash(void);

/**
 * @brief Writes data to the utility user space in flash memory.
 *
 * This function writes data to the utility user space in flash memory.
 *
 * @param data_to_be_written Pointer to the data to be written.
 * @param num_of_bytes Number of bytes to write.
 *
 * @return Returns 1 on successful write, -1 if the data is too large, and the error code from save_counters_and_offset_to_utility_section function if writing fails.
 */
int flash_write_to_utility_user_space(uint8_t *data_to_be_written, uint16_t num_of_bytes);

/**
 * @brief Reads data from the utility user space in flash memory.
 *
 * This function reads data from the utility user space in flash memory.
 *
 * @param read_data Pointer to store the read data.
 * @param num_of_bytes Number of bytes to read.
 *
 * @return Returns 1 on successful read, -1 if the data is too large, and the error code from flash_read_flash_utility_section function if reading fails.
*/
int flash_read_from_utility_user_space(uint8_t *read_data, uint16_t num_of_bytes);

//Flash user data functions:

/**
 * @brief Checks if user data is stored to flash memory.
 *
 * This function checks if user data is stored to flash memory.
 *
 * @return Returns true if user data is stored to flash memory, otherwise returns false.
 */
bool is_user_data_stored_to_flash(void);

/**
 * @brief Sets the flash target offset for user data and optionally erases the first sector.
 *
 * This function sets the flash target offset for user data and optionally erases the first sector if specified.
 *
 * @param offset The offset to set for user data.
 * @param erase_first_sector Boolean flag indicating whether to erase the first sector or not.
 *
 * @return Returns 1 on successful operation, 2 if user data is stored in flash and settings are successfully restored,
 * -1 if the offset is too big or flash is already full, -2 if the utility section offset was not set.
 */
int flash_set_flash_target_offset_for_user_data(uint32_t flash_target_offset, bool erase_first_sector);

/**
 * @brief Gets the current offset for user data in flash memory.
 *
 * This function retrieves the current offset for user data in flash memory.
 *
 * @return Returns the current offset for user data in flash memory.
 */
uint32_t flash_get_current_user_data_offset(void);

/**
 * @brief Writes bytes to flash memory.
 *
 * This function writes bytes to flash memory, ensuring that the data is written in full pages and handles necessary erasures of flash sectors.
 *
 * @param bytes Pointer to the data to be written.
 * @param num_of_bytes Number of bytes to write.
 * @param data_start_address Pointer to store the start address of the written data.
 *
 * @return Returns 1 on successful write, -1 if the target offset is too big, -2 if the target offset is not set, or the error code from flash_range_program function if writing fails.
 */
/*NOTE: 
    The minimum size is a flash page size (256 bytes) if num_of_bytes is shorter then a page size the other part of the section
    wont be written to but for the next data the offset is one page apart. That means essentially : if(num_of_bytes < 256): 256-num_of_bytes are wasted space on the flash
*/
int write_bytes_to_flash(uint32_t *data_start_address, uint8_t *bytes, uint16_t num_of_bytes);

/**
 * @brief Reads bytes from flash memory.
 *
 * This function reads bytes from flash memory starting from the specified address.
 *
 * @param start_read_address The starting address to read from.
 * @param num_of_bytes_to_read Number of bytes to read.
 * @param bytes Pointer to store the read bytes.
 *
 * @return Returns 1 on successful read, -1 if attempting to read outside the physical flash address space.
 */
int read_bytes_from_flash(uint32_t start_read_address, uint16_t num_of_bytes_to_read, uint8_t *byte);

//FUTURE_FEATURE: Change this function to void so the user of this driver could not mess up. Check for set flash offset.
//FUTURE_FEATURE: Let the user specify how many sectors and from where they want to erase in flash.
/**
 * @brief Erases flash sector.
 *
 * This function erases all the used flash sectors starting from the specified offset. 
 *
 * @param flash_user_data_offset The offset to start erasing flash sectors from. This offset needs to be the offset set, where the user data starts.
 *
 * @return Returns 1 on successful erase, -1 if attempting to erase data at an offset greater than the physically available flash offset, -2 if there are no sectors to erase.
 */
int flash_erase_sector(uint32_t flash_user_data_offset);

//end file flash.h
