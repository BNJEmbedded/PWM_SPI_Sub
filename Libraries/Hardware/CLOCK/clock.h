//File: clock.h
//Project: Pico_MRI_Test_M

/* Description:



*/

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:
#include "pico/stdlib.h" //Pico Standard-Lib for pico specific datatypes in function prototypes
#include "hardware/clocks.h"

//Pico Hardware-Libraries:

//Own Libraries:

//Preprocessor constants:


//Type definitions:
typedef enum EN_State_e {
    OFF = 0,
    ON
}EN_State_t;

//Enum for enumerating on chip plls
typedef enum PLL_Type_e {
    PLL_SYS = 0,
    PLL_USB
}PLL_Type_t;

//Enum for enumerating on chip / external oscillators
typedef enum OSC_Type_e {
    XOSC = 0,
    ROSC
}OSC_Type_t;

typedef enum Clock_Destination_e {
    DEST_REF_CLK = 0,
    DEST_SYS_CLK,
    DEST_USB_CLK,
    DEST_ADC_CLK,
    DEST_PERI_CLK,
    DEST_RTC_CLK,

    DEST_GPIO0_CLK,
    DEST_GPIO1_CLK,
    DEST_GPIO2_CLK,
    DEST_GPIO3_CLK

}Clock_Destination_t;

typedef enum Clock_Sources_e {
    SOURCE_REF_CLK = 0,
    SOURCE_SYS_CLK,
    SOURCE_USB_CLK,
    SOURCE_ADC_CLK ,
    SOURCE_PERI_CLK,
    SOURCE_RTC_CLK,

    SOURCE_USB_PLL,
    SOURCE_SYS_PLL,

    SOURCE_XOSC,
    SOURCE_ROSC,

    SOURCE_GPIO_IN0,
    SOURCE_GPIO_IN1

}Clock_Sources_t;


//Function Prototypes:

//Module init:
void init_clock_module(void);

//PLL:

int enable_pll(PLL_Type_t pll, uint ref_div, uint output_frequency, uint post_div1, uint post_div2);
int disable_pll(PLL_Type_t pll);

//TODO: ROSC:

//Clock domains:

//Enable/Disable, source clock domains, set frequency
int disable_clock_domain(Clock_Destination_t domain);
int enable_clock_domain(Clock_Destination_t domain, Clock_Sources_t source, uint32_t frequency);

//end file clock.h