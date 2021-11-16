/*!
 * \file WVT_BME280.c
 * \author Sergei Savkin (ssavkin@waviot.ru)
 * \brief Combined humidity and pressure sensor
 * \version 0.1
 * \date 10-10-2020
 *
 * \copyright WAVIoT 2020
 *
 */
#include "WVT_BME280.h"
#include "bme280.h"
#include "gpio.h"
#include "rtc.h"
#include "scheduler_hal.h"
#include "spi.h"
#include <stdio.h>
#include "water7.h"

struct bme280_dev DevBME280;
uint32_t Req_delay;

void user_delay_us(uint32_t period, void *intf_ptr)
{
    /*
     * Return control or wait,
     * for a period amount of milliseconds
     */
    LL_mDelay(1+period/1000);
}

int8_t user_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    /*
     * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
     * to be set low to activate the relevant device on the SPI bus
     */

    /*
     * Data on the bus should be like
     * |----------------+---------------------+-------------|
     * | MOSI           | MISO                | Chip Select |
     * |----------------+---------------------|-------------|
     * | (don't care)   | (don't care)        | HIGH        |
     * | (reg_addr)     | (don't care)        | LOW         |
     * | (don't care)   | (reg_data[0])       | LOW         |
     * | (....)         | (....)              | LOW         |
     * | (don't care)   | (reg_data[len - 1]) | LOW         |
     * | (don't care)   | (don't care)        | HIGH        |
     * |----------------+---------------------|-------------|
     */
    uint8_t *ptr_reg_addr = &reg_addr;
    GPIO_SetSpiState(SPI_DEVICE_BME280, true);
    SPI_Tx(ptr_reg_addr, 1);
    SPI_Rx(reg_data, len);
    GPIO_SetSpiState(SPI_DEVICE_BME280, false);
    return rslt;
}

int8_t user_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    /*
     * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
     * to be set low to activate the relevant device on the SPI bus
     */

    /*
     * Data on the bus should be like
     * |---------------------+--------------+-------------|
     * | MOSI                | MISO         | Chip Select |
     * |---------------------+--------------|-------------|
     * | (don't care)        | (don't care) | HIGH        |
     * | (reg_addr)          | (don't care) | LOW         |
     * | (reg_data[0])       | (don't care) | LOW         |
     * | (....)              | (....)       | LOW         |
     * | (reg_data[len - 1]) | (don't care) | LOW         |
     * | (don't care)        | (don't care) | HIGH        |
     * |---------------------+--------------|-------------|
     */

    GPIO_SetSpiState(SPI_DEVICE_BME280, true);
    SPI_Tx(&reg_addr, 1);
    SPI_Tx((uint8_t *)reg_data, len);
    GPIO_SetSpiState(SPI_DEVICE_BME280, false);
    return rslt;
}

void print_sensor_data(struct bme280_data *comp_data)
{
    // printf_s("%ld, %ld, %ld\r\n", comp_data->temperature, comp_data->pressure, comp_data->humidity);
}

int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev)
{
    int8_t rslt;
    uint8_t settings_sel;

    struct bme280_data comp_data;

    /* Recommended mode of operation: Indoor navigation */
    dev->settings.osr_h = BME280_OVERSAMPLING_16X;
    dev->settings.osr_p = BME280_OVERSAMPLING_16X;
    dev->settings.osr_t = BME280_OVERSAMPLING_16X;
    dev->settings.filter = BME280_FILTER_COEFF_16;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, dev);

    /*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
     *  and the oversampling configuration. */
    Req_delay = bme280_cal_meas_delay(&dev->settings);

    //printf("Temperature, Pressure, Humidity\r\n");
    /* Continuously stream sensor data */

    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    /* Wait for the measurement to complete and print data @25Hz */
    dev->delay_us(Req_delay, dev->intf_ptr);
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
    print_sensor_data(&comp_data);

    return rslt;
}

int8_t WVT_BME280_Init(void)
{
    SPI_Activate();
    int8_t rslt = BME280_OK;

    /* Sensor_0 interface over SPI with native chip select line */
    uint8_t dev_addr = 0;

    DevBME280.intf_ptr = &dev_addr;
    DevBME280.intf = BME280_SPI_INTF;
    DevBME280.read = user_spi_read;
    DevBME280.write = user_spi_write;
    DevBME280.delay_us = user_delay_us;

    rslt = bme280_init(&DevBME280);
    stream_sensor_data_forced_mode(&DevBME280);
    int32_t temp = 0;
    uint32_t hum, press = 0;
    rslt |= WVT_BME280_GetData(&temp, &hum, &press);
    return rslt;
}

int8_t WVT_BME280_GetData(int32_t* temp, uint32_t* hum, uint32_t* press)
{
    int8_t rslt = BME280_OK;
    struct bme280_data comp_data = {0,0,0};
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &DevBME280);
    /* Wait for the measurement to complete and print data @25Hz */
    DevBME280.delay_us(Req_delay, DevBME280.intf_ptr);
    DevBME280.delay_us(40000, DevBME280.intf_ptr);
    rslt |= bme280_get_sensor_data(BME280_ALL, &comp_data, &DevBME280);
    print_sensor_data(&comp_data);
    *temp = comp_data.temperature;
    *hum = comp_data.humidity;
    *press = comp_data.pressure;

//    rslt |= bme280_set_sensor_mode(BME280_SLEEP_MODE, &DevBME280);

    return rslt;
}