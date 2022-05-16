#ifndef __BMS_MAIN_H__
#define __BMS_MAIN_H__

typedef struct set_time
{
    uint8_t seconds; 
    uint8_t minutes; 
    uint8_t hours; 
    uint8_t days; 
    uint8_t months;
    uint8_t week; 
    uint8_t years; 
}SET_TIME_T;

typedef struct _s_universal_form{
char device_id;
}S_UNIVERSAL_FORM;

#endif /*__BMS_MAIN_H__*/