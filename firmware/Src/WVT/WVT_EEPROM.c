/*!
 * \file WVT_EEPROM.c
 * \author Sergei Savkin (ssavkin@waviot.ru)
 * \brief save data to eeprom or flash or backup registers
 * \version 0.1
 * \date 10-07-2020
 *
 * \copyright WAVIoT 2020
 *
 */
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "crc.h"
#include "main.h"
#include "nbfi_types.h"
#include "rtc.h"
#include "water7.h"
#include "update.h"
#include "WVT_EEPROM.h"

/*!
 * \brief Chose last sector
 * !!! Be careful the user area should be in another bank than the code !!!
 *
 */
#define FLASH_USER_START_ADDR ADDR_FLASH_PAGE_62                          /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR (FLASH_USER_START_ADDR + FLASH_PAGE_SIZE - 1) /* End @ of user Flash area */

#define SIZE_NBFI sizeof(nbfi_settings_t)
#define SIZE_METER sizeof(meter_params_str)
#define SIZE_WATER7 WATER7_PAR_LENGTH * 4 //sizeof(water7_params_str)
#define SIZE_ALL (SIZE_NBFI + SIZE_METER + SIZE_WATER7)

/**
  * @brief user data struct definition
  */
typedef __PACKED_UNION
{
    __PACKED_STRUCT
    {
        uint8_t nbfiSetting[SIZE_NBFI];
        uint8_t meterParams[SIZE_METER];
        uint8_t water7Params[SIZE_WATER7];
    };
    uint8_t buffer[SIZE_ALL];
}
EEPROM_DataTypeDef;

uint32_t WVT_EERPROM_CalcMeterCrc(meter_data_str *data);
/*!
 * \brief Load data from flash
 *
 * \param data destination pointer
 * \param type what settings do you need?
 * \return true if memory is empty
 */
int32_t WVT_EEPROM_LoadAll(void *data, enum SettingsType type)
{
    /// \todo check this
    bool isEmpty = true;
    uint32_t *addr = 0;
    size_t size = 0;

    switch (type)
    {
    case EEPROM_NBFI_SETTING:
    {
        addr = (uint32_t *)FLASH_USER_START_ADDR;
        size = SIZE_NBFI;
        break;
    }
    case EEPROM_METER_PARAMS:
    {
        addr = (uint32_t *)(FLASH_USER_START_ADDR + SIZE_NBFI);
        size = SIZE_METER;
        break;
    }
    case EEPROM_WATER7_PARAMS:
    {
        addr = (uint32_t *)(FLASH_USER_START_ADDR + SIZE_NBFI + SIZE_METER);
        size = SIZE_WATER7;
        break;
    }
    default:
    {
        return -1;
        break;
    }
    };
    if (CRC_Crc32(addr, size, 0xFFFFFFFF) == 0)
    {
#if __STDC_WANT_LIB_EXT1__ == 1
        memcpy_s(data, size, addr, size);
#else
        memcpy(data, addr, size);
#endif
        isEmpty = false;
    }
    else
    {
        isEmpty = true;
    }
    return isEmpty;
}

/*!
 * \brief Save settings to flash
 *
 * \param data pointer of settings
 * \param type settings type
 */
int32_t WVT_EEPROM_SaveAll(void *data, enum SettingsType type)
{
    static EEPROM_DataTypeDef oldData = {0};
    int32_t result = 0;
    uint32_t *addr = 0;
    size_t size = 0;
    /// \todo check this on time to execution
#if __STDC_WANT_LIB_EXT1__ == 1
    result |= memcpy_s((void *)oldData.buffer, SIZE_ALL, ((const void *)FLASH_USER_START_ADDR), SIZE_ALL);
#else
    memcpy((void *)oldData.buffer, ((const void *)FLASH_USER_START_ADDR), SIZE_ALL);
#endif
    switch (type)
    {
    case EEPROM_NBFI_SETTING:
    {
        /// \todo check this
        addr = (void *)oldData.nbfiSetting;
        size = SIZE_NBFI;
        ((nbfi_settings_t *)data)->crc = CRC_Crc32((uint32_t *)data, size - sizeof(((nbfi_settings_t *)data)->crc), 0xFFFFFFFF);
        break;
    }
    case EEPROM_METER_PARAMS:
    {
        addr = (void *)oldData.meterParams;
        size = SIZE_METER;
        ((meter_params_str *)data)->crc = CRC_Crc32((uint32_t *)data, size - sizeof(((meter_params_str *)data)->crc), 0xFFFFFFFF);
        break;
    }
    case EEPROM_WATER7_PARAMS:
    {
        addr = (void *)oldData.water7Params;
        size = SIZE_WATER7;
        ((water7_params_str *)data)->crc = CRC_Crc32((uint32_t *)data, size - sizeof(((water7_params_str *)data)->crc), 0xFFFFFFFF);
        break;
    }
    default:
    {
        result = -1;
        return result;
        break;
    }
    };
#if __STDC_WANT_LIB_EXT1__ == 1
    result |= memcpy_s((void *)addr, sizeof(addr), (const void *)data, size);
#else
    memcpy(addr, (const void *)data, size);
#endif
    /// \todo check this
    if (result == 0)
    {
        UpdateInternalFlashErase(USR_FLASH_PAGE_NUMBER);
        UpdateInternalFlashWrite((unsigned char *)oldData.nbfiSetting, FLASH_USER_START_ADDR, SIZE_ALL); /// \todo check for overflow and underflow
    }
    return result;
}

/*!
 * \brief calculate crc32 from meter data
 *
 * \param data
 * \return uint32_t
 */
uint32_t WVT_EERPROM_CalcMeterCrc(meter_data_str *data)
{
    /// \todo fix this, because not right calculation
    return CRC_Crc32((uint32_t *)data, sizeof(meter_data_str) - sizeof(data->crc), 0xFFFFFFFF);
}
/*!
 * \brief read data from backup registers
 *
 * \param data pointer of data
 */
bool WVT_EERPROM_ReadMeterData(meter_data_str *data)
{
    uint32_t tempData = 0;
    for (uint32_t i = 0; i < (sizeof(meter_data_str)) / sizeof(uint32_t); i++)
    {
        tempData = RTC_BackupRead(i);
        //        memcpy_s((void *)(data + i * sizeof(uint32_t)), (void const *)&tempData, sizeof(uint32_t));
        *((uint32_t *)data + i) = tempData;
    }
    if (data->crc == WVT_EERPROM_CalcMeterCrc(data))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*!
 * \brief save data to backup registers
 *
 * \param data to be saved
 */
void WVT_EERPROM_WriteMeterData(meter_data_str *data)
{
    data->crc = WVT_EERPROM_CalcMeterCrc(data);

    for (uint32_t i = 0; i < (sizeof(meter_data_str)) / sizeof(uint32_t); i++)
    {
        uint32_t *ptrValue = (uint32_t *)data + i;
        RTC_BackupWrite(*ptrValue, i); //+ i * sizeof(uint32_t)))
    }
}