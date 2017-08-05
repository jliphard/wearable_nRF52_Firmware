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

#ifndef BLE_MA_H__
#define BLE_MA_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Mentaid Service event type. */
typedef enum
{
    BLE_MA_EVT_NOTIFICATION_ENABLED,                   /**< Mentaid value notification enabled event. */
    BLE_MA_EVT_NOTIFICATION_DISABLED                   /**< Mentaid value notification disabled event. */
} ble_ma_evt_type_t;

/**@brief Mentaid Service event. */
typedef struct
{
    ble_ma_evt_type_t evt_type;                        /**< Type of event. */
} ble_ma_evt_t;

// Forward declaration of the ble_ma_t type.
typedef struct ble_ma_s ble_ma_t;

/**@brief Mentaid Service event handler type. */
typedef void (*ble_ma_evt_handler_t) (ble_ma_t * p_ma, ble_ma_evt_t * p_evt);

/**@brief Mentaid Service init structure. This contains all options and data needed for
 *        initialization of the service. */
typedef struct
{
    ble_ma_evt_handler_t         evt_handler;                                          /**< Event handler to be called for handling events in the Mentaid Service. */
    bool                         is_sensor_contact_supported;                          /**< Determines if sensor contact detection is to be supported. */
    uint8_t *                    p_body_sensor_location;                               /**< If not NULL, initial value of the Body Sensor Location characteristic. */
    ble_srv_cccd_security_mode_t ma_attr_md;                                           /**< Initial security level for heart rate service measurement attribute */
    ble_srv_security_mode_t      bsl_attr_md;                                          /**< Initial security level for body sensor location attribute */
} ble_ma_init_t;

/**@brief Mentaid Service structure. This contains various status information for the service. */
struct ble_ma_s
{
    ble_ma_evt_handler_t         evt_handler;                                          /**< Event handler to be called for handling events in the Mentaid Service. */
    bool                         is_expended_energy_supported;                         /**< TRUE if Expended Energy measurement is supported. */
    bool                         is_sensor_contact_supported;                          /**< TRUE if sensor contact detection is supported. */
    uint16_t                     service_handle;                                       /**< Handle of Mentaid Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t     ma_handles;                                           /**< Handles related to the Mentaid Measurement characteristic. */
    ble_gatts_char_handles_t     bsl_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     hrcp_handles;                                         /**< Handles related to the Mentaid Control Point characteristic. */
    uint16_t                     conn_handle;                                          /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    bool                         is_sensor_contact_detected;                           /**< TRUE if sensor contact has been detected. */
    uint8_t                      max_ma_len;                                           /**< Current maximum MA measurement length, adjusted according to the current ATT MTU. */
};

/**@brief Function for initializing the Mentaid Service.
 *
 * @param[out]  p_hrs       Mentaid structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_ma_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_ma_init(ble_ma_t * p_ma, const ble_ma_init_t * p_ma_init);


/**@brief Function for handling the GATT module's events.
 *
 * @details Handles all events from the GATT module of interest to the Mentaid Service.
 *
 * @param[in]   p_hrs       Mentaid Service structure.
 * @param[in]   p_gatt_evt  Event received from the GATT module.
 */
void ble_ma_on_gatt_evt(ble_ma_t * p_ma, const nrf_ble_gatt_evt_t * p_gatt_evt);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Mentaid Service.
 *
 * @param[in]   p_hrs      Mentaid Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_ma_on_ble_evt(ble_ma_t * p_hrs, ble_evt_t * p_ble_evt);

/**@brief Function for sending heart rate measurement if notification has been enabled.
 */

uint8_t  ma_encode(          ble_ma_t * p_ma, uint16_t hr, uint8_t b8, uint8_t p1, uint8_t t1, uint8_t h1, 
        uint16_t l_white, int16_t ax, int16_t ay, int16_t az, uint16_t storage, uint8_t * p_encoded_buffer);

uint32_t ma_measurement_send(ble_ma_t * p_ma, uint16_t hr, uint8_t b8, uint8_t p1, uint8_t t1, uint8_t h1, 
        uint16_t l_white, int16_t ax, int16_t ay, int16_t az, uint16_t storage);

uint32_t ma_measurement_send_16(ble_ma_t * p_ma, uint8_t * data );
    
/**@brief Function for setting the state of the Sensor Contact Supported bit.
 *
 * @param[in]   p_ma                         Mentaid Service structure.
 * @param[in]   is_sensor_contact_supported  New state of the Sensor Contact Supported bit.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_ma_sensor_contact_supported_set(ble_ma_t * p_ma, bool is_sensor_contact_supported);

/**@brief Function for setting the state of the Sensor Contact Detected bit.
 *
 * @param[in]   p_hrs                        Mentaid Service structure.
 * @param[in]   is_sensor_contact_detected   TRUE if sensor contact is detected, FALSE otherwise.
 */
void ble_ma_sensor_contact_detected_update(ble_ma_t * p_ma, bool is_sensor_contact_detected);

/**@brief Function for setting the Body Sensor Location.
 *
 * @details Sets a new value of the Body Sensor Location characteristic. The new value will be sent
 *          to the client the next time the client reads the Body Sensor Location characteristic.
 *
 * @param[in]   p_ma                  Mentaid Service structure.
 * @param[in]   body_sensor_location  New Body Sensor Location.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_ma_body_sensor_location_set(ble_ma_t * p_ma, uint8_t body_sensor_location);
uint32_t ble_ma_body_sensor_location_get(ble_ma_t * p_ma);


#ifdef __cplusplus
}
#endif

#endif // BLE_MA_H__

/** @} */
