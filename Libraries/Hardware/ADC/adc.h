//File: adc.h
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

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h"

//Pico Hardware-Libraries:

//Own Libraries:

//Preprocessor constants:

//Type definitions:

//Function prototypes:

//Configuration:

/**
 * @brief Configures ADC for single-channel data capture.
 *
 * This function configures the ADC (Analog-to-Digital Converter) for capturing data from a single ADC channel.
 * It sets up the ADC sampling rate, DMA channels for data transfer, and other necessary configurations.
 *
 * @param adc_channel The ADC channel to be configured for data capture.
 * @param adc_sample_rate The desired sample rate for ADC data capture (samples per second).
 * @param dma_capture_channel The DMA channel to be used for capturing ADC data.
 * @param dma_control_channel The DMA channel to be used for controlling ADC data transfer.
 *
 * @return Returns 1 on successful configuration, -1 for wrong parameters, and -3 if DMA channel is already claimed.
 */
int configure_adc_single_channel(uint adc_channel, float adc_sample_rate, uint dma_capture_channel, uint dma_control_channel);

/**
 * @brief Configures ADC for multi-channel data capture.
 *
 * This function configures the ADC (Analog-to-Digital Converter) for capturing data from multiple ADC channels.
 * It sets up the ADC sampling rate, DMA channels for data transfer, and other necessary configurations.
 *
 * @param adc_channels An array containing the ADC channels to be configured for data capture.
 * @param number_of_adc_channels The number of ADC channels in the adc_channels array.
 * @param adc_sample_rate The desired sample rate for ADC data capture (samples per second).
 * @param dma_capture_channel The DMA channel to be used for capturing ADC data.
 * @param dma_control_channel The DMA channel to be used for controlling ADC data transfer.
 *
 * @return Returns 1 on successful configuration, -1 for wrong parameters, -2 if ADC is configured in single-channel already, and -3 if DMA channel is already claimed.
 */
int configure_adc_multi_channel(uint *adc_channels, uint8_t number_of_adc_channels, float adc_sample_rate, uint dma_capture_channel, uint dma_control_channel);

/**
 * @brief De-configures ADC and associated resources.
 *
 * This function stops the ADC and DMA operations, cleans up associated resources,
 * and resets relevant flags and variables.
 *
 * @return Returns 1 on successful de-configuration, -1 if there is another function for single-channel setup,
 * and -1 if there are only five ADC channels.
 */
int deconfigure_adc(void);

//Measurement:

/**
 * @brief Starts or stops ADC measurement.
 *
 * This function starts or stops ADC measurement based on the provided flag.
 * If the measurement is started, DMA transfer is also initiated to transfer ADC data.
 *
 * @param start Boolean flag indicating whether to start (true) or stop (false) the ADC measurement.
 *
 * @return Returns 1 on successful operation and -1 if no ADC channel was configured.
 */
int adc_start_measurement(bool start);

/**
 * @brief Retrieves ADC measurement for a specified GPIO pin.
 *
 * This function retrieves the measured value from the ADC for a specified GPIO pin.
 *
 * @param adc_gpio_pin The GPIO pin connected to the ADC channel.
 * @param measured_value Pointer to store the measured ADC value.
 *
 * @return Returns 1 on successful retrieval, -1 if configured ADC channel is not connected to gpio_pin
 * or gpio_pin doesn't have analog channel function, -2 if ADC channel is not configured.
 * 
 */
int adc_get_gpio_measurement(uint adc_gpio_pin, uint8_t *measured_value);

/**
 * @brief Retrieves temperature measurement from ADC.
 *
 * This function retrieves the temperature measurement from the ADC if the temperature sensor is configured.
 *
 * @param temperature_value Pointer to store the temperature measurement value.
 *
 * @return Returns 1 on successful retrieval, and -1 if the temperature sensor is not configured.
 */
int adc_get_temperature_measurement(uint8_t *temperature_value);

//end file adc.h
