//File: data_to_byte.h
//Project: Pico_MRI_Test_M

/* Description:
    Utility functions for transforming several data types into byte arrays, and corrsponding functions to transform them back from byte array.
    The functions only work for little endian.
*/

//Libraries:

//Standard-C:

//Own Libraries:

//Preprocessor constants:

//Type definitions:

//Function Prototypes:

void float_to_byte_array(float float_to_transform, unsigned char *byte_array);
float byte_array_to_float(unsigned char *byte_array);

void uint16_t_to_byte_array(unsigned int int_to_transform, unsigned char *byte_array);
unsigned int byte_array_to_uint16_t(unsigned char*byte_array);

void uint32_t_to_byte_array(unsigned long int_to_transform, unsigned char *byte_array);
unsigned long byte_array_to_uint32_t(unsigned char *byte_array);

void int16_t_to_byte_array(int int_to_transform, unsigned char *byte_array);
int byte_array_to_int16_t(unsigned char *byte_array);

void int32_t_to_byte_array(long int_to_transform, unsigned char *byte_array);
long byte_array_to_int32_t(unsigned char *byte_array);

//end file data_to_byte.h
