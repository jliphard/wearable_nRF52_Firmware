/* Copyright (c) 2017, Stanford University
 * All rights reserved.
 * 
 * The point of contact for the MENTAID wearables dev team is 
 * Jan Liphardt (jan.liphardt@stanford.edu)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of STANFORD UNIVERSITY nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY "AS IS" AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY OR ITS CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Parts of this software, primarily the basic BLE control code, were derived or 
 * directly copied from the Nordic reference implementations available in their SDK.  
 * For those sections, the following license applies:
 * 
 * Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sdk_common.h"
#include "ble_ma.h"
#include <string.h>
#include "ble_l2cap.h"
#include "ble_srv_common.h"

#define OPCODE_LENGTH 1                                                              /**< Length of opcode inside Mentaid Measurement packet. */
#define HANDLE_LENGTH 2                                                              /**< Length of handle inside Mentaid Measurement packet. */
#define MAX_MA_LEN (NRF_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)       /**< Maximum size of a transmitted Mentaid Measurement.  */
#define INITIAL_VALUE_MA 0                                                           /**< Initial Mentaid Measurement value. */

#define BLE_UUID_MENTAID_SERVICE           0x180D //use HRS for now... change later

#define BLE_UUID_MENTAID_MEASUREMENT_CHAR  0x2A4A
#define BLE_UUID_MENTAID_COMMAND_CHAR      0x2A4B
#define BLE_UUID_MENTAID_STATUS_CHAR       0x2A4C

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_ma       Mentaid Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
static void on_connect(ble_ma_t * p_ma, ble_evt_t * p_ble_evt)
{
    p_ma->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_ma       Mentaid Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
static void on_disconnect(ble_ma_t * p_ma, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_ma->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling write events to the Mentaid Measurement characteristic.
 *
 * @param[in]   p_ma         Mentaid Service structure.
 * @param[in]   p_evt_write  Write event received from the BLE stack.
 */
static void on_ma_cccd_write(ble_ma_t * p_ma, ble_gatts_evt_write_t * p_evt_write)
{
    if (p_evt_write->len == 2)
    {
        // CCCD written, update notification state
        if (p_ma->evt_handler != NULL)
        {
            ble_ma_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_MA_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_MA_EVT_NOTIFICATION_DISABLED;
            }

            p_ma->evt_handler(p_ma, &evt);
        }
    }
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_ma       Mentaid Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_ma_t * p_ma, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_ma->ma_handles.cccd_handle)
    {
        on_ma_cccd_write(p_ma, p_evt_write);
    }
}

void ble_ma_on_ble_evt(ble_ma_t * p_ma, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_ma, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_ma, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_ma, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for encoding a Mentaid Measurement.
 *
 * @param[in]   p_ma              Mentaid Service structure.
 * @param[in]   time etc          Measurements to be encoded.
 * @param[out]  p_encoded_buffer  Buffer where the encoded data will be written.
 *
 * @return      Size of encoded data.
 */
uint8_t ma_encode(ble_ma_t * p_ma, uint16_t time, uint8_t b8, uint8_t p1, uint8_t t1, 
        uint8_t h1, uint16_t l_white, int16_t ax, int16_t ay, int16_t az, uint16_t storage, 
        uint8_t * p_encoded_buffer)
{
    uint8_t flags = 0;
    uint8_t len   = 1;
    
    len += uint16_encode(time, &p_encoded_buffer[len]);
    
    p_encoded_buffer[3] = b8;
    p_encoded_buffer[4] = p1;
    p_encoded_buffer[5] = t1;
    p_encoded_buffer[6] = h1;
    
    len = 7;

    len += uint16_encode(l_white, &p_encoded_buffer[len]);
    len += uint16_encode(ax,      &p_encoded_buffer[len]);
    len += uint16_encode(ay,      &p_encoded_buffer[len]);
    len += uint16_encode(az,      &p_encoded_buffer[len]);
    len += uint16_encode(storage, &p_encoded_buffer[len]);
    
    // Add flags
    p_encoded_buffer[0] = flags;
    
    return len;
}

/**@brief Function for adding the Mentaid Measurement characteristic.
 *
 * @param[in]   p_ma        Mentaid Service structure.
 * @param[in]   p_ma_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t mentaid_measurement_char_add(ble_ma_t * p_ma, const ble_ma_init_t * p_ma_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    
    uint8_t             encoded_initial_ma[MAX_MA_LEN];
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    cccd_md.write_perm = p_ma_init->ma_attr_md.cccd_write_perm;
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    
    static char user_desc[] = "Mentaid Data";
    char_md.p_char_user_desc  = (uint8_t *) user_desc;
    char_md.char_user_desc_size = strlen(user_desc);
    char_md.char_user_desc_max_size = strlen(user_desc);
    
    //char_md.p_char_user_desc  = NULL;
    
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_MENTAID_MEASUREMENT_CHAR);

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ma_init->ma_attr_md.read_perm;
    attr_md.write_perm = p_ma_init->ma_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = MAX_MA_LEN;
    attr_char_value.p_value   = encoded_initial_ma;

    return sd_ble_gatts_characteristic_add(p_ma->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_ma->ma_handles);
}

static uint32_t mentaid_status_char_add(ble_ma_t * p_ma, const ble_ma_init_t * p_ma_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    cccd_md.write_perm = p_ma_init->ma_attr_md.cccd_write_perm;
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    
    static char user_desc[] = "Mentaid Status Notify";
    char_md.p_char_user_desc  = (uint8_t *) user_desc;
    char_md.char_user_desc_size = strlen(user_desc);
    char_md.char_user_desc_max_size = strlen(user_desc);
        
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_MENTAID_STATUS_CHAR);

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ma_init->status_attr_md.read_perm;
    attr_md.write_perm = p_ma_init->status_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = 1;
    attr_char_value.p_value   = 12;

    return sd_ble_gatts_characteristic_add(p_ma->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_ma->status_handles);
}

/**@brief Function for adding the Command characteristic.
 *
 * @param[in]   p_ma        Mentaid Service structure.
 * @param[in]   p_ma_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t mentaid_command_char_add(ble_ma_t * p_ma, const ble_ma_init_t * p_ma_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    //phone does not need to be able to read this
    //char_md.char_props.read  = 1;
    
    //UNSAFE UNSAFE UNSAFE CHECK CHECK CHECK
    //the phone can change this number
    char_md.char_props.write         = 1;
    char_md.char_props.write_wo_resp = 0;
    
    static char user_desc[] = "Mentaid Command";
    char_md.p_char_user_desc  = (uint8_t *) user_desc;
    char_md.char_user_desc_size = strlen(user_desc);
    char_md.char_user_desc_max_size = strlen(user_desc);

    //char_md.p_char_user_desc = NULL;
    
    char_md.p_char_pf        = NULL;
    
    char_md.p_user_desc_md   = NULL;
    
    char_md.p_cccd_md        = NULL;
    char_md.p_sccd_md        = NULL;

    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_MENTAID_COMMAND_CHAR);

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_ma_init->command_attr_md.read_perm;
    attr_md.write_perm = p_ma_init->command_attr_md.write_perm;
    
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = 1;
    
    //set better initial value
    //uint8_t encoded_sc[1];
    //encoded_sc[0] = 0;
    attr_char_value.p_value = 0;

    return sd_ble_gatts_characteristic_add(p_ma->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_ma->command_handles);
}


uint32_t ble_ma_init(ble_ma_t * p_ma, const ble_ma_init_t * p_ma_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_ma->evt_handler                 = p_ma_init->evt_handler;
    
    p_ma->current_status              = p_ma_init->current_status;
        
    p_ma->conn_handle                 = BLE_CONN_HANDLE_INVALID;
    
    p_ma->max_ma_len                  = MAX_MA_LEN;
    
    // Add service
    BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_MENTAID_SERVICE);
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_ma->service_handle);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add measurement characteristic
    err_code = mentaid_measurement_char_add(p_ma, p_ma_init);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Add command characteristic
    err_code = mentaid_command_char_add(p_ma, p_ma_init);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Add status characteristic
    err_code = mentaid_status_char_add(p_ma, p_ma_init);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;
}

uint32_t ma_measurement_send_16( ble_ma_t * p_ma, uint8_t * data )
{
    uint32_t err_code;
    
    // Send value if connected and notifying
    if (p_ma->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t  encoded_ma[MAX_MA_LEN];
        uint16_t len = 17;
        
        uint16_t hvx_len;
        ble_gatts_hvx_params_t hvx_params;
        
        //should be 17 bytes long
        encoded_ma[0] = 0; //no flags
                
        for( uint8_t i = 1; i < len; i++ )
        {
            encoded_ma[i] = data[i-1];
        }
         
        hvx_len = len;
        
        memset(&hvx_params, 0, sizeof(hvx_params));
        
        hvx_params.handle = p_ma->ma_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = encoded_ma; //this is the data packet
        
        err_code = sd_ble_gatts_hvx(p_ma->conn_handle, &hvx_params);
        
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    
    return err_code;
}

uint32_t ma_measurement_send(ble_ma_t * p_ma, uint16_t time, uint8_t b8, uint8_t p1, uint8_t t1, 
        uint8_t h1, uint16_t l_white, int16_t ax, int16_t ay, int16_t az, uint16_t storage)
{
    uint32_t err_code;
    
    // Send value if connected and notifying
    if (p_ma->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t                encoded_ma[MAX_MA_LEN];
        uint16_t               len;
        uint16_t               hvx_len;
        ble_gatts_hvx_params_t hvx_params;
        
        len = ma_encode(p_ma, time, b8, p1, t1, h1, l_white, ax, ay, az, storage, encoded_ma);
        
        hvx_len = len;
        
        memset(&hvx_params, 0, sizeof(hvx_params));
        
        hvx_params.handle = p_ma->ma_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = encoded_ma; //this is the data packet
        
        err_code = sd_ble_gatts_hvx(p_ma->conn_handle, &hvx_params);
        
        if ((err_code == NRF_SUCCESS) && (hvx_len != len))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    
    return err_code;
}

uint32_t ma_status_send( ble_ma_t * p_ma, uint8_t status )
{
    uint32_t err_code;
    
    // Send value if connected and notifying
    if (p_ma->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint8_t  encoded_ma[1];
        encoded_ma[0] = status;
        
        uint16_t hvx_len;
        ble_gatts_hvx_params_t hvx_params;
         
        hvx_len = 1;
        
        memset(&hvx_params, 0, sizeof(hvx_params));
        
        hvx_params.handle = p_ma->status_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = encoded_ma; //encoded_ma; //this is the data packet
        
        err_code = sd_ble_gatts_hvx(p_ma->conn_handle, &hvx_params);
        
        if ((err_code == NRF_SUCCESS) && (hvx_len != 1))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    
    return err_code;
}

void ble_ma_on_gatt_evt(ble_ma_t * p_ma, nrf_ble_gatt_evt_t const * p_gatt_evt)
{
    if (    (p_ma->conn_handle == p_gatt_evt->conn_handle)
        &&  (p_gatt_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        p_ma->max_ma_len = p_gatt_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
    }
}

