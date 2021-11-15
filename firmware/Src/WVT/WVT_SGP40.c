/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "gpio.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sgp40_i2c.h"
#include "WVT_sgp40.h"

#include <inttypes.h> // PRIu64
#include <math.h>

/* TO USE CONSOLE OUTPUT (//printf) AND WAIT (sleep) YOU MAY NEED TO ADAPT THE
 * INCLUDES ABOVE OR DEFINE THEM ACCORDING TO YOUR PLATFORM.
 * #define //printf(...)
 * #define sleep(...)
 */

int16_t WVT_SGP40_Init(void)
{
    volatile int16_t error = 0;

    sensirion_i2c_hal_init();

    uint16_t serial_number[3];
    uint8_t serial_number_size = 3;

    error = sgp40_get_serial_number(serial_number, serial_number_size);

    if (error) {
//        printf("Error executing sgp40_get_serial_number(): %i\n", error);
    } else {
        // printf("Serial number: %" PRIu64 "\n",
        //         (((uint64_t)serial_number[0]) << 32) |
        //         (((uint64_t)serial_number[1]) << 16) |
        //         ((uint64_t)serial_number[2]));
//        printf("serial: 0x%04x%04x%04x\n", serial_number[0], serial_number[1],
//               serial_number[2]);
//        printf("\n");
    }

    // Start Measurement

    // Parameters for deactivated humidity compensation:
    uint16_t default_rh = 0x8000;
    uint16_t default_t = 0x6666;

    for (int i = 0; i < 5; i++) {
        uint16_t sraw_voc;

        sensirion_i2c_hal_sleep_usec(10000);

        error |= sgp40_measure_raw_signal(default_rh, default_t, &sraw_voc);
        if (error) {
//            printf("Error executing sgp40_measure_raw_signal(): "
//                   "%i\n",
//                   error);
        } else {
//            printf("SRAW VOC: %u\n", sraw_voc);
        }
    }
    uint16_t test_result = 0;
    error |= sgp40_execute_self_test(&test_result);
    if (test_result==0xD400) {
      //no errors
    } else {
      //errors
    }

    error |= sgp40_turn_heater_off();


    return error;
}

int16_t WVT_SGP40_Read(int32_t temp, uint32_t hum, uint16_t *ptrTvoc_ppb, uint16_t *ptrCo2_eq_ppm)
{
    int16_t error = 0;
    uint16_t a_humidity = 0;
    uint16_t a_temp = 0;

    a_humidity = hum * 65535 / 100;
    a_temp = (temp + 45) * 65535 / 175;

    error |= sgp40_measure_raw_signal(a_humidity, a_temp, ptrTvoc_ppb);
    error |= sgp40_turn_heater_off();

    return error;
}