#ifndef METER_H_
#define METER_H_

#include <stdint.h>
#include <time.h>

#define SECONDS_IN_HOUR 3600

typedef enum
{
	METER_PARAMS,
	METER_DATA,
	METER_STATE,
} meter_get_set_enum;

typedef struct
{
	int32_t temp;
	int32_t hum;
	uint32_t press;
	int32_t tvoc;
	int32_t co2;
	time_t timestamp;
	uint8_t errors;
	uint32_t crc;
} meter_data_str;

typedef struct
{
	uint32_t ErrorsMask;
	uint32_t crc;
} meter_params_str;

void meter_get(void *data, meter_get_set_enum type);
void meter_set(void *data, meter_get_set_enum type);
void meter_init(meter_params_str *init_params, void fn(void));
void meter_save_data(void);
void MeterClearErrorsMask(void);

#endif
