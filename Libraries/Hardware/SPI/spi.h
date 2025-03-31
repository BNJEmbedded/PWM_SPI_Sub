//File: spi.h
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

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes

//Pico Hardware-Libraries:
#include "hardware/spi.h" //Hardware SPI for specific spi datatypes

//Own Libraries:

//Preprocessor constants:
#define MAX_SPI_DATA_SIZE 64

//Type definitions:

//Function Prototypes:

//SPI hardware configuration

/**
 * @brief Configures SPI interface as main.
 *
 * This function configures the SPI interface as the main device with the specified parameters.
 *
 * @param spi_instance     Pointer to the SPI instance to be configured.
 * @param spi_miso_pin     GPIO pin for SPI Main In Sub Out (MISO - Master In Slave Out).
 * @param spi_mosi_pin     GPIO pin for SPI Main Out Sub In (MOSI - Master Out Slave In).
 * @param spi_clk_pin      GPIO pin for SPI clock.
 * @param spi_cs_pin       GPIO pin for SPI chip select.
 * @param spi_clk_frequency SPI clock frequency in Hz.
 * @param spi_data_length  Length of SPI data in bytes.
 *
 * @return Returns the configured SPI clock frequency on success, -1 if the data size is too big,
 *         -2 if the SPI instance is already configured, or -1 if the given parameter does not represent real hardware.
 */
int32_t configure_spi_as_main(spi_inst_t *spi_instance, uint spi_miso_pin, uint spi_mosi_pin, uint spi_clk_pin, uint spi_cs_pin, uint spi_clk_frequency, uint spi_data_length);

/**
 * @brief Configures SPI interface as sub.
 *
 * This function configures the SPI interface as the sub device with the specified parameters.
 *
 * @param spi_instance      Pointer to the SPI instance to be configured.
 * @param spi_miso_pin     GPIO pin for SPI Main Out Sub In (MISO - Master In Slave Out).
 * @param spi_mosi_pin     GPIO pin for SPI Main In Sub Out (MOSI - Master Out Slave In).
 * @param spi_clk_pin      GPIO pin for SPI clock.
 * @param spi_cs_pin       GPIO pin for SPI chip select.
 * @param spi_clk_frequency SPI clock frequency in Hz.
 * @param spi_data_length  Length of SPI data in bytes.
 *
 * @return Returns the configured SPI clock frequency on success, -1 if the data size is too big,
 *         -2 if the SPI instance is already configured, or -1 if the given parameter does not represent real hardware.
 */
int32_t configure_spi_as_sub(spi_inst_t *spi_instance, uint spi_miso_pin, uint spi_mosi_pin, uint spi_clk_pin, uint spi_cs_pin, uint spi_clk_frequency, uint spi_data_length);

/**
 * @brief Reconfigures the SPI interface with new settings.
 *
 * This function reconfigures the SPI interface with the new clock frequency and data length.
 *
 * @param spi_instance      Pointer to the SPI instance to be reconfigured.
 * @param spi_clk_frequency SPI clock frequency in Hz.
 * @param spi_data_length   Length of SPI data in bytes.
 *
 * @return Returns the configured SPI clock frequency on success, -2 if the data length is too big,
 *         -1 if the given parameter does not represent real hardware, or -2 if the SPI was not configured.
 */
int32_t reconfigure_spi(spi_inst_t *spi_instance, uint spi_clk_frequency, uint spi_data_length);

/**
 * @brief De-configures the SPI interface.
 *
 * This function de-configures the SPI interface, releasing its resources and resetting its configuration.
 *
 * @param spi_instance Pointer to the SPI instance to be de-configured.
 *
 * @return Returns 1 on success, -1 if the given parameter does not represent real hardware,
 *         -2 if the SPI was not configured, or 1 if the SPI was successfully de-configured.
 */
int deconfigure_spi(spi_inst_t *spi_instance);

//Main-SPI-functions

/**
 * @brief Receives data from the sub-device when SPI is configured as the main device.
 *
 * This function polls data from the sub-device until it receives a response.
 *
 * @param spi_instance Pointer to the SPI instance.
 * @param rx_data_from_sub Pointer to the buffer where the received data from the sub-device will be stored.
 *
 * @return Returns the number of bytes received from the sub-device on success, -1 if the given parameter does not represent real hardware, or -2 if SPI is not configured as the main device.
 */
int spi_main_rx_data(spi_inst_t *spi_instance, int8_t *rx_data_from_sub);

/**
 * @brief Sends data to the sub-device when SPI is configured as the main device.
 *
 * This function writes data to the sub-device.
 *
 * @param spi_instance Pointer to the SPI instance.
 * @param tx_data_to_sub Pointer to the buffer containing the data to be sent to the sub-device.
 *
 * @return Returns the number of bytes written to the sub-device on success, -1 if the given parameter does not represent real hardware, or -2 if SPI is not configured as the main device.
 */
int spi_main_tx_data(spi_inst_t *spi_instance, int8_t *tx_data_to_sub);

//Sub-SPI-functions

/**
 * @brief Sets the data to be transmitted to the main device when SPI is configured as the sub-device.
 *
 * This function sets the data to be transmitted to the main device when SPI is configured as the sub-device.
 *
 * @param spi_instance Pointer to the SPI instance.
 * @param tx_data_to_main Pointer to the buffer containing the data to be transmitted to the main device.
 *
 * @return Returns 1 on success, -1 if the given parameter does not represent real hardware, or -2 if SPI is not configured as the sub-device.
 */
int spi_sub_set_tx_data(spi_inst_t *spi_instance, uint8_t *tx_data_to_main);

/**
 * @brief Sets the busy flag status for the SPI sub-device.
 *
 * This function sets the busy flag status for the SPI sub-device. The busy flag indicates whether the sub-device is currently busy transmitting data.
 *
 * @param spi_instance Pointer to the SPI instance.
 * @param new_flag_status The new status of the busy flag (true for busy, false for not busy).
 *
 * @return Returns 1 on success, -1 if the given parameter does not represent real hardware, or -2 if the SPI instance is not configured as a sub-device.
 */
int spi_sub_set_tx_busy_flag(spi_inst_t *spi_instance, bool new_flag_status);

/**
 * @brief Gets the status of the transmit busy flag for the SPI device.
 *
 * This function retrieves the status of the transmit busy flag for the SPI device. The transmit busy flag indicates whether the SPI device is currently transmitting data.
 *
 * @param spi_instance Pointer to the SPI instance.
 *
 * @return Returns true if the transmit busy flag is set, indicating that the SPI device is currently transmitting data. Returns false otherwise.
 */
bool spi_get_tx_busy_flag_status(spi_inst_t *spi_instance);

/**
 * @brief Gets the status of the receive data flag for the SPI sub-device.
 *
 * This function retrieves the status of the receive data flag for the SPI sub-device. The receive data flag indicates whether the sub-device has completed receiving data.
 *
 * @param spi_instance Pointer to the SPI instance.
 *
 * @return Returns true if the receive data flag is set, indicating that the sub-device has completed receiving data. Returns false otherwise.
 */
bool spi_sub_get_rx_data_flag_status(spi_inst_t *spi_instance);

/**
 * @brief Retrieves received data from the main SPI device when the data reception is complete.
 *
 * This function retrieves received data from the main SPI device when the data reception is complete. It checks if the receive data complete flag is set and copies the received data into the provided buffer. After copying, it clears the receive data buffer and resets the receive data complete flag.
 *
 * @param spi_instance Pointer to the SPI instance.
 * @param rx_data_from_main Pointer to the buffer where the received data from the main SPI device will be copied.
 *
 * @return Returns 1 if the data reception is successful and the data is copied into the buffer. Returns -1 if the given parameter does not represent real hardware. Returns -2 if the SPI was not configured as a sub device. Returns -3 if there is no data received from the main SPI device; use the spi_sub_get_rx_data_flag_status() function to check first.
 */
int spi_sub_get_rx_data(spi_inst_t *spi_instance ,uint8_t *rx_data_from_main);

/**
 * @brief Clears the SPI buffer.
 *
 * This function clears the SPI buffer by setting all elements of the buffer to '\0' (null character).
 * The buffer should be at least MAX_SPI_DATA_SIZE long.
 *
 * @param spi_buffer Pointer to the SPI buffer to be cleared.
 */
void clear_spi_buffer(uint8_t *spi_buffer);

//end file spi.h
