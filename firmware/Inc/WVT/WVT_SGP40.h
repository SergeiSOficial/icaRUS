/*!
 * \file WVT_SGP40.h
 * \author Sergei Savkin (ssavkin@waviot.ru)
 * \brief User button handler and logic
 * \version 0.1
 * \date 10-07-2020
 *
 * \copyright WAVIoT 2020
 *
 */
#ifndef WVT_SGP40_H
#define WVT_SGP40_H

#include <stdint.h>

int16_t WVT_SGP40_Init(void);
int16_t WVT_SGP40_Read(int32_t temp, uint32_t hum, uint16_t *ptrTvoc_ppb, uint16_t *ptrCo2_eq_ppm);
#endif //WVT_SGP40_H