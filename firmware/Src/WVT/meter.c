#include <string.h>
#include <stdbool.h>

#include "meter.h"
#include "meter_defaults.h"
#include "main.h"
#include "rtc.h"
#include "water7.h"
#include "WVT_BME280.h"
#include "WVT_SGP40.h"

void (*_inc_cb)(void);

static meter_data_str _data = {0};


void meter_get(void *data, meter_get_set_enum type)
{
    static int32_t temp = 0;
    static uint32_t hum, press = 0;
    static uint16_t tvoc_ppb, co2_eq_ppm = 0;
    switch (type)
    {
    case METER_DATA:
        WVT_BME280_GetData(&temp, &hum, &press);
        WVT_SGP40_Read(temp, hum, &tvoc_ppb, &co2_eq_ppm);
        _data.temp = temp;
        _data.hum = hum;//"temp ",temp:%d, " hum:", hum:%d, "  press:", press:%d, " tvoc:", tvoc_ppb:%d
        _data.press = press;
        _data.tvoc = tvoc_ppb;
        _data.co2 = co2_eq_ppm;
        _data.timestamp = RTC_GetSeconds();
#if __STDC_WANT_LIB_EXT1__ == 1
        memcpy_s((uint8_t *)data, sizeof(meter_data_str), (uint8_t *)&_data, sizeof(meter_data_str));
#else
        memcpy((uint8_t *)data, (uint8_t *)&_data, sizeof(meter_data_str));
#endif
        break;
    default:
        break;
    }
}

void meter_set(void *data, meter_get_set_enum type)
{
    switch (type)
    {
    case METER_DATA:

#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s((uint8_t *)&_data, sizeof(meter_data_str), (uint8_t *)data, sizeof(meter_data_str));
#else
            memcpy((uint8_t *)&_data, (uint8_t *)data, sizeof(meter_data_str));
#endif
        break;
    default:
        break;
    }
}

void meter_init(meter_params_str *init_params, void fn(void))
{
  UNUSED(fn);

	/// \todo check this
	meter_data_str meter_data_last;
	if (WVT_EERPROM_ReadMeterData(&meter_data_last)) //loaded
	{
#if __STDC_WANT_LIB_EXT1__ == 1
		memcpy_s((uint8_t *)&_data, sizeof(meter_data_str), (uint8_t *)&meter_data_last, sizeof(meter_data_str));
#else
		memcpy((uint8_t *)&_data, (uint8_t *)&meter_data_last, sizeof(meter_data_str));
#endif
	}
}

void meter_save_data(void)
{
    WVT_EERPROM_WriteMeterData(&_data);
}

void MeterClearErrorsMask(void)
{
  _data.errors = 0;
}