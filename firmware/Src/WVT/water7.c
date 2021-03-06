#include <stdlib.h>
#include <string.h>

/// \note must define _DLIB_TIME_USES_64

#include "nbfi.h"
#include "scheduler.h"
#include "main.h"
#include "water7.h"
#include "radio.h"
#include "nbfi_config.h"

static struct scheduler_desc water7fastdl_off_desc;
static struct scheduler_desc water7crx_off_desc;
/*!
 * \brief send message scheduler descriptor
 *
 */
struct scheduler_desc send_desc;

static int32_t (*_get_data)(int32_t *data);
static int32_t (*_set_data)(int32_t *data);
static int32_t (*_save_data)(int32_t *data);
static int32_t (*_rfl)(uint32_t addr, uint32_t len, uint32_t index, uint8_t *data, uint8_t cmd);

static water7_state_str _state;
static water7_params_str *_waterParams = (water7_params_str *)&_state.parameters_array[WATER7_PARAMS_OFFSET];

void NBFi_Switch_Mode(nbfi_mode_t mode);
void Water7SendMessage(struct scheduler_desc *desc);

static void bigendian_cpy(uint8_t *from, uint8_t *to, uint8_t len)
{
    for (uint8_t i = 0; i != len; i++)
    {
        to[i] = from[len - i - 1];
    }
}

static void UpdatePeriodTable(void)
{
    for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
        _state.PeriodTable[i] = _waterParams->period / HOURS_PER_DAY * (i + 1);
}

static void hours2bits(int32_t *RegularMas, uint8_t *Reg2Bits)
{
    uint32_t maxVal = 1;

    for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
    {
        if (RegularMas[i] > maxVal)
            maxVal = RegularMas[i];
    }

    for (uint8_t i = 0; i < 12; i++)
    {
        Reg2Bits[i] = 0;
        if (maxVal != 0)
        {
            if (RegularMas[0 + i * 2])
                Reg2Bits[i] |= ((((uint32_t)RegularMas[0 + i * 2] * 15 * 99 / maxVal / 100) + 1) << 4) & 0xF0;

            if (RegularMas[1 + i * 2])
                Reg2Bits[i] |= (((uint32_t)RegularMas[1 + i * 2] * 15 * 99 / maxVal / 100) + 1) & 0x0F;
        }
    }
}

static void hours2bits_1bit(int32_t *RegularMas, uint8_t *Reg2Bits)
{
    uint32_t maxVal = 1;

    for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
    {
        if (RegularMas[i] > maxVal)
            maxVal = RegularMas[i];
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        Reg2Bits[i] = 0;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (RegularMas[i * 8 + j])
                Reg2Bits[i] |= (0x01 << (7 - j));
        }
    }
}

static uint16_t Water7PeriodToUINT16(uint32_t period)
{
    uint16_t days = period / SECONDS_PER_DAY;
    if (days > 0x7FF)
        days = 0x7FF;
    period %= SECONDS_PER_DAY;
    uint16_t hours = period / SECONDS_PER_HOUR;

    return (days << 5) | (hours & 0x1F);
}

static void Water7Regular(void)
{
    uint8_t buf[WATER7_REGULAR_BUF_SIZE];
    uint8_t index = 0;

    for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
    {
        uint32_t need_send = READ_BIT(_waterParams->send_flag, (uint64_t)1 << (i & 0x3F));
        uint32_t need_send_hour = READ_BIT(_waterParams->send_hour_flag, (uint64_t)1 << (i & 0x3F));
        uint8_t len = 0;

        if (need_send)
            len += 5;
        if (need_send_hour)
            len += 12 + 2;
        if (len + index > WATER7_REGULAR_BUF_SIZE)
            break;

        if (need_send)
        {
            if (index == 0)
            {
                buf[index++] = CMD_REGULAR | (i & 0x3F) | (need_send_hour ? 0x40 : 0);
                uint16_t tmp = Water7PeriodToUINT16(_waterParams->period);
                bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
                index += 2;
            }
            else
                buf[index++] = (i & 0x3F) | (need_send_hour ? 0x40 : 0);

            bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
            index += 4;

            if (need_send_hour)
            {
                int16_t tmp = _state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevRegular[i];
                bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
                index += 2;
                hours2bits(&_state.RegularMas[i][0], &buf[index]);
                index += 12;
            }
        }
    }

    if (index == 0)
        buf[index++] = CMD_REGULAR;

    NBFi_Send(buf, index);
    _waterParams->mesNum++;

    for (uint8_t i = 0; i < index; i++)
        _waterParams->BufPrev[i] = buf[i];

    // clear errors
    MeterClearErrorsMask();

    _waterParams->length_buf = index;
}

static void Water7RegularShort(void)
{
    uint8_t buf[WATER7_REGULAR_BUF_SIZE];
    uint8_t index = 0, sended_message = 0;

    for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
    {
        uint32_t need_send = READ_BIT(_waterParams->send_flag, (uint64_t)1 << (i & 0x3F));
        uint32_t need_send_hour = READ_BIT(_waterParams->send_hour_flag, (uint64_t)1 << (i & 0x3F));
        index = 0;

        if (need_send)
        {
            buf[index++] = CMD_REGULAR | (i & 0x3F) | (need_send_hour ? 0x40 : 0);
            bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
            index += 4;

            if (need_send_hour)
            {
                hours2bits_1bit(&_state.RegularMas[i][0], &buf[index]);
                index += 3;
            }
            else
            {
                for (uint8_t i = 0; i < 3; i++)
                    buf[index++] = 0;
            }
            NBFi_Send(buf, index);
            _waterParams->mesNum++;

            for (uint8_t i = 0; i < index; i++)
                _waterParams->BufPrev[i] = buf[i];

            _waterParams->length_buf = index;
            sended_message++;
        }
    }

    if (sended_message == 0)
    {
        buf[index++] = CMD_REGULAR;
        NBFi_Send(buf, index);
        _waterParams->mesNum++;

        for (uint8_t i = 0; i < index; i++)
            _waterParams->BufPrev[i] = buf[i];

        _waterParams->length_buf = index;
        sended_message++;
    }
}

void Water7PushFunc(uint8_t func_type, int32_t func(int32_t *))
{
    switch (func_type)
    {
    case WATER7_FUNC_SET_DATA:
        _set_data = func;
        break;
    case WATER7_FUNC_GET_DATA:
        _get_data = func;
        break;
    case WATER7_FUNC_SAVE_DATA:
        _save_data = func;
        break;

    default:
        break;
    }
}
void Water7PushFuncRfl(uint8_t func_type, int32_t func(uint32_t addr, uint32_t len, uint32_t index, uint8_t *data, uint8_t cmd))
{
    if (func_type == WATER7_FUNC_RFL)
    {
        _rfl = func;
    }
}

water7_params_str *Water7GetParams(void)
{
    return _waterParams;
}

void Water7Init(water7_params_str *params)
{
    if ((!params) || (params->mesNum == 0xffffffff) || (params->period != ONEDAY))
    {
#if __STDC_WANT_LIB_EXT1__ == 1
        memset_s((uint8_t *)_waterParams, sizeof(water7_params_str), 0, sizeof(water7_params_str));
#else
        memset((uint8_t *)_waterParams, 0, sizeof(water7_params_str));
#endif
        /// \todo setup this on radio
        _waterParams->send_flag = 0x0f;
        _waterParams->send_hour_flag = 0;
        _waterParams->period = ONEHOUR;
    }
    else if (params != _waterParams)
    {
#if __STDC_WANT_LIB_EXT1__ == 1
        memcpy_s((uint8_t *)_waterParams, sizeof(water7_params_str), (uint8_t *)params, sizeof(water7_params_str));
#else
        memcpy((uint8_t *)_waterParams, (uint8_t *)params, sizeof(water7_params_str));
#endif
    }
    UpdatePeriodTable();

    uint32_t delayTime = (_waterParams->period + rand() % _waterParams->period) / WATER7_ACCEL_REGULAR;
    scheduler_add_task(&send_desc, Water7SendMessage, RUN_SINGLE_RELATIVE, SECONDS(delayTime));
}

uint8_t Water7Loop(void)
{
    return 0;
}

uint8_t Water7isCanSleep(void)
{
    return 1;
}

void Water7SendMessage(struct scheduler_desc *desc)
{

#define WATER7_MAX_NRX_TRY 6
#define WATER7_MAX_DRX_TRY 1

    _state.SecondsRegular = rand() % (_waterParams->period / WATER7_ACCEL_REGULAR);

    static uint32_t fault_prev, fault_count, success_prev, nrx_count, drx_count;

    static nbfi_state_t nbfi_water_state;
    NBFi_get_state(&nbfi_water_state);
    static nbfi_settings_t nbfi_settings;
    NBFi_get_Settings(&nbfi_settings);
    if (nbfi_settings.mode != CRX)
    {
        if (nbfi_water_state.fault_total > fault_prev && nbfi_water_state.success_total == success_prev)
            fault_count++;

        fault_prev = nbfi_water_state.fault_total;
        success_prev = nbfi_water_state.success_total;
        if (nbfi_settings.mode == NRX)
        {
            if (nrx_count >= WATER7_MAX_NRX_TRY)
            {
                nrx_count = fault_count = 0;
                NBFi_Switch_Mode(DRX);
            }
        }
        else
        {
            if (fault_count >= WATER7_MAX_DRX_TRY)
            {
                nrx_count = fault_count = 0;
                NBFi_Switch_Mode(NRX);
            }
        }

        if (nbfi_settings.mode == NRX)
        {
            drx_count = 0;
            nrx_count++;
        }
        else
        {
            nrx_count = 0;
            drx_count++;
        }

        if (drx_count > WATER7_MAX_DRX_TRY)
            Water7Regular();
        else
            Water7RegularShort();

        Water7ForceEvent();
    }

    _state.SecondsRegular = 0;
    for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
    {
        _state.PrevRegularMas[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
        _state.PrevRegular[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
        _state.MasCount = 0;
    }

    uint32_t delayTime = _waterParams->period / WATER7_ACCEL_REGULAR;
    scheduler_add_task(desc, Water7SendMessage, RUN_SINGLE_RELATIVE, SECONDS(delayTime));
}
void Water7OneSec(struct tm newTime)
{
    static struct tm oldTime;

    if (newTime.tm_min != oldTime.tm_min)
    {

        if (newTime.tm_hour != oldTime.tm_hour)
        {
            if (_get_data)
            {
              _get_data(_state.parameters_array);
            }

            meter_save_data();
            for (uint8_t i = 0; i < WATER7_PAR_COUNT - WATER7_PAR_OFFSET_NOT24; i++)
            {
                if ((_state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevMinute[i]) > _waterParams->par_max[i])
                    _waterParams->par_max[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevMinute[i];
                _state.PrevMinute[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
            }

            if (newTime.tm_hour >= _state.PeriodTable[_state.MasCount] / WATER7_ACCEL_REGULAR / SECONDS_IN_HOUR)
            {
                if (_state.MasCount < 24)
                {
                    for (uint8_t i = 0; i < WATER7_PAR_COUNT - WATER7_PAR_OFFSET_NOT24; i++)
                    {
                        _state.RegularMas[i][_state.MasCount] = abs(_state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevRegularMas[i]);
                        _state.PrevRegularMas[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
                    }
                    _state.MasCount++;
                }
            }
            /// for every 24 hour message
            for (uint8_t i = 0; i < WATER7_PAR_OFFSET_NOT24; i++)
            {
                /// \todo check this
                _state.Regular24Hour[i][newTime.tm_hour] = _state.parameters_array[i];
                if (newTime.tm_hour == 0)
                {
                    _state.PrevRegularMas[WATER7_INDEX_24H_PRAMETER + i] = _state.parameters_array[WATER7_INDEX_24H_PRAMETER];
                    _state.parameters_array[WATER7_INDEX_24H_PRAMETER + i] = _state.parameters_array[i];
                    _state.RegularMas[WATER7_INDEX_24H_PRAMETER + i][23] = _state.parameters_array[WATER7_INDEX_24H_PRAMETER] - _state.Regular24Hour[i][23];
                    for (uint8_t j = 0; j < 24 - 1; j++)
                    {
                        /// \todo add Abs to this
                        _state.RegularMas[WATER7_INDEX_24H_PRAMETER + i][j] = _state.Regular24Hour[i][j + 1] - _state.Regular24Hour[i][j];
                    }
                }
            }

            if (newTime.tm_mday != oldTime.tm_mday)
            {
                /// \todo change this on backup registers
                _save_data((int32_t *)_state.parameters_array);
            }
        }
    }

    oldTime = newTime;
}

void NBFi_Switch_Mode(nbfi_mode_t mode)
{
    nbfi_settings_t nbfi_settings;
    NBFi_get_Settings(&nbfi_settings);
    if (nbfi_settings.mode != mode)
    {
        nbfi_settings.mode = mode;
        rf_state = STATE_CHANGED;
        if (mode == NRX)
            nbfi_settings.handshake_mode = HANDSHAKE_NONE;
        else
            nbfi_settings.handshake_mode = HANDSHAKE_SIMPLE;
        NBFi_Config_Send_Mode(0, NBFI_PARAM_MODE);
    }
}

void Water7fastdl_off(struct scheduler_desc *desc)
{
    nbfi_settings_t nbfi_settings;
    NBFi_get_Settings(&nbfi_settings);
    if (nbfi_settings.mode == CRX)
    {
        if (!desc)
            scheduler_remove_task(&water7fastdl_off_desc);
        /// \todo check this
        //radio_switch_to_from_short_range(false);
    }
}

void Water7fastdl_on(void)
{
    nbfi_settings_t nbfi_settings;
    NBFi_get_Settings(&nbfi_settings);
    if (nbfi_settings.mode == NRX || nbfi_settings.mode == DRX)
    {
        /// \todo check this
        //radio_switch_to_from_short_range(true);
        scheduler_add_task(&water7fastdl_off_desc, &Water7fastdl_off, RELATIVE, SECONDS(WATER7_FASTDL_TIMEOUT));
    }
}

void Water7crx_off(struct scheduler_desc *desc)
{
    NBFi_Switch_Mode(DRX);
}

void Water7RXcallback(uint8_t *data, uint16_t length)
{
    uint16_t len = 0;
    uint8_t tmpbuf[WATER7_CALLBACK_BUF_SIZE];

    len = Water7Handler(data, length, tmpbuf);
    if (len > 0)
    {
        NBFi_Send(tmpbuf, len);
    }
}

void Water7PushEvent(uint16_t event, uint16_t payload)
{
    if (_state.events_to_send >= WATER7_MAX_EVENT_COUNT)
        return;

    for (uint8_t i = 0; i < _state.events_to_send; i++)
        if (_state.events[i][0] == event && _state.events[i][1] == payload)
            return;

    _state.events[_state.events_to_send][0] = event;
    _state.events[_state.events_to_send][1] = payload;
    _state.events_to_send++;
}

void Water7ForceEvent(void)
{
    uint8_t tmpbuf[WATER7_MAX_EVENT_COUNT * 4 + 1];
    uint8_t index = 0;

    if (!_state.events_to_send)
        return;

    tmpbuf[index++] = CMD_EVENT;
    for (uint16_t i = 0; i < _state.events_to_send; i++)
    {
        bigendian_cpy((uint8_t *)&_state.events[i][0], (uint8_t *)&tmpbuf[index], 2);
        index += 2;
        bigendian_cpy((uint8_t *)&_state.events[i][1], (uint8_t *)&tmpbuf[index], 2);
        index += 2;
    }
    _state.events_to_send = 0;
    NBFi_Send(tmpbuf, index);
}

void Water7SendEvent(uint16_t event, uint16_t payload)
{
    uint8_t tmpbuf[10];
    uint8_t index = 0;

    tmpbuf[index++] = CMD_EVENT;
    bigendian_cpy((uint8_t *)&event, (uint8_t *)&tmpbuf[index], 2);
    index += 2;
    bigendian_cpy((uint8_t *)&payload, (uint8_t *)&tmpbuf[index], 2);
    index += 2;

    NBFi_Send(tmpbuf, index);
}

void Water7ForceData(void)
{
    static uint8_t buf[WATER7_REGULAR_BUF_SIZE] = {0};
    uint8_t index = 0;

    for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
    {
        uint32_t need_send = READ_BIT(_waterParams->send_flag, (uint64_t)1 << (i & 0x3F));
        uint8_t len = 0;

        if (need_send)
            len += 5;
        if (len + index > WATER7_REGULAR_BUF_SIZE)
            break;

        if (need_send)
        {
            if (index == 0)
            {
                buf[index++] = CMD_REGULAR | (i & 0x3F);
                uint16_t tmp = Water7PeriodToUINT16(_waterParams->period);
                bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
                index += 2;
            }
            else
                buf[index++] = (i & 0x3F);

            bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
            index += 4;
        }
    }

    if (index == 0)
        buf[index++] = CMD_REGULAR;

    NBFi_Send(buf, index);
}

uint16_t Water7Handler(uint8_t *dataIn, uint16_t length, uint8_t *dataOut)
{
    uint16_t len = 0, addr = 0, cmd = 0, index = 0;
    uint16_t lengthOut = 0;
    uint32_t addr32 = 0, tmp32 = 0;
    int32_t ret = 0;

    _state.period_prev = _waterParams->period;

    /// \todo fix this
    //    if (scheduler_check(&water7fastdl_off_desc))
    //        scheduler_add_task(&water7fastdl_off_desc, &Water7fastdl_off, RELATIVE, SECONDS(WATER7_FASTDL_TIMEOUT));

    if (_get_data)
        _get_data(_state.parameters_array);

    switch (dataIn[0])
    {
    case CMD_MULTIREAD:
        bigendian_cpy((uint8_t *)&dataIn[1], (uint8_t *)&addr, 2);
        bigendian_cpy((uint8_t *)&dataIn[3], (uint8_t *)&len, 2);
        if ((length == 5) && ((5 + len * 4) <= WATER7_CALLBACK_BUF_SIZE) && (addr + len <= WATER7_PAR_LENGTH))
        {
            for (uint8_t i = 0; i < 5; i++)
                dataOut[i] = dataIn[i];
            for (uint8_t i = 0; i < len; i++)
                bigendian_cpy((uint8_t *)&_state.parameters_array[addr + i], (uint8_t *)&dataOut[5 + i * 4], 4);
            lengthOut = 5 + len * 4;
        }
        else
        {
            dataOut[0] = dataIn[0] | CMD_ERR;
            dataOut[1] = 0x02;
            lengthOut = 2;
        }
        break;
    case CMD_MULTIPRESS:
        bigendian_cpy((uint8_t *)&dataIn[1], (uint8_t *)&addr, 2);
        bigendian_cpy((uint8_t *)&dataIn[3], (uint8_t *)&len, 2);
        if ((length == (len * 4 + 5)) && (addr + len <= WATER7_PAR_LENGTH))
        {
            for (uint8_t i = 0; i < len; i++)
            {
                bigendian_cpy((uint8_t *)&dataIn[5 + i * 4], (uint8_t *)&_state.parameters_array[addr + i], 4);
            }
            for (uint8_t i = 0; i < 5; i++)
                dataOut[i] = dataIn[i];
            lengthOut = 5;
        }
        else
        {
            dataOut[0] = dataIn[0] | CMD_ERR;
            dataOut[1] = 0x02;
            lengthOut = 2;
        }
        break;
    case CMD_SINGLEREAD:
        bigendian_cpy((uint8_t *)&dataIn[1], (uint8_t *)&addr, 2);
        if ((length == 3) && (addr < WATER7_PAR_LENGTH))
        {
            for (uint8_t i = 0; i < length; i++)
                dataOut[i] = dataIn[i];
            bigendian_cpy((uint8_t *)&_state.parameters_array[addr], (uint8_t *)&dataOut[3], 4);
            lengthOut = 7;
        }
        else
        {
            dataOut[0] = dataIn[0] | CMD_ERR;
            dataOut[1] = 0x02;
            lengthOut = 2;
        }
        break;
    case CMD_SINGLEPRESS:
        bigendian_cpy((uint8_t *)&dataIn[1], (uint8_t *)&addr, 2);
        if ((length == 7) && (addr < WATER7_PAR_LENGTH))
        {
            for (uint8_t i = 0; i < length; i++)
                dataOut[i] = dataIn[i];
            bigendian_cpy((uint8_t *)&dataIn[3], (uint8_t *)&_state.parameters_array[addr], 4);
            lengthOut = length;
        }
        else
        {
            dataOut[0] = dataIn[0] | CMD_ERR;
            dataOut[1] = 0x02;
            lengthOut = 2;
        }
        break;
    case CMD_RLF:
        if (length < 2)
            break;
        NBFi_Switch_Mode(CRX);
        scheduler_add_task(&water7crx_off_desc, &Water7crx_off, RELATIVE, SECONDS(WATER7_CRX_TIMEOUT));
        switch (dataIn[1])
        {
        case RFL_CMD_WRITE_HEX:
            bigendian_cpy((uint8_t *)&dataIn[2], (uint8_t *)&addr32, 4);
            bigendian_cpy((uint8_t *)&dataIn[6], (uint8_t *)&len, 2);
            if (_rfl)
                _rfl(addr32, len, 0, &dataIn[8], dataIn[1]);
            for (uint8_t i = 0; i < 8; i++)
                dataOut[i] = dataIn[i];
            lengthOut = 8;
            break;
        case RFL_CMD_READ_HEX:
            bigendian_cpy((uint8_t *)&dataIn[2], (uint8_t *)&addr32, 4);
            bigendian_cpy((uint8_t *)&dataIn[6], (uint8_t *)&len, 2);
            if (len > WATER7_CALLBACK_BUF_SIZE - 8)
            {
                dataOut[0] = dataIn[0] | CMD_ERR;
                dataOut[1] = 0x02;
                lengthOut = 2;
            }
            else
            {
                for (uint8_t i = 0; i < 8; i++)
                    dataOut[i] = dataIn[i];
                if (_rfl)
                    _rfl(addr32, len, 0, &dataOut[8], dataIn[1]);
                if (len < 256 - 8)
                {
                    lengthOut = len + 8;
                }
            }
            break;
        case RFL_CMD_WRITE_HEX_INDEX:
            bigendian_cpy((uint8_t *)&dataIn[2], (uint8_t *)&addr32, 4);
            bigendian_cpy((uint8_t *)&dataIn[6], (uint8_t *)&len, 2);
            bigendian_cpy((uint8_t *)&dataIn[8], (uint8_t *)&index, 2);
            if (_rfl)
                ret = _rfl(addr32, len, index, &dataIn[10], dataIn[1]);
            if (ret >= 0)
            {
#if __STDC_WANT_LIB_EXT1__ == 1
                memcpy_s(dataOut, sizeof(dataOut), dataIn, 2);
#else
                memcpy(dataOut, dataIn, 2);
#endif
                bigendian_cpy((uint8_t *)&ret, &dataOut[2], 4);
                lengthOut = 6;
            }
            break;
        case RFL_CMD_GET_CRC:
            if (length != 10)
                break;
            bigendian_cpy((uint8_t *)&dataIn[2], (uint8_t *)&addr32, 4);
            bigendian_cpy((uint8_t *)&dataIn[6], (uint8_t *)&tmp32, 4);
            if (_rfl)
                ret = _rfl(addr32, tmp32, 0, 0, dataIn[1]);
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, 2);
#else
            memcpy(dataOut, dataIn, 2);
#endif
            bigendian_cpy((uint8_t *)&ret, &dataOut[2], 4);
            lengthOut = 6;
            break;
        case RFL_CMD_EXEC_PATCH0:
        case RFL_CMD_EXEC_PATCH1:
        case RFL_CMD_EXEC_PATCH2:
            bigendian_cpy((uint8_t *)&dataIn[2], (uint8_t *)&addr32, 4);
            if (_rfl)
                ret = _rfl(addr32, 0, 0, 0, dataIn[1]);
            bigendian_cpy((uint8_t *)&ret, &dataOut[2], 4);
            lengthOut = 6;
            break;
        case RFL_CMD_CLEAR_INDEX:
        case RFL_CMD_CHECK_UPDATE:
        case RFL_CMD_CLEAR_CACHE:
        case RFL_CMD_CPY_ACTUAL:
        case RFL_CMD_SOFT_RESET:
        case RFL_CMD_MASS_ERASE:
        case RFL_CMD_GET_INDEX:
        case RFL_CMD_GET_VERSION:
        default:
            if (_rfl)
                ret = _rfl(0, 0, 0, 0, dataIn[1]);
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, 2);
#else
            memcpy(dataOut, dataIn, 2);
#endif
            bigendian_cpy((uint8_t *)&ret, &dataOut[2], 4);
            lengthOut = 6;
            break;
        }
    case CMD_CTRL:
        if (length != 7)
            break;
        bigendian_cpy((uint8_t *)&dataIn[1], (uint8_t *)&cmd, 2);
        bigendian_cpy((uint8_t *)&dataIn[3], (uint8_t *)&tmp32, 4);

        switch (cmd)
        {
        case CTRL_SETFASTDL:
            Water7fastdl_on();
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, length);
#else
            memcpy(dataOut, dataIn, length);
#endif
            lengthOut = length;
            break;
        case CTRL_RESETFASTDL:
            scheduler_remove_task(&water7fastdl_off_desc);
            Water7fastdl_off(0);
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, length);
#else
            memcpy(dataOut, dataIn, length);
#endif
            lengthOut = length;
            break;
        case CTRL_RESET:
            if (_rfl)
                _rfl(0, 0, 0, 0, RFL_CMD_SOFT_RESET);
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, length);
#else
            memcpy(dataOut, dataIn, length);
#endif
            lengthOut = length;
            break;
        case CTRL_SAVE:
            if (_save_data)
                _save_data((int32_t *)_state.parameters_array);
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, length);
#else
            memcpy(dataOut, dataIn, length);
#endif
            lengthOut = length;
            break;
        case CTRL_CLEAR_ERRORS:
            MeterClearErrorsMask();
#if __STDC_WANT_LIB_EXT1__ == 1
            memcpy_s(dataOut, sizeof(dataOut), dataIn, length);
#else
            memcpy(dataOut, dataIn, length);
#endif
            lengthOut = length;
            break;
        default:
            dataOut[0] = dataIn[0];
            dataOut[1] = (uint8_t)CTRL_NOCTRL;
            dataOut[2] = (uint8_t)CTRL_NOCTRL;
            dataOut[3] = dataOut[4] = 0;
            dataOut[5] = dataIn[3];
            dataOut[6] = dataIn[4];
            lengthOut = length;
            break;
        }
        break;
    default:
        break;
    }
    if (_state.period_prev != _waterParams->period)
        UpdatePeriodTable();

    if (_set_data)
        _set_data(_state.parameters_array);

    return lengthOut;
}