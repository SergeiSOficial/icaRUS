/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
  enum spi_device_state
  {
    SPI_DEVICE_NONE = 0,
    SPI_DEVICE_BME280 = 1,
    SPI_DEVICE_WA1470 = 2
  };
/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

  void GPIO_EnablePinIrq(void);
  void GPIO_DisablePinIrq(void);
  void GPIO_ChipEnable(void);
  void GPIO_ChipDisable(void);
  uint8_t GPIO_GetIrqPinState(void);
  void GPIO_Wa1470_WriteCs(uint8_t state);
  void GPIO_BME280_WriteCs(uint8_t state);
  bool GPIO_SetSpiState(enum spi_device_state NewSpiState, bool enable);
  enum spi_device_state GPIO_GetSpiState(void);

  void GPIO_EXTI5_Callback(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
