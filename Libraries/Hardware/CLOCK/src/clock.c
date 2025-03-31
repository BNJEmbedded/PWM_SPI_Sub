//File: clock.c
//Project: Pico_MRI_Test_M

/* Description:


*/


//Corresponding header-file:
#include "clock.h"

//Libraries:

//Standard-C:

//Pico:

//Pico High-LvL-Libraries:

//Pico Hardware-Libraries:
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"


//Own Libraries:

//Preprocessor constants:

//Type definitions:

//Enum for enumerating main chip clock sources (XOSC, ROSC and GPIO IN)
typedef enum Main_Clock_Source_Type_e {
    M_CLK_SRC_XOSC = 0,
    M_CLK_SRC_ROSC,
}Main_Clock_Source_Type_t;

//Enum for enumerating the source types for the System Clock and Reference Clock.
typedef enum REF_SYS_CLK_Source_Types_e {
    SYS_REF_SRC_ROSC = 0,
    SYS_REF_SRC_AUX,
    SYS_REF_SRC_XOSC,
    SYS_REF_SRC_REF
}REF_SYS_CLK_Source_Types_t;

//Enum for enumerating the general source type
typedef enum Clock_Source_Type_s {
    SRC_TYPE_AUX,
    SRC_TYPE_MAIN,
}Clock_Source_Type_t;

//Clock destination types for clocking the clock domains from auxilary sources
typedef enum Clock_Destination_Types_e {

    //Destination clock output over gpio
    CLK_DEST_TYPE_GPOUT_0 = 0,
    CLK_DEST_TYPE_GPOUT_1,
    CLK_DEST_TYPE_GPOUT_2,
    CLK_DEST_TYPE_GPOUT_3,

    //If Reference Clock or System Clock is configured to be sourced by auxilary source
    CLK_DEST_TYPE_REF,
    CLK_DEST_TYPE_SYS,

    //Peripheral clock domain
    CLK_DEST_TYPE_PERI,
    
    //ADC and USB clock domains destination
    CLK_DEST_TYPE_ADC,
    CLK_DEST_TYPE_USB,

    //RTC clock domain
    CLK_DEST_TYPE_RTC

}Clock_Destination_Types_t;

//Enumeration of auxilary clock sources.
typedef enum Clock_AUX_Source_Types_e {

    //PLLs:
    CLK_AUX_SRC_PLL_SYS,
    CLK_AUX_SRC_PLL_USB,

    //Main clock sources:
    CLK_AUX_SRC_GPIO_IN0,
    CLK_AUX_SRC_GPIO_IN1,
    CLK_AUX_SRC_ROSC,
    CLK_AUX_SRC_XOSC,

    //Clock domains:
    CLK_AUX_SRC_SYS_CLK,
    CLK_AUX_SRC_USB_CLK,
    CLK_AUX_SRC_ADC_CLK,
    CLK_AUX_SRC_RTC_CLK,
    CLK_AUX_SRC_REF_CLK

}Clock_AUX_Source_Types_t;

//Structure representing PLL clock source.
typedef struct PLL_s {
    PLL_Type_t pll_type;
    EN_State_t state;
    uint ref_div;
    uint output_frequency;
    uint post_div1;
    uint post_div2;
}PLL_t;

//Structure representing oscillator clock source.
typedef struct OSC_s {
    OSC_Type_t osc_type;
    EN_State_t state;
    uint32_t frequency;
}OSC_t;

//Structure representing GPIO clock input source.
typedef struct GPIO_Clock_s {
    uint8_t gpio_num;
    EN_State_t state;
    uint32_t frequency;
}GPIO_Clock_t;

//Structure representing auxiliary clock source.
typedef struct AUX_Clock_s {
    Clock_AUX_Source_Types_t type;
    EN_State_t state;
    uint32_t frequency;
}AUX_Clock_t;

typedef union CLK_SRC_u {
    PLL_t pll;
    OSC_t osc;
    GPIO_Clock_t gpio;
    AUX_Clock_t aux;
}CLK_SRC_t;

typedef struct Clock_Source_s {
    Clock_Source_Type_t clk_src_type;
    CLK_SRC_t src;
}Clock_Source_t;

//Enumeration of clock domain type. This type is created for configuring clocks, because they take different sources depending on clock.

typedef enum Clock_Domain_Type_e {
    CLK_DOMAIN_REF_SYS = 0,
    CLK_DOMAIN_GPIO_OUT,
    CLK_DOMAIN_PERI_USB_ADC,
    CLK_DOMAIN_RTC

}Clock_Domain_Type_t;

//Structure of a clock domain (called clocks in RP2040 datasheet), this is used for all clock domains other then system and reference clock.
typedef struct Clock_Domain_s {
    Clock_Domain_Type_t domain_type;
    Clock_Destination_Types_t domain;
    Clock_AUX_Source_Types_t aux_clk_src;
    Clock_Source_t *src;
    EN_State_t state;
    uint32_t frequency;
}Clock_Domain_t;

//Structure for reference and system clock domain. These domains have separate structure because they can have more sources then the other domains.
//Depending if the source is one of the main clock or a auxiliary clock the source is choosen (union).
typedef struct REF_SYS_Clock_Domain_s {
    Clock_Domain_Type_t domain_type;
    Clock_Destination_Types_t domain;
    REF_SYS_CLK_Source_Types_t source_type;
    Clock_Source_t *src;

    //No enable state as system and reference clock always needs to be on
    uint32_t frequency;

}REF_SYS_Clock_Domain_t;

//File global (static) variables:

//Clock destinations:
static REF_SYS_Clock_Domain_t sys_clk;
static REF_SYS_Clock_Domain_t ref_clk;
static Clock_Domain_t gpio_clk_x[4];
static Clock_Domain_t usb_clk;
static Clock_Domain_t adc_clk;
static Clock_Domain_t rtc_clk;
static Clock_Domain_t peri_clk;

//PLL:
static PLL_t usb_pll;
static PLL_t sys_pll;

//OSC:
static OSC_t xosc;
static OSC_t rosc;

//Clock sources:

static Clock_Source_t clk_src_gpio_clk_x[4];
static Clock_Source_t clk_src_sys_clk;
static Clock_Source_t clk_src_ref_clk;
static Clock_Source_t clk_src_usb_clk;
static Clock_Source_t clk_src_adc_clk;
static Clock_Source_t clk_src_rtc_clk;
static Clock_Source_t clk_src_peri_clk;

static Clock_Source_t clk_src_usb_pll;
static Clock_Source_t clk_src_sys_pll;

static Clock_Source_t clk_src_gpio_in_0;
static Clock_Source_t clk_src_gpio_in_1;

static Clock_Source_t clk_src_xosc;
static Clock_Source_t clk_src_rosc;


//File global (static) function declaration:

//Conversion between library enumeration and pico clock.h/clock.c module enumeration
static clock_handle_t get_clock_from_domain(Clock_Destination_Types_t domain);
static uint32_t get_aux_clock_source_for_clock_domain(Clock_Domain_t domain);
static uint32_t get_aux_clock_source_for_ref_sys_clk(REF_SYS_Clock_Domain_t domain);
static uint32_t get_main_clock_source_for_ref_sys_clk(REF_SYS_Clock_Domain_t domain);

//PLL:

static int enable_pll_internal(PLL_t *pll, uint ref_div, uint output_frequency, uint post_div1, uint post_div2);
static int disable_pll_internal(PLL_t *pll);

//ROSC:

static int set_rosc_frequency_internal(OSC_t rosc, uint32_t frequency);

//Clock domains:

//Enable/Disable, source clock domains, set frequency
static int disable_clock_domain_internal(Clock_Domain_t *domain);
static int enable_clock_domain_internal(Clock_Domain_t *domain, Clock_Source_t *source, uint32_t frequency);

//System and reference clock settings
static int configure_sys_clock_domain(REF_SYS_Clock_Domain_t *ref_sys_clk_domain, Clock_Source_t *source, uint32_t frequency);

//If clock domain is configured that also act as clock source for other domains the corresponding source variable is changed.
static void reconfigure_source(Clock_Source_t *source, uint32_t new_frequency, EN_State_t new_en_state);
//File global (static) function definition:

static clock_handle_t get_clock_from_domain(Clock_Destination_Types_t domain) {
    switch (domain) {
        case CLK_DEST_TYPE_GPOUT_0: return clk_gpout0;
        case CLK_DEST_TYPE_GPOUT_1: return clk_gpout1;
        case CLK_DEST_TYPE_GPOUT_2: return clk_gpout2;
        case CLK_DEST_TYPE_GPOUT_3: return clk_gpout3;
        case CLK_DEST_TYPE_SYS: return clk_sys;
        case CLK_DEST_TYPE_REF: return clk_ref;
        case CLK_DEST_TYPE_ADC: return clk_adc;
        case CLK_DEST_TYPE_USB: return clk_usb;
        case CLK_DEST_TYPE_PERI: return clk_peri;
        case CLK_DEST_TYPE_RTC: return clk_rtc;
        default: return 10;
    }
}//end get_clock_from_domain

static uint32_t get_aux_clock_source_for_clock_domain(Clock_Domain_t domain) {
    switch(domain.domain) {
        case CLK_DEST_TYPE_GPOUT_0: switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_REF_CLK: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_REF;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_ROSC_CLKSRC;
                                        case CLK_AUX_SRC_RTC_CLK: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_RTC;
                                        case CLK_AUX_SRC_SYS_CLK: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS;
                                        case CLK_AUX_SRC_USB_CLK: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_GPOUT_1: switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_REF_CLK: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_REF;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_ROSC_CLKSRC;
                                        case CLK_AUX_SRC_RTC_CLK: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_RTC;
                                        case CLK_AUX_SRC_SYS_CLK: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_SYS;
                                        case CLK_AUX_SRC_USB_CLK: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_USB;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_GPOUT_2: switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_REF_CLK: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLK_REF;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_RTC_CLK: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLK_RTC;
                                        case CLK_AUX_SRC_SYS_CLK: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLK_SYS;
                                        case CLK_AUX_SRC_USB_CLK: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_CLK_USB;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_GPOUT2_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_GPOUT_3: switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_REF_CLK: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_REF;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_RTC_CLK: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_RTC;
                                        case CLK_AUX_SRC_SYS_CLK: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_SYS;
                                        case CLK_AUX_SRC_USB_CLK: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_CLK_USB;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_GPOUT3_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_ADC:     switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_USB:     switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_PERI:    switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        case CLK_AUX_SRC_SYS_CLK: return CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_RTC:     switch(domain.aux_clk_src) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        default: return INT32_MAX;
    }

}//end get_aux_clock_source_for_clock_domain

static uint32_t get_aux_clock_source_for_ref_sys_clk(REF_SYS_Clock_Domain_t domain) {
    switch(domain.domain) {
        case CLK_DEST_TYPE_SYS:     switch(domain.src->src.aux.type) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        case CLK_AUX_SRC_PLL_SYS: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS;
                                        case CLK_AUX_SRC_ROSC: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_ROSC_CLKSRC;
                                        case CLK_AUX_SRC_XOSC: return CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    };
        case CLK_DEST_TYPE_REF:     switch(domain.src->src.aux.type) {
                                        case CLK_AUX_SRC_GPIO_IN0: return CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0;
                                        case CLK_AUX_SRC_GPIO_IN1: return CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1;
                                        case CLK_AUX_SRC_PLL_USB: return CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB;
                                        default: return INT32_MAX;
                                    };
        default: return INT32_MAX;
    }

}//end get_aux_clock_source_for_ref_sys_clk

static uint32_t get_main_clock_source_for_ref_sys_clk(REF_SYS_Clock_Domain_t domain) {
    switch(domain.domain) {
        case CLK_DEST_TYPE_SYS:     switch(domain.source_type) {
                                        case SYS_REF_SRC_AUX: return CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX;
                                        case SYS_REF_SRC_REF: return CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF;
                                        default: return INT32_MAX;
                                    }
        case CLK_DEST_TYPE_REF:     switch(domain.source_type) {
                                        case SYS_REF_SRC_AUX: return CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH;
                                        case SYS_REF_SRC_XOSC: return CLOCKS_CLK_REF_CTRL_SRC_VALUE_CLKSRC_CLK_REF_AUX;
                                        case SYS_REF_SRC_ROSC: return CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC;
                                        default: return INT32_MAX;
                                    }
        default: return INT32_MAX;
    }

}//end get_main_clock_source_for_ref_sys_clk

//PLL:

//Enables PLL and set dividers and frequency
static int enable_pll_internal(PLL_t *pll, uint ref_div, uint output_frequency, uint post_div1, uint post_div2) {
    if(pll->state == OFF) {

        //Check parameters:

        //Check post div1 for valid range
        if(!(post_div1 >= 1 && post_div1 <= 7)) {
            return -2;
        }

        //Check post div2 for valid range
        if(!(post_div1 >= 1 && post_div1 <= 7)) {
            return -2;
        }

        //Check for right parameter combination
        if(!(post_div1 >= post_div2)) {
            return -2;
        }

        if(pll->pll_type == PLL_SYS) {
            pll_init(pll_sys, ref_div, output_frequency, post_div1, post_div2);
            pll->ref_div = ref_div;
            pll->output_frequency = output_frequency;
            pll->post_div1 = post_div1;
            pll->post_div2 = post_div2;
            pll->state = ON;
            return 1;
        }
        if(pll->pll_type == PLL_USB) {
            pll_init(pll_usb, ref_div, output_frequency, post_div1, post_div2);
            pll->ref_div = ref_div;
            pll->output_frequency = output_frequency;
            pll->post_div1 = post_div1;
            pll->post_div2 = post_div2;
            pll->state = ON;
            return 1;
        }

        //No such hardware instance (wrong parameter) return error.
        return -2;

    }
    //PLL is already activated, first stop PLL then start it again to configure. Note if disabling first make sure that system clock and ref clock are getting another source if this PLL is used as source
    return -1;
}//End enable_pll_internal

//Disables pll, checks if pll is enabled and if not return error (-1). Note if disabling the pll watch out that system and reference clock are not driven from this PLL.
static int disable_pll_internal(PLL_t *pll) {
    if(pll->state == ON) {
         if(pll->pll_type == PLL_SYS) {
            pll_deinit(pll_sys);
            pll->state = OFF;
            return 1;
        }
        if(pll->pll_type == PLL_USB) {
            pll_deinit(pll_usb);
            pll->state = OFF;
            return 1;
        }
            //No such hardware instance available return error.
            return -2; 
    }
    //PLL is not on so it can not be switched off return error.
    return -1;
}//end disable_pll_internal

//TODO: ROSC

//Clock domains:

static int disable_clock_domain_internal(Clock_Domain_t *domain) {

    if(domain->state == ON) {
        clock_stop(get_clock_from_domain(domain->domain));
        domain->state = OFF;
        return 1;
    } 
    //Clock domain is already off.
    return -1;
}//end disable_clock_domain_internal

static int enable_clock_domain_internal(Clock_Domain_t *domain, Clock_Source_t *source, uint32_t frequency) {

    uint32_t source_frequency = 0;
    if(domain->state == OFF) {
        switch(source->clk_src_type) {
            case SRC_TYPE_AUX:  if(source->src.aux.state == OFF) {
                                    //If the source is off return error, no clocking from source possible
                                    return -3;
                                }
                                source_frequency = source->src.aux.frequency;
                                break;
            default:    //Unknown source type or main source type which is not used for clock domains (except clock ref and clock sys) return error (wrong parameter)
                        return -2;
        }

        if(!clock_configure(get_clock_from_domain(domain->domain), 0, get_aux_clock_source_for_clock_domain(*domain), source_frequency, frequency)) {
            //Clock configuration failed return error.
            return -4;
        }

        domain->aux_clk_src = source->src.aux.type;
        domain->src = source;
        domain->frequency = frequency;
        domain->state = ON;

        return 1;  
    }
    //Clock domain is already on. First disable domain then reenable domain.
    return -1;
}//end enable_clock_domain_internal

//System and reference clock settings
static int configure_ref_sys_clock_domain_internal(REF_SYS_Clock_Domain_t *ref_sys_clk_domain, Clock_Source_t *source, uint32_t frequency) {
    uint32_t source_frequency = 0;

    //Check for parameters:
    if(ref_sys_clk_domain->domain_type != CLK_DOMAIN_REF_SYS) {
        //Type is not clock domain reference or system return error (wrong parameter)
        return -2;
    }

    switch(source->clk_src_type) {
    case SRC_TYPE_AUX:  
                        if(source->src.aux.state == OFF) {
                            //If the source is off return error, no clocking from source possible
                            return -3;
                        }
                        source_frequency = source->src.aux.frequency;
                        if(!clock_configure(get_clock_from_domain(ref_sys_clk_domain->domain), get_main_clock_source_for_ref_sys_clk(*ref_sys_clk_domain), get_aux_clock_source_for_ref_sys_clk(*ref_sys_clk_domain), source_frequency, frequency)) {
                            //Clock configuration failed return error.
                            return -4;
                        }
                        ref_sys_clk_domain->source_type = SRC_TYPE_AUX;
                        break;
    case SRC_TYPE_MAIN: if(source->src.osc.state == OFF) {
                            //If the source is off return error, no clocking from source possible
                            return -3;
                        }
                        source_frequency = source->src.osc.frequency;
                        if(!clock_configure(get_clock_from_domain(ref_sys_clk_domain->domain), get_main_clock_source_for_ref_sys_clk(*ref_sys_clk_domain), 0, source_frequency, frequency)) {
                            //Clock configuration failed return error.
                            return -4;
                        }
                        ref_sys_clk_domain->source_type = SRC_TYPE_MAIN;
                        break;
    default:            //Unknown domain source type return error (wrong parameter)
                        return -2;

    }

    ref_sys_clk_domain->frequency = frequency;
    ref_sys_clk_domain->src = source;

    return 1;

}//end configure_ref_sys_clock_domain_internal

//Function definitions:

//Module init:

void init_clock_module(void) {

    //Write values to clock, pll and osc structures.
    //NOTE: These values are only like this if using the standard raspberry pi pico dev board. If crystal is replaced with different crystal/oscillator the frequencies may differ.
    //NOTE: All the startup information is taken from runtime_init_clocks.c of the rp2_common/pico_runtime_init module
    
    //Oscillators:

    //XOSC:
    xosc.osc_type = XOSC;
    xosc.frequency = 12*MHZ;
    xosc.state = ON;

    //Set source:
    
    //ROSC:

    //Read ROSC frequency as it is dependant on PVT.
    rosc.osc_type = ROSC;
    rosc.frequency = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC)*KHZ;
    rosc.state = ON;

    //Set source:

    //PLLs:

    //System PLL:
    sys_pll.output_frequency = 1500*MHZ;
    sys_pll.pll_type = PLL_SYS;
    sys_pll.post_div1 = 6;
    sys_pll.post_div2 = 2;
    sys_pll.ref_div = 1;
    sys_pll.state = ON;

    //Set source:
    clk_src_sys_pll.clk_src_type = SRC_TYPE_AUX;
    clk_src_sys_pll.src.pll = sys_pll;

    //USB PLL:
    usb_pll.output_frequency = 1200*MHZ;
    usb_pll.post_div1 = 5;
    usb_pll.post_div2 = 5;
    usb_pll.ref_div = 1;
    usb_pll.state = ON;

    //Set source

    //Clock domains:

    //Ref clock:
    ref_clk.domain = CLK_DEST_TYPE_REF;
    ref_clk.domain_type = CLK_DOMAIN_REF_SYS;
    ref_clk.frequency = 12*MHZ;
    ref_clk.source_type = SYS_REF_SRC_XOSC;
    ref_clk.src = &clk_src_xosc;

    //Set source:

    //System clock:
    sys_clk.domain = CLK_DEST_TYPE_SYS;
    sys_clk.domain_type = CLK_DOMAIN_REF_SYS;
    sys_clk.frequency = 125*MHZ;
    sys_clk.source_type = SYS_REF_SRC_AUX;
    sys_clk.src = &clk_src_sys_pll;

    //Set source:

    //ADC clock:
    adc_clk.domain = CLK_DEST_TYPE_ADC;
    adc_clk.domain_type = CLK_DOMAIN_PERI_USB_ADC;
    adc_clk.frequency = 45*MHZ;
    adc_clk.aux_clk_src = CLK_AUX_SRC_PLL_USB;
    adc_clk.src = &clk_src_usb_pll;
    adc_clk.state = ON;

    //Set source:

    //USB clock:
    usb_clk.domain = CLK_DEST_TYPE_USB;
    usb_clk.domain_type = CLK_DOMAIN_PERI_USB_ADC;
    usb_clk.frequency = 45*MHZ;
    usb_clk.aux_clk_src = CLK_AUX_SRC_PLL_USB;
    usb_clk.src = &clk_src_usb_pll;
    usb_clk.state = ON;

    //Set source:

    //RTC clock:
    rtc_clk.domain = CLK_DEST_TYPE_RTC;
    rtc_clk.domain_type = CLK_DOMAIN_RTC;
    rtc_clk.frequency = 46875;
    rtc_clk.aux_clk_src = CLK_AUX_SRC_PLL_USB;
    rtc_clk.src = &clk_src_usb_pll;
    rtc_clk.state = ON;

    //Set source:

    //PERI clock:
    peri_clk.domain = CLK_DEST_TYPE_PERI;
    peri_clk.domain_type = CLK_DOMAIN_PERI_USB_ADC;
    peri_clk.frequency = 125*MHZ;
    peri_clk.aux_clk_src = CLK_AUX_SRC_SYS_CLK;
    peri_clk.src = &clk_src_sys_clk;
    peri_clk.state = ON;

    //Set source:

    //Finished setting up structures. Set configuration flag:


}//end init_clock_module

//PLL:

int enable_pll(PLL_Type_t pll, uint ref_div, uint output_frequency, uint post_div1, uint post_div2) {

    switch(pll) {
        case PLL_USB: return enable_pll_internal(&usb_pll, ref_div, output_frequency, post_div1, post_div2);
        case PLL_SYS: return enable_pll_internal(&sys_pll, ref_div, output_frequency, post_div1, post_div2);
        default: return -2; //Return error wrong parameter
    }

}//end enable_pll

int disable_pll(PLL_Type_t pll) {

    switch(pll) {
        case PLL_USB: return disable_pll_internal(&usb_pll);
        case PLL_SYS: return disable_pll_internal(&sys_pll);
        default: return -2; //Return error wrong parameter
    }

}//end disable_pll

//TODO: ROSC:

//Clock domains:

//Enable/Disable, source clock domains, set frequency
int disable_clock_domain(Clock_Destination_t domain) {

    switch(domain) {
        case DEST_USB_CLK: return disable_clock_domain_internal(&usb_clk);
        case DEST_ADC_CLK: return disable_clock_domain_internal(&adc_clk);
        case DEST_RTC_CLK: return disable_clock_domain_internal(&rtc_clk);
        case DEST_PERI_CLK: return disable_clock_domain_internal(&peri_clk);
        case DEST_GPIO0_CLK: return disable_clock_domain_internal(&gpio_clk_x[0]);
        case DEST_GPIO1_CLK: return disable_clock_domain_internal(&gpio_clk_x[1]);
        case DEST_GPIO2_CLK: return disable_clock_domain_internal(&gpio_clk_x[2]);
        case DEST_GPIO3_CLK: return disable_clock_domain_internal(&gpio_clk_x[3]);
        case DEST_SYS_CLK: return -5; //Turning system clock off is not allowed!
        case DEST_REF_CLK: return -5; //Turning reference clock off is not allowed!
        default: return -2; //Wrong parameter error
    }

}//end disable_clock_domain

int enable_clock_domain(Clock_Destination_t domain, Clock_Sources_t source, uint32_t frequency) {
    switch(domain) {
        switch(domain) {
            case DEST_USB_CLK:  switch(source) {
                                case SOURCE_SYS_PLL: enable_clock_domain_internal(&usb_clk, &clk_src_sys_pll, frequency);
                                case SOURCE_GPIO_IN0: enable_clock_domain_internal(&usb_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: enable_clock_domain_internal(&usb_clk, &clk_src_gpio_in_1, frequency);
                                case SOURCE_USB_PLL: enable_clock_domain_internal(&usb_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_ROSC: enable_clock_domain_internal(&usb_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: enable_clock_domain_internal(&usb_clk, &clk_src_xosc, frequency);
                                case SOURCE_SYS_CLK: enable_clock_domain_internal(&usb_clk, &clk_src_sys_clk, frequency);
                                default: return -2;
                            }
            case DEST_ADC_CLK:  switch(source) {
                                case SOURCE_SYS_PLL: enable_clock_domain_internal(&adc_clk, &clk_src_sys_pll, frequency);
                                case SOURCE_GPIO_IN0: enable_clock_domain_internal(&adc_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: enable_clock_domain_internal(&adc_clk, &clk_src_gpio_in_1, frequency);
                                case SOURCE_USB_PLL: enable_clock_domain_internal(&adc_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_ROSC: enable_clock_domain_internal(&adc_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: enable_clock_domain_internal(&adc_clk, &clk_src_xosc, frequency);
                                default: return -2;
                            }
            case DEST_RTC_CLK:  switch(source) {
                                case SOURCE_SYS_PLL: enable_clock_domain_internal(&rtc_clk, &clk_src_sys_pll, frequency);
                                case SOURCE_GPIO_IN0: enable_clock_domain_internal(&rtc_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: enable_clock_domain_internal(&rtc_clk, &clk_src_gpio_in_1, frequency);
                                case SOURCE_USB_PLL: enable_clock_domain_internal(&rtc_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_ROSC: enable_clock_domain_internal(&rtc_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: enable_clock_domain_internal(&rtc_clk, &clk_src_xosc, frequency);
                                default: return -2;
                            }
            case DEST_PERI_CLK: switch(source) {
                                case SOURCE_SYS_PLL: enable_clock_domain_internal(&peri_clk, &clk_src_sys_pll, frequency);
                                case SOURCE_GPIO_IN0: enable_clock_domain_internal(&peri_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: enable_clock_domain_internal(&peri_clk, &clk_src_gpio_in_1, frequency);
                                case SOURCE_USB_PLL: enable_clock_domain_internal(&peri_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_ROSC: enable_clock_domain_internal(&peri_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: enable_clock_domain_internal(&peri_clk, &clk_src_xosc, frequency);
                                default: return -2;
                            }
            case DEST_GPIO0_CLK: switch(source) {
                                case SOURCE_USB_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_usb_clk, frequency);
                                case SOURCE_ADC_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_adc_clk, frequency);
                                case SOURCE_RTC_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_rtc_clk, frequency);
                                case SOURCE_PERI_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_peri_clk, frequency);
                                case SOURCE_USB_PLL: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_usb_pll, frequency);
                                case SOURCE_SYS_PLL: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_sys_pll, frequency);
                                case SOURCE_ROSC: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_xosc, frequency);
                                case SOURCE_SYS_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_sys_clk, frequency);
                                case SOURCE_REF_CLK: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_ref_clk, frequency);
                                case SOURCE_GPIO_IN0: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return enable_clock_domain_internal(&gpio_clk_x[0], &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            case DEST_GPIO1_CLK: switch(source) {
                                case SOURCE_USB_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_usb_clk, frequency);
                                case SOURCE_ADC_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_adc_clk, frequency);
                                case SOURCE_RTC_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_rtc_clk, frequency);
                                case SOURCE_PERI_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_peri_clk, frequency);
                                case SOURCE_USB_PLL: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_usb_pll, frequency);
                                case SOURCE_SYS_PLL: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_sys_pll, frequency);
                                case SOURCE_ROSC: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_xosc, frequency);
                                case SOURCE_SYS_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_sys_clk, frequency);
                                case SOURCE_REF_CLK: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_ref_clk, frequency);
                                case SOURCE_GPIO_IN0: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return enable_clock_domain_internal(&gpio_clk_x[1], &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            case DEST_GPIO2_CLK: switch(source) {
                                case SOURCE_USB_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_usb_clk, frequency);
                                case SOURCE_ADC_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_adc_clk, frequency);
                                case SOURCE_RTC_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_rtc_clk, frequency);
                                case SOURCE_PERI_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_peri_clk, frequency);
                                case SOURCE_USB_PLL: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_usb_pll, frequency);
                                case SOURCE_SYS_PLL: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_sys_pll, frequency);
                                case SOURCE_ROSC: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_xosc, frequency);
                                case SOURCE_SYS_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_sys_clk, frequency);
                                case SOURCE_REF_CLK: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_ref_clk, frequency);
                                case SOURCE_GPIO_IN0: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return enable_clock_domain_internal(&gpio_clk_x[2], &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            case DEST_GPIO3_CLK: switch(source) {
                                case SOURCE_USB_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_usb_clk, frequency);
                                case SOURCE_ADC_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_adc_clk, frequency);
                                case SOURCE_RTC_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_rtc_clk, frequency);
                                case SOURCE_PERI_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_peri_clk, frequency);
                                case SOURCE_USB_PLL: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_usb_pll, frequency);
                                case SOURCE_SYS_PLL: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_sys_pll, frequency);
                                case SOURCE_ROSC: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_xosc, frequency);
                                case SOURCE_SYS_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_sys_clk, frequency);
                                case SOURCE_REF_CLK: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_ref_clk, frequency);
                                case SOURCE_GPIO_IN0: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return enable_clock_domain_internal(&gpio_clk_x[3], &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            case DEST_SYS_CLK:  switch(source) {
                                case SOURCE_USB_PLL: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_SYS_PLL: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_sys_pll, frequency);
                                case SOURCE_ROSC: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_xosc, frequency);
                                case SOURCE_REF_CLK: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_ref_clk, frequency);
                                case SOURCE_GPIO_IN0: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return configure_ref_sys_clock_domain_internal(&sys_clk, &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            case DEST_REF_CLK:  switch(source) {
                                case SOURCE_USB_PLL: return configure_ref_sys_clock_domain_internal(&ref_clk, &clk_src_usb_pll, frequency);
                                case SOURCE_ROSC: return configure_ref_sys_clock_domain_internal(&ref_clk, &clk_src_rosc, frequency);
                                case SOURCE_XOSC: return configure_ref_sys_clock_domain_internal(&ref_clk, &clk_src_xosc, frequency);
                                case SOURCE_GPIO_IN0: return configure_ref_sys_clock_domain_internal(&ref_clk, &clk_src_gpio_in_0, frequency);
                                case SOURCE_GPIO_IN1: return configure_ref_sys_clock_domain_internal(&ref_clk, &clk_src_gpio_in_1, frequency);
                                default: return -2;
                            }
            default: return -2; //Wrong parameter error
        } 
    }

}//end enable_clock_domain

//end file clocks.c