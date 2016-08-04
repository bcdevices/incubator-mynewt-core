/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <hal/hal_i2c.h>

#include <string.h>
#include <errno.h>

#include <nrf.h>
#include <nrf_drv_twi.h>

struct nrf52_hal_i2c {
    nrf_drv_twi_t nhi_nrf_master;
};

#define NRF52_HAL_I2C_MAX (2)

#if TWI0_ENABLED
struct nrf52_hal_i2c hal_twi_i2c0;
#endif
#if TWI1_ENABLED
struct nrf52_hal_i2c hal_twi_i2c1;
#endif

struct nrf52_hal_i2c *nrf52_hal_i2cs[NRF52_HAL_I2C_MAX] = {
#if TWI0_ENABLED
        &hal_twi_i2c0,
#else
        NULL,
#endif
#if TWI1_ENABLED
        &hal_twi_i2c1
#else
        NULL
#endif
};

#define NRF52_HAL_I2C_RESOLVE(__n, __v) \
    if ((__n) >= NRF52_HAL_I2C_MAX) {   \
        rc = EINVAL;                    \
        goto err;                       \
    }                                   \
    (__v) = nrf52_hal_i2cs[(__n)];     \
    if ((__v) == NULL) {                \
        rc = EINVAL;                    \
        goto err;                       \
    }

int
hal_i2c_init(uint8_t i2c_num, int scl_pin, int sda_pin,
        uint32_t i2c_frequency)
{
    struct nrf52_hal_i2c *i2c;
    nrf_drv_twi_config_t cfg;
    int rc;

    NRF52_HAL_I2C_RESOLVE(i2c_num, i2c);

    cfg.scl = scl_pin;
    cfg.sda = sda_pin;
    switch (i2c_frequency) {
        case 100:
            cfg.frequency = NRF_TWI_FREQ_100K;
            break;
        case 250:
            cfg.frequency = NRF_TWI_FREQ_250K;
            break;
        case 400:
            cfg.frequency = NRF_TWI_FREQ_400K;
            break;
        default:
            rc = EINVAL;
            goto err;
    }

    rc = nrf_drv_twi_init(&i2c->nhi_nrf_master, &cfg, NULL, NULL);
    if (rc != 0) {
        goto err;
    }

    nrf_drv_twi_enable(&i2c->nhi_nrf_master);

    return (0);
err:
    return (rc);
}

int
hal_i2c_master_write(uint8_t i2c_num, struct hal_i2c_master_data *pdata)
{
    struct nrf52_hal_i2c *i2c;
    int rc;

    NRF52_HAL_I2C_RESOLVE(i2c_num, i2c);

    rc = nrf_drv_twi_tx(&i2c->nhi_nrf_master, pdata->address, pdata->buffer,
            pdata->len, true);
    if (rc != 0) {
        goto err;
    }

    return (0);
err:
    return (rc);
}

int
hal_i2c_master_read(uint8_t i2c_num, struct hal_i2c_master_data *pdata)
{
    struct nrf52_hal_i2c *i2c;
    int rc;

    NRF52_HAL_I2C_RESOLVE(i2c_num, i2c);

    rc = nrf_drv_twi_rx(&i2c->nhi_nrf_master, pdata->address, pdata->buffer,
            pdata->len);
    if (rc != 0) {
        goto err;
    }

    return (0);
err:
    return (rc);
}

int
hal_i2c_master_begin(uint8_t i2c_num)
{
    return (0);
}

int
hal_i2c_master_end(uint8_t i2c_num)
{
    struct nrf52_hal_i2c *i2c;
    int rc;

    NRF52_HAL_I2C_RESOLVE(i2c_num, i2c);

    /**
     * XXX: Nordic doesn't provide a function for generating the stop
     * character, and I don't trust issue'ing nrf_drv_twi_tx() with NULL
     * data, and 0 length, so directly use the stop task in HAL driver.
     * This is subject to break as NRF SDK is updated, as reg.p_twi is
     * private, however, seems like it will be reasonably easy to spot.
     * Famous last words.
     */
    nrf_twi_task_trigger(i2c->nhi_nrf_master.reg.p_twi, NRF_TWI_TASK_STOP);

    return (0);
err:
    return (rc);
}

int
hal_i2c_master_probe(uint8_t i2c_num, uint8_t address)
{
    struct nrf52_hal_i2c *i2c;
    uint8_t buf;
    int rc;

    NRF52_HAL_I2C_RESOLVE(i2c_num, i2c);

    rc = nrf_drv_twi_rx(&i2c->nhi_nrf_master, address, &buf, 1);
    if (rc != 0) {
        goto err;
    }

    return (0);
err:
    return (rc);
}