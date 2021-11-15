/*!
 * \file WVT_BME280.h
 * \author Sergei Savkin (ssavkin@waviot.ru)
 * \brief User button handler and logic
 * \version 0.1
 * \date 10-07-2020
 *
 * \copyright WAVIoT 2020
 *
 */
#ifndef WVT_BME280_H
#define WVT_BME280_H

#include <stdint.h>

int8_t WVT_BME280_Init(void);
int8_t WVT_BME280_GetData(int32_t* temp, uint32_t* hum, uint32_t* press);
#endif //WVT_BME280_H