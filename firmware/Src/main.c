/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cb.h"
#include "radio.h"
#include "WVT_BME280.h"
#include "WVT_SGP40.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
water7_params_str *Water7_params_p;
/*!
 * \brief
 * \todo check this
 */
main_par_t main_par =
{
  MODE_TEST,0,0,0
};
/*!
 * \brief every second scheduler descriptor
 *
 */
struct scheduler_desc everysec_desc;
/*!
 * \brief measurement on BME280 scheduler descriptor
 *
 */
struct scheduler_desc meas_desc;

bool WeCanSleep = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*!
 * \brief set water7 protocol and meter configuration
 *
 */
void waterAndMeterInit(void)
{
  water7_params_str *water7_params_p = Water7GetParams();
  meter_params_str *meter_params_p = 0;
  /// \todo fix this
  get_saved_param(water7_params_p, meter_params_p);

  Water7PushFunc(WATER7_FUNC_SET_DATA, water7set_data);
  Water7PushFunc(WATER7_FUNC_GET_DATA, water7get_data);
  Water7PushFunc(WATER7_FUNC_SAVE_DATA, water7save_data);
  Water7PushFuncRfl(WATER7_FUNC_RFL, water7_rfl);

  Water7Init(water7_params_p);
  /// \todo fix this
  meter_init(meter_params_p, meter_inc_cb);
  Water7SendEvent(EVENT_RESET, 0);
}

/*!
 * \brief every wakeup tick
 *
 * \param desc
 */
void EverySec(struct scheduler_desc *desc)
{
    time_t timeNow = RTC_GetSeconds();
    Water7OneSec(RTC_GetTime());
}

/*!
 * \brief send complete callback from nb-fi
 *
 * \param ul
 */
void nbfi_send_complete(nbfi_ul_sent_status_t ul)
{
}

/*!
 * \brief receive complete callback from nb-fi
 *
 * \param data
 * \param length
 */
void nbfi_receive_complete(uint8_t *data, uint16_t length)
{
    Water7RXcallback(data, length);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
#ifdef DEBUG
//    LL_DBGMCU_EnableDBGStopMode();
#endif
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_IWDG_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  IWDG_Init();
  /* Ensure that MSI is wake-up system clock */
  LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI);

  //LL_PWR_EnableLowPowerRunMode();

  GPIO_Wa1470_WriteCs(0);//ensure that we disable wa1470 chip select
  SPI_Activate(); //enable SPI
  radio_init(); //Init radio NB-Fi and timers
  waterAndMeterInit(); // Init protocol for NB-Fi
  scheduler_add_task(&everysec_desc, EverySec, RUN_CONTINUOSLY_RELATIVE, SECONDS(30)); //should not exceed 30 seconds, because need to reset the watchdog

  WVT_BME280_Init();
  WVT_SGP40_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    scheduler_run_callbacks();//need check timers
    NBFI_Main_Level_Loop(); 
    IWDG_Refresh();
    if (NBFi_can_sleep() && scheduler_can_sleep() && Water7isCanSleep())
    {
      GPIO_Wa1470_WriteCs(0); //disable chip select pin to ensure that no high level on input pin WA1470
      HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    }
    //      HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_EnableRangeSelection();
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_5);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_PWR_EnableBkUpAccess();
  LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
  LL_RCC_LSE_Enable();

   /* Wait till LSE is ready */
  while(LL_RCC_LSE_IsReady() != 1)
  {

  }
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_SetSystemCoreClock(2000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
     tex: printf_s("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
