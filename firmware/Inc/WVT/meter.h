#ifndef METER_H_
#define METER_H_

#include <stdint.h>
#include <time.h>
#include "water7.h"

#define METER_VOLUME_BY_24_HOUR_OFFSET 14 //first id of liter24ByHour

#define MKL_TO_L(x) ((x) / 1000000)
#define MKL_TO_100ML(x) ((x) / 100000)
#define L_TO_MKL(x) ((x)*1000000)

#define SECONDS_IN_HOUR 3600

#define SIZE_OF_CALIB_Q 7

#define GUARD_TIME_MS 60000

typedef enum
{
	METER_FLOW_FORWARD,
	METER_FLOW_BACKWARD,
	METER_FLOW_DIFF,
	METER_FLOW_SIZE
} meter_flow_enum;

typedef enum
{
	METER_PARAMS,
	METER_DATA,
	METER_STATE,
} meter_get_set_enum;

typedef enum
{
	METER_ERROR_NONE = 0x00,
	METER_ERROR_LEAK = 0x01,
	METER_ERROR_BREAK = 0x02,
	METER_ERROR_FROST = 0x04,
	METER_ERROR_SENSOR = 0x08
} meter_flow_error;

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
	int32_t QInc[SIZE_OF_CALIB_Q];	 //mkl per impulse * second
	int32_t QTimes[SIZE_OF_CALIB_Q]; //ps delta tof
	int32_t QBreak;					 //liter per hour
	int32_t timeToBreak;			 //time s of break
	int32_t QLeak;					 //liter per hour
	int32_t timeToLeak;				 //time s of leak
	int32_t QFrost;					 //time s of frost
	int32_t tempToFrost;			 //temperature to frost
	int32_t zeroThreshold;			 //zero flow threshold
	int32_t reverseThreshold;		 //zero volume threshold
	int32_t initVolumeThreshold;	 //max init volume for reverse installation
	int32_t startThreshold;			 //reverse volume for reverse installation error
	uint32_t ErrorsMask;
	uint32_t crc;
} meter_params_str;

void meter_get(void *data, meter_get_set_enum type);
void meter_set(void *data, meter_get_set_enum type);
void meter_init(meter_params_str *init_params, void fn(void));
void meter_save_data(void);
int64_t MeterGetForwVolumeMkl(void);
int64_t MeterGetReverseVolumeMkl(void);
int64_t MeterGetTotalVolumeMkl(void);
int64_t MeterGetFlow(void);
int16_t MeterGetMaxFlow(void);
int16_t MeterGetMinFlow(void);
void MeterDTofOnePulse(int64_t dTofPs, time_t deltaTimeMs);
uint8_t MeterEverySecHandler(time_t timeNow);
uint8_t MeterErrorGet(void);
uint8_t MeterErrorHandler(uint8_t errors);
void MeterClearErrorsMask(void);
uint32_t meter_irq_handler(void);

#endif
