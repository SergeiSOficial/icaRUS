/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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

/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include "scheduler.h"

volatile uint32_t rtc_counter = 0;

uint16_t PrescalerS = 0;

/*!
 * \brief backup register array
 *
 */
uint32_t aBKPDataReg[RTC_BKP_NUMBER] =
    {
        RTC_BKP_DR0, RTC_BKP_DR1, RTC_BKP_DR2,
        RTC_BKP_DR3, RTC_BKP_DR4, RTC_BKP_DR5,
        RTC_BKP_DR6, RTC_BKP_DR7, RTC_BKP_DR8,
        RTC_BKP_DR9, RTC_BKP_DR10, RTC_BKP_DR11,
        RTC_BKP_DR12, RTC_BKP_DR13, RTC_BKP_DR14,
        RTC_BKP_DR15, RTC_BKP_DR16, RTC_BKP_DR17,
        RTC_BKP_DR18, RTC_BKP_DR19, RTC_BKP_DR20,
        RTC_BKP_DR21, RTC_BKP_DR22, RTC_BKP_DR23,
        RTC_BKP_DR24, RTC_BKP_DR25, RTC_BKP_DR26,
        RTC_BKP_DR27, RTC_BKP_DR28, RTC_BKP_DR29,
        RTC_BKP_DR30, RTC_BKP_DR31};

bool WaitForSynchro_RTC(void);
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_AUGUST;
  sDate.Date = 0x1;
  sDate.Year = 0x21;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable the Alarm A
  */
  sAlarm.AlarmTime.Hours = 0x0;
  sAlarm.AlarmTime.Minutes = 0x0;
  sAlarm.AlarmTime.Seconds = 0x0;
  sAlarm.AlarmTime.SubSeconds = 0x30;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_ALL;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
  if(LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
  {
    FlagStatus pwrclkchanged = RESET;
    /* Update LSE configuration in Backup Domain control register */
    /* Requires to enable write access to Backup Domain if necessary */
    if (LL_APB1_GRP1_IsEnabledClock (LL_APB1_GRP1_PERIPH_PWR) != 1U)
    {
      /* Enables the PWR Clock and Enables access to the backup domain */
      LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
      pwrclkchanged = SET;
    }
    if (LL_PWR_IsEnabledBkUpAccess () != 1U)
    {
      /* Enable write access to Backup domain */
      LL_PWR_EnableBkUpAccess();
      while (LL_PWR_IsEnabledBkUpAccess () == 0U)
      {
      }
    }
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
    LL_RCC_LSE_Enable();

   /* Wait till LSE is ready */
    while(LL_RCC_LSE_IsReady() != 1)
    {
    }
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    /* Restore clock configuration if changed */
    if (pwrclkchanged == SET)
    {
      LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
    }
  }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
    HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/*!
 * \brief read backup register
 * \param registerIndex index of register must be below RTC_BKP_NUMBER
 * \return uint32_t data from register
 */
uint32_t RTC_BackupRead(uint32_t registerIndex)
{
    return HAL_RTCEx_BKUPRead(&hrtc, aBKPDataReg[registerIndex]);
}

/*!
 * \brief write data to backup register
 *
 * \param data for writing to register
 * \param registerIndex index of register  must be below RTC_BKP_NUMBER
 */
void RTC_BackupWrite(uint32_t data, uint32_t registerIndex)
{
    HAL_RTCEx_BKUPWrite(&hrtc, aBKPDataReg[registerIndex], data);
}

uint64_t RTC_GetAbsMilliseconds(void)
{
    return rtc_counter +(uint32_t)HAL_LPRTC_GetCounter() * SECONDS(1) / PrescalerS;
}

uint32_t RTC_GetSeconds(void)
{
    return rtc_counter;
}

void RTC_SetSeconds(uint32_t seconds)
{
    rtc_counter = seconds;
}

/**
  * @brief  Alarm callback \ref stm32l4xx_it.c
  * @param  hrtc : RTC handle
  * @retval None
  */
 void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);
    /* Alarm callback */
    rtc_counter++;
}

void HAL_LPRTC_Start(void)
{
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc,2047,0,0);
  RTC_TimeTypeDef  stimestructure;
  HAL_RTC_GetTime(&hrtc,&stimestructure,RTC_FORMAT_BIN);
  PrescalerS = stimestructure.SecondFraction + 1;
}

void HAL_LPRTC_EnableIt(void)
{
   __HAL_RTC_ALARMA_ENABLE(&hrtc);
}

void HAL_LPRTC_DisableIt(void)
{
   __HAL_RTC_ALARMA_DISABLE(&hrtc);
}

void HAL_LPRTC_SetCompare(uint16_t data)
{
  RTC_AlarmTypeDef salarmstructure;
  /*##-3- Configure the RTC Alarm peripheral #################################*/
  salarmstructure.Alarm = RTC_ALARM_A;
  salarmstructure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
  salarmstructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
  salarmstructure.AlarmMask = RTC_ALARMMASK_ALL;
  salarmstructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
  salarmstructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
  salarmstructure.AlarmTime.SubSeconds = data * PrescalerS / MS_IN_SECOND;

  if(HAL_RTC_SetAlarm_IT(&hrtc,&salarmstructure,RTC_FORMAT_BIN) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

uint16_t HAL_LPRTC_GetCompare(void)
{
  RTC_AlarmTypeDef salarmstructure;
  HAL_RTC_GetAlarm(&hrtc,&salarmstructure,RTC_ALARM_A,RTC_FORMAT_BIN);
   return salarmstructure.SubSeconds * MS_IN_SECOND / PrescalerS;
}

uint16_t HAL_LPRTC_GetCounter(void)
{
  RTC_TimeTypeDef  stimestructure;
  HAL_RTC_GetTime(&hrtc,&stimestructure,RTC_FORMAT_BIN);
    return stimestructure.SubSeconds;
}

uint8_t HAL_LPRTC_CheckIrq(void)
{
    return __HAL_RTC_ALARM_GET_IT(&hrtc, RTC_IT_ALRA);
}


uint16_t HAL_LPRTC_GetPrescalerS(void)
{
    return PrescalerS;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);
//  scheduler_irq();
}

/*!
 * \brief get realtime
 * \return struct tm in unixtimestamp
 */
struct tm RTC_GetTime(void)
{
    struct tm *p_timeNow;
#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)
#if __STDC_WANT_LIB_EXT1__ == 1
    gmtime_s(tm_buf, tt);
#else
    &tt = gmtime(tm_buf);
#endif

#elif defined(__IAR_SYSTEMS_ICC__)
#if __STDC_WANT_LIB_EXT1__ == 1
    gmtime_s((const time_t *)&rtc_counter, &timeNow);
#else
    p_timeNow = gmtime((const time_t *)&rtc_counter);
#endif

#elif !defined(PLATFORM_UTIL_USE_GMTIME)
    gmtime_r(tt, tm_buf);
#endif
    return *p_timeNow;
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
