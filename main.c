/**
 This file contains the source code for a sample application using the Heart Rate service
 * (and also Battery and Device Information services). This application uses the
 * @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"

#include "app_error.h"

#include "math.h"

#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_hrs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"

#include "boards.h"
#include "softdevice_handler.h"

#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"

#include "bsp.h"
#include "bsp_btn_ble.h"

#include "nrf_ble_gatt.h"

#include "peer_manager.h"

#include "fds.h"
#include "fstorage.h"

#define NRF_LOG_MODULE_NAME "APP"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "I2C.h"        //everything on the I2C bus
#include "BME280.h"     //pressure, temp, humidity
#include "BMA280.h"     //accelerometer
#include "ADC.h"        //used for battery reading
#include "SPIFlash.h"   //used for storage
#include "VEML6040.h"    //light levels

#include "SEGGER_RTT.h"

#define DEVICE_NAME                      "MENTAID"                                 /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "Stanford University"                     /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 1600                                      //The advertising interval (in units of 0.625 ms
                                                                                   // 3200 = 2 seconds
#define APP_ADV_TIMEOUT_IN_SECONDS       2                                         // The advertizing timeout in units of seconds. Used to be 180 seconds
                                                                                   // 0 = infinite advertizing                                                                                        
#define CONN_CFG_TAG                     1                                          /**< A tag that refers to the BLE stack configuration we set with @ref sd_ble_cfg_set. Default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */

#define MEAS_INTERVAL                    APP_TIMER_TICKS(1000)

#define SENSOR_CONTACT_DETECTED_INTERVAL APP_TIMER_TICKS(5000)                      /**< Sensor Contact Detected toggle interval (ticks). */

#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(400, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.40 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(650, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                      /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000)                     /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                   0                                          /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS               0                                          /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED        BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2       /**< Reply when unsupported features are requested. */

static uint16_t  m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

static ble_bas_t m_bas;                                   /**< Structure used to identify the battery service. */
static ble_hrs_t m_hrs;                                   /**< Structure used to identify the heart rate service. */

static nrf_ble_gatt_t m_gatt;                             /**< Structure for gatt module*/

//this uses the RTC1
APP_TIMER_DEF(m_timer_id);

static ble_uuid_t m_adv_uuids[] = 
{
    {BLE_UUID_HEART_RATE_SERVICE,         BLE_UUID_TYPE_BLE},
    {BLE_UUID_BATTERY_SERVICE,            BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
}; /* Universally unique service identifiers. */

static uint16_t heartbeat16 = 0;

static int32_t  resultPTH[3]; //PTH from sensor 1
static int16_t  resultVME[4];
static int16_t  resultBMA[3];

static uint8_t  battery_level8 = 0;
static uint8_t fb[16];

uint8_t BMP280P8;
uint8_t BMP280T8;
uint8_t BMP280H8;

uint8_t record_counter = 0;
static uint8_t  flash_page_buffer[16*16];
static uint16_t GLOB_datastart = 0;
uint8_t batt_cycle = 0;

ret_code_t err_code;

//LEDS
//these are all defined in d52_BA.h and boards.c

void flash_blue( void ) {
    bsp_board_led_on(LED_BLUE);
    nrf_delay_ms(25);
    bsp_board_led_off(LED_BLUE);
}

void flash_green( void ) {
    bsp_board_led_on(LED_GREEN);
    nrf_delay_ms(25);
    bsp_board_led_off(LED_GREEN);
}

void flash_red( void ) {
    bsp_board_led_on(LED_RED);
    nrf_delay_ms(25);
    bsp_board_led_off(LED_RED);
}

void bsp_event_handler(bsp_event_t event);

static void leds_init( void )
{
    err_code = bsp_init(BSP_INIT_LED, bsp_event_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t err_code;
        err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling File Data Storage events.
 *
 * @param[in] p_evt  Peer Manager event.
 * @param[in] cmd
 */
static void fds_evt_handler(fds_evt_t const * const p_evt)
{
    if (p_evt->id == FDS_EVT_GC)
    {
        //NRF_LOG_DEBUG("GC completed\n");
    }
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
        {
            //NRF_LOG_INFO("Connected to a previously bonded device.\r\n");
        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            //NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.\r\n",
            //             ble_conn_state_role(p_evt->conn_handle),
            //             p_evt->conn_handle,
            //             p_evt->params.conn_sec_succeeded.procedure);
        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
        } break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {
            //NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED\r\n");
            advertising_start(false);
        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
//static void update_slow(void);
//static void update_fast(void);

/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */

static void timeout_handler(void * p_context);


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */

// General application timer settings.
//#define APP_TIMER_PRESCALER             15    // Value of the RTC1 PRESCALER register.
//#define APP_TIMER_OP_QUEUE_SIZE         4     // Size of timer operation queues.
    
static void RTC1_timer_init(void)
{
    //APP_TIMER_BUF_SIZE
    // Initialize timer module.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create timers.
    err_code = app_timer_create(&m_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                timeout_handler);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for starting application timers.
 */
static void RTC1_timer_start(void)
{
    //Start application timers.
    err_code = app_timer_start(m_timer_id, MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting application timers.
 */
static void RTC1_timer_stop(void)
{
    //Stop application timers.
    err_code = app_timer_stop(m_timer_id);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
    {
        //NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.\r\n",
        //             p_evt->conn_handle,
        //             p_evt->params.att_mtu_effective);
    }

    ble_hrs_on_gatt_evt(&m_hrs, p_evt);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void ble_services_init(void)
{
    ble_hrs_init_t hrs_init;
    ble_bas_init_t bas_init;
    ble_dis_init_t dis_init;
    uint8_t        body_sensor_location;

    // Initialize Heart Rate Service.
    body_sensor_location = BLE_HRS_BODY_SENSOR_LOCATION_WRIST;

    memset(&hrs_init, 0, sizeof(hrs_init));

    hrs_init.evt_handler                 = NULL;
    hrs_init.is_sensor_contact_supported = true;
    hrs_init.p_body_sensor_location      = &body_sensor_location;

    // Here the sec level for the Heart Rate Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_hrm_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.write_perm);
    
    // Body sensor location???
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_bsl_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_bsl_attr_md.write_perm);

    err_code = ble_hrs_init(&m_hrs, &hrs_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    // Here the sec level for the Battery Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = m_hrs.hrm_handles.cccd_handle;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.\r\n");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            //keep going!!!!!!! JTL
            //sleep_mode_enter();
            break;

        default:
            break;
    }
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.\r\n");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break; // BLE_GAP_EVT_CONNECTED

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected, reason %d.\r\n",
                          p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTC_EVT_TIMEOUT

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_TIMEOUT

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break; // BLE_EVT_USER_MEM_REQUEST

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST


        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_state_on_ble_evt(p_ble_evt);
    pm_on_ble_evt(p_ble_evt);
    ble_hrs_on_ble_evt(&m_hrs, p_ble_evt);
    ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    nrf_ble_gatt_on_ble_evt(&m_gatt, p_ble_evt);
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    // Dispatch the system event to the fstorage module, where it will be
    // dispatched to the Flash Data Storage (FDS) module.
    fs_sys_event_handler(sys_evt);

    // Dispatch to the Advertising module last, since it will check if there are any
    // pending flash operations in fstorage. Let fstorage process system events first,
    // so that it can report correctly to the Advertising module.
    ble_advertising_on_sys_evt(sys_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */

static void lfclk_request(void)
{
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

static void ble_stack_init(void)
{
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = softdevice_app_ram_start_get(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Overwrite some of the default configurations for the BLE stack.
    ble_cfg_t ble_cfg;

    // Configure number of custom UUIDS.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 0;
    err_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure the maximum number of connections.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gap_cfg.role_count_cfg.periph_role_count  = BLE_GAP_ROLE_COUNT_PERIPH_DEFAULT;
    ble_cfg.gap_cfg.role_count_cfg.central_role_count = 0;
    ble_cfg.gap_cfg.role_count_cfg.central_sec_count  = 0;
    err_code = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure the maximum ATT MTU.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                 = CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = NRF_BLE_GATT_MAX_MTU_SIZE;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Configure the maximum event length.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                     = CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = 320;
    ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count   = BLE_GAP_CONN_COUNT_DEFAULT;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = softdevice_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE) {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID) {
                err_code = ble_advertising_restart_without_whitelist();
                if (err_code != NRF_ERROR_INVALID_STATE) {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = fds_register(fds_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ble_advdata_t          advdata;
    ble_adv_modes_config_t options;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    memset(&options, 0, sizeof(options));
    options.ble_adv_fast_enabled   = true;
    options.ble_adv_fast_interval  = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout   = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(CONN_CFG_TAG);
}

/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

//=====================================
void FLASH_Write_Record( uint8_t wp[] )
{
  
  if( record_counter < 16 )
  {
    //keep adding to buffer...
    flash_page_buffer[record_counter * 16 +  0] = wp[ 0];
    flash_page_buffer[record_counter * 16 +  1] = wp[ 1];  
    flash_page_buffer[record_counter * 16 +  2] = wp[ 2];  
    flash_page_buffer[record_counter * 16 +  3] = wp[ 3];  
    flash_page_buffer[record_counter * 16 +  4] = wp[ 4];  
    flash_page_buffer[record_counter * 16 +  5] = wp[ 5];  
    flash_page_buffer[record_counter * 16 +  6] = wp[ 6];  
    flash_page_buffer[record_counter * 16 +  7] = wp[ 7];  
    flash_page_buffer[record_counter * 16 +  8] = wp[ 8];  
    flash_page_buffer[record_counter * 16 +  9] = wp[ 9]; 
    flash_page_buffer[record_counter * 16 + 10] = wp[10];  
    flash_page_buffer[record_counter * 16 + 11] = wp[11];  
    flash_page_buffer[record_counter * 16 + 12] = wp[12];  
    flash_page_buffer[record_counter * 16 + 13] = wp[13];  
    flash_page_buffer[record_counter * 16 + 14] = wp[14];
    flash_page_buffer[record_counter * 16 + 15] = wp[15];
            
    record_counter++;   
  }
  
  if (record_counter >= 16 )
  {
        flash_green();
        
        SEGGER_RTT_printf(0, "TH:%d VB:%d DP:%d\r\n", heartbeat16, battery_level8, GLOB_datastart);
        //SEGGER_RTT_printf(0, "Datapage:%d\r\n", GLOB_datastart);
        
        //flush to memory.... 
        FLASH_Page_Write( GLOB_datastart, flash_page_buffer );  
     
        //advance counter  
        GLOB_datastart++;
    
        //clear the buffer  
        memset(flash_page_buffer, 0, sizeof(flash_page_buffer));
        
        //and reset the counter
        record_counter = 0; 
    }
  
};

void add_to_flash( uint16_t counter, uint8_t batt,               \
        uint8_t pressure, uint8_t temperature, uint8_t humidity, \
        uint16_t l_white,                                        \
        int16_t ax, int16_t ay, int16_t az )
{ 

    memset(fb, 0, sizeof(fb));
    
    //timestamp
    fb[ 0] = (uint8_t) ( counter      & 0xff);
    fb[ 1] = (uint8_t) ( counter >> 8 & 0xff);
    
    fb[ 2] = batt;
    
    fb[ 3] = pressure;
    fb[ 4] = temperature;
    fb[ 5] = humidity;
    
    fb[ 6] = (uint8_t) ( l_white      & 0xff ); 
    fb[ 7] = (uint8_t) ( l_white >> 8 & 0xff ); 
    
    fb[ 8] = (uint8_t) ( ax       & 0xff); 
    fb[ 9] = (uint8_t) ( ax >> 8  & 0xff); 
    
    fb[10] = (uint8_t) ( ay       & 0xff); 
    fb[11] = (uint8_t) ( ay >> 8  & 0xff); 
    
    fb[12] = (uint8_t) ( az       & 0xff);
    fb[13] = (uint8_t) ( az >> 8  & 0xff); 
    
    fb[14] = 0;
    fb[15] = 0; 

    FLASH_Write_Record( fb );    
}

static void update_battery(void)
{            
    
    //Battery ADC read
    //this will update Current_VBATT() via callback
    nrf_drv_saadc_sample(); 
    
    //convert to percent from voltage - 3.8V is full
    //battery_level8  = (uint8_t )(100.00 * (float)Current_VBATT()/380.00);
    
    //for debugging let's record the actual voltage
    battery_level8 = (uint8_t)(Current_VBATT() - 300);
    //SEGGER_RTT_printf(0, "Battery: %d\n", battery_level8);
    
    /*
    //bluetooth update
    
    ret_code_t err_code = NRF_SUCCESS;  
    err_code = ble_bas_battery_level_update(&m_bas, battery_level);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {
        APP_ERROR_HANDLER(err_code);
    } 
    */
}

static void update_fast(void)
{

    if ( batt_cycle > 3 ) 
    {
        update_battery();
        batt_cycle = 0;
    } else {
        batt_cycle++;
    };
    
    heartbeat16++; //this is the number of sampling cycles. 
        
    //Pressure, Temp, and Humidity
    BME280_Get_Data(resultPTH);
    
    BMP280P8 = (uint8_t)( (resultPTH[0]/  10.00) - 10000.0 ); //need to add 10000 to the pressure and then divide by 10. 
    BMP280T8 = (uint8_t)( (resultPTH[1]/  10.00) - 200.0   ); //need to add 200 to the temp and then divide by 10
    BMP280H8 = (uint8_t)( (resultPTH[2]/1000.00)           );
    
    //light intensity
    VEML6040_Get_Data(resultVME);
    //the different colors are largely useless - just get white
    
    //acceleration
    BMA280_Get_Data(resultBMA);
    
    /*
    float ax = (float)resultBMA[0];
    float ay = (float)resultBMA[1];
    float az = (float)resultBMA[2];//take off gravity
    
    float accelT = (ax*ax) + (ay*ay) + (az*az);
    
    accelT = sqrt(accelT);
    */
    
    //SEGGER_RTT_printf(0, "x-axis offset = %d mg\n", (int16_t)(100.0f*(float)offsetX*FCres/256.0f));
    //SEGGER_RTT_printf(0, "accel:%d\n", (uint16_t)accelT);
    
    add_to_flash( heartbeat16, battery_level8,  \
                  BMP280P8, BMP280T8, BMP280H8, \
                  resultVME[3],                 \
                  resultBMA[0], resultBMA[1], resultBMA[2] );    
    
    /*
    ret_code_t err_code = NRF_SUCCESS;
    err_code = ble_hrs_heart_rate_measurement_send_3(&m_hrs, BMP280P, BMP280T, BMP280H);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
    */
}

static void timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //SEGGER_RTT_WriteString(0, "TH.\n"); 
    flash_green();
    update_fast();
}

int main(void)
{
    SEGGER_RTT_WriteString(0, "\n\n***********\n");
    
    RTC1_timer_init();
    leds_init();
    
    //bool erase_bonds;
    //buttons_leds_init(&erase_bonds);
    
    if( 1 == 2 ) {
        lfclk_request(); // Request LF clock - needed when softdevice is off
    } else {
        ble_stack_init(); //fire up the softdevice
    }
        
    //gap_params_init();
    //gatt_init();
    //advertising_init();
    //ble_services_init();
    //conn_params_init();
    //peer_manager_init();
    
    //ADC subsystem 
    ADC_init();
        
    //I2C bus. 
    I2C_init();
    
    //BME280 init. 
    BME280_Turn_On();

    nrf_delay_ms(500);
    
    VEML6040_Turn_On();
    
    nrf_delay_ms(500);
    
    BMA280_Turn_On_Fast();
    
    nrf_delay_ms(500);
    
    BMA280_Calibrate();
       
    nrf_delay_ms(500);
    
    //save energy - do not need to sample that quickly
    BMA280_Turn_On_Slow();
    
    //Flash memory
    FLASH_Init();
    
    FLASH_Print_ID();
    
    nrf_delay_ms(500);
    
    //FLASH_Reset();    
    FLASH_Erase();
    
    GLOB_datastart = FLASH_Get_First_Available_Location();
    SEGGER_RTT_printf(0, "First empty page: %d\r\n", GLOB_datastart);
    
    flash_red();
    
    /*
    uint16_t j = 0;
 
    for(j = 0; j < 100; j++) 
    {  
        nrf_delay_ms(100);
        FLASH_Page_Read(j);
    };
    */
       
    /* The four basic functions are to:
     * (1) erase
     * (2) record
     * (3) record + stream data to phone. 
     * (4) get data from phone - especially position and time
     * 
     * We could advertize for two mins after turn on. 
     * If no connection -> record
     * If connection, then would synchronize time, and get occasional GPS packets.
     * Also, would send data to phone app. 
     * In the same app, could chose to delete data
     * Could also flush data to pubnub?
    */ 
    
    RTC1_timer_start();
    //advertising_start(erase_bonds);
     
    while( 1 ) 
    {
        __WFE();
    };
     
    /*
    for (;;)
        {
            if (NRF_LOG_PROCESS() == false)
            {
                power_manage();
            }
        }
    */
}
