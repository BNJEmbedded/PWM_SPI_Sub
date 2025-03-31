//File: statistic.c
//Project: Pico_MRI_Test_M

/* Description:

   Utility functions for getting mean value and standard deviation of given data set

*/


//Corresponding header-file:
#include "statistic.h"

//Libraries:

//Standard-C:
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//Own Libraries:

//Preprocessor constants:

//File global (static) variables:

//Functions:

//File global (static) function definitions:

//Function definition:

float get_mean_value(float *data, unsigned long data_length) {

    float mean_value = 0;

    for(unsigned long n = 0; n < data_length; n++) {
        mean_value = mean_value + data[n];
    }
    mean_value = mean_value/data_length;
    return mean_value;

}//end get_mean_value
float get_std_deviation(float *data, unsigned long data_length) {

    float std_deviation = 0;
    float mean_value = get_mean_value(data, data_length);

    for(unsigned long n = 0; n < data_length; n++) {
        std_deviation = std_deviation + pow((data[n]-mean_value),2);
    }
    std_deviation = sqrt(std_deviation/(data_length-1));
    return std_deviation;

}//end get_std_deviation

//end file statistic.c