//File: data_to_byte.c
//Project: Pico_MRI_Test_M

/* Description:

    Utility functions for transforming several data types into byte arrays, and corrsponding functions to transform them back from byte array.
    The functions only work for little endian.

*/


//Corresponding header-file:
#include "data_to_byte.h"

//Libraries:

//Standard-C:
#include <stdio.h>
#include <stdlib.h>

//Own Libraries:

//Preprocessor constants:

//File global (static) variables:

//Functions:

//File global (static) function definitions:

//Function definition:

//Float
void float_to_byte_array(float float_to_transform, unsigned char *byte_array) {

    //Transform float to bytes
    unsigned char *float_bytes = (unsigned char *)&float_to_transform;

    // Copy bytes of the float into the byte array
    for (int i = 0; i < sizeof(float); ++i) {
        byte_array[i] = float_bytes[i];
    }

}//end float_to_byte_array
float byte_array_to_float(unsigned char *byte_array) {
    float result;

    //Set address of the byte array to result
    unsigned char *float_bytes = (unsigned char *)&result;

    // Copy bytes from the byte array into the float
    for (int i = 0; i < sizeof(float); ++i) {
        float_bytes[i] = byte_array[i];
    }

    return result;

}//end byte_array_to_float

//unsigned 16-bit-integer
void uint16_t_to_byte_array(unsigned int int_to_transform, unsigned char *byte_array) {
    byte_array[0] = (int_to_transform & 0xFF);          //Least significant byte
    byte_array[1] = ((int_to_transform >> 8) & 0xFF);   //Most significant byte
}//end uint16_t_to_byte_array
unsigned int byte_array_to_uint16_t(unsigned char *byte_array) {
    unsigned int result = byte_array[0];//Initialize with least significant byte
    result |= (byte_array[1] << 8);//Add most significant byte
    return result;
}//end byte_array_to_uint16_t

//unsigned 32-bit-integer
void uint32_t_to_byte_array(unsigned long int_to_transform, unsigned char *byte_array) {
    byte_array[0] = (int_to_transform & 0xFF); //Least significant byte
    byte_array[1] = ((int_to_transform >> 8) & 0xFF);
    byte_array[2] = ((int_to_transform >> 16) & 0xFF);
    byte_array[3] = ((int_to_transform >> 24) & 0xFF);//Most significant byte
}//end uint32_t_to_byte_array
unsigned long byte_array_to_uint32_t(unsigned char *byte_array) {
    unsigned long result = byte_array[0];//Initialize with least significant byte
    result |= (byte_array[1] << 8);
    result |= (byte_array[2] << 16);
    result |= (byte_array[3] << 24);//Add most significant byte
    return result;
}//end byte_array_to_uint32_t

//signed 16-bit-integer
void int16_t_to_byte_array(int int_to_transform, unsigned char *byte_array) {
    byte_array[0] = (int_to_transform & 0xFF);//Least significant byte
    byte_array[1] = ((int_to_transform >> 8) & 0xFF);//Most significant byte
}//end int16_t_to_byte_array
int byte_array_to_int16_t(unsigned char *byte_array) {
    int result = byte_array[0]; //Initialize with least significant byte
    result |= (byte_array[1] << 8);//Add most significant byte
    //If the most significant bit of the most significant byte is set (indicating a negative number),  exte4nd the sign to ensure correct conversion
    if (byte_array[1] & 0x80) {
        result |= 0xFFFF0000;
    }
    return result;
}//end byte_array_to_int16_t

//signed 32-bit-integer
void int32_t_to_byte_array(long int_to_transform, unsigned char *byte_array) {
    byte_array[0] = (int_to_transform & 0xFF);//Least significant byte
    byte_array[1] = ((int_to_transform >> 8) & 0xFF);
    byte_array[2] = ((int_to_transform >> 16) & 0xFF);
    byte_array[3] = ((int_to_transform >> 24) & 0xFF);//Most significant byte
}//end int32_t_to_byte_array
long byte_array_to_int32_t(unsigned char *byte_array) {
    long result = byte_array[0]; //Initialize with least significant byte
    result |= (long)(byte_array[1] << 8);
    result |= (long)(byte_array[2] << 16);
    result |= (long)(byte_array[3] << 24);//Add most significant byte
    //If the most significant bit of the most significant byte is set (indicating a negative number), extend the sign to ensure correct conversion 
    if (byte_array[3] & 0x80) {
        result |= 0xFFFFFFFF00000000;
    }
    return result;
}//end byte_array_to_int32_t

//end file data_to_byte.c
