/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

#include "stm32l4xx_ll_crc.h"
#include "stm32l4xx_ll_iwdg.h"
#include "stm32l4xx_ll_crs.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_utils.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_spi.h"
#include "stm32l4xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WVT_EEPROM.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
  typedef struct
  {
    uint32_t work_mode;
    uint32_t display_mode;
    uint32_t voltage;
    uint32_t temperature;
  } main_par_t;

  enum mode_t
  {
    MODE_NORMAL,
    MODE_TEST,
  };

  enum event_t
  {
    EVENT_FROST = 0,
    EVENT_CMD,
    EVENT_HALL1,
    EVENT_HALL2,
    EVENT_TEST_LINK,
    EVENT_RESET,
    EVENT_SENSOR,
    EVENT_BUTTON = 6001,
    EVENT_REED_SWITCH = 6002,
    EVENT_WATER = 6003
  };
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

#define HW_VERSION ((uint32_t)0)
#define HW_SUB_VERSION ((uint32_t)1)
#define FW_VERSION ((uint32_t)1)
#define FW_SUB_VERSION ((uint32_t)7)

//#define BAND				UL868100_DL869550		//	EU
#define BAND UL868800_DL869150 //	RU
  //#define BAND				UL864000_DL863500		//	KZ
  //#define BAND				UL458550_DL453750		//	UZ

#define USE_WA1470
  //#define WA1470_LOG
#define USE_EXTERNAL_RTC


#define SPI_TIMEOUT 50000

/*!
 * \brief milliseconds in seconds
 *
 */
#define MS_IN_SECOND 1000

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MAX_TIMEOUT 10000
#define SPI1_BME_SEL_Pin LL_GPIO_PIN_3
#define SPI1_BME_SEL_GPIO_Port GPIOA
#define LED_RED_Pin LL_GPIO_PIN_3
#define LED_RED_GPIO_Port GPIOB
#define WA_CHIP_EN_Pin LL_GPIO_PIN_4
#define WA_CHIP_EN_GPIO_Port GPIOB
#define WA_IRQ_Pin LL_GPIO_PIN_5
#define WA_IRQ_GPIO_Port GPIOB
#define WA_IRQ_EXTI_IRQn EXTI9_5_IRQn
#define SPI1_NSS_Pin LL_GPIO_PIN_6
#define SPI1_NSS_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
