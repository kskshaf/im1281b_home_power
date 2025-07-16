#ifndef DATA_H
#define DATA_H

#include "im1281b_home_power.h"

typedef volatile struct {
    float U;  // 电压
    float I;  // 电流
    float P;  // 功率
    float PF; // 功率因数
    float T;  // 温度
    float F;  // 频率
} IM1281B_PARAM;

extern IM1281B_PARAM IM1281B_OUT;


int check(enum sp_return result);
uint16_t combine_bytes(uint8_t high, uint8_t low);
uint16_t modbus_crc16(uint8_t *buffer, size_t length);
void ConvertBytesToUInt32Array(uint8_t* data, uint32_t* output);
void get_module_data(uint8_t *frame);
void write_pw_status_to_html(int status);
void error_to_data_file(void);

#endif