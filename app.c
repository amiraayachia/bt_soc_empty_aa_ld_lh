/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "temperature.h"
#include "gatt_db.h"

#define TEMPERATURE_TIMER_SIGNAL (1<<0)


sl_sleeptimer_timer_handle_t my_handle;

int step=0;
uint8_t connection;
uint16_t characteristic;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log_info("%s\n", __FUNCTION__);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void callback(sl_sleeptimer_timer_handle_t *handle, void *data){
  handle = handle;
  data = data;
  sl_bt_external_signal(TEMPERATURE_TIMER_SIGNAL);
  // app_log_info("%s: Timer Step %d \n", __FUNCTION__, ++step);

}

void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("%s: connection_opened!\n", __FUNCTION__);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("%s: connection_closed!\n", __FUNCTION__);
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
case sl_bt_evt_gatt_server_characteristic_status_id:
  switch(evt->data.evt_gatt_server_characteristic_status.characteristic){
  case gattdb_temperature:
    switch(evt->data.evt_gatt_server_characteristic_status.status_flags){
      case sl_bt_gatt_server_client_config :
        switch(evt->data.evt_gatt_server_characteristic_status.client_config_flags){
            case sl_bt_gatt_server_disable :
            app_log_info("%s: Notify OFF\n", __FUNCTION__);
            sl_sleeptimer_stop_timer(&my_handle);
            break;
            case sl_bt_gatt_server_notification :
            app_log_info("%s: Notify ON\n", __FUNCTION__);
            connection = evt ->data.evt_gatt_server_characteristic_status.connection;
            characteristic = evt ->data.evt_gatt_server_characteristic_status.characteristic;
            sl_sleeptimer_start_periodic_timer_ms(&my_handle,
                                                              1000,
                                                              callback,
                                                              NULL,
                                                              0,
                                                              0);
            break;
            default :
            app_log_info("%s: default\n", __FUNCTION__);
            break;
        }
        break;
      break;
      default:
      app_log_info("%s: default\n", __FUNCTION__);
      break;

    }
    break;
    default:
    app_log_info("%s: default\n", __FUNCTION__);
    break;
  }
  break;
  break;

  case sl_bt_evt_system_external_signal_id:
    switch(evt->data.evt_system_external_signal.extsignals){
      case TEMPERATURE_TIMER_SIGNAL:
        int temp_send = temp_recup() * 10;
        sl_bt_gatt_server_send_notification(connection,
                                            characteristic,
                                            sizeof(temp_send),
                                            (uint8_t*)&temp_send);
        app_log_info("%s: %dC\n", __FUNCTION__,(int)temp_recup()/10);
        break;
      default:
        app_log_info("%s: default\n", __FUNCTION__);
        break;
    }
    break;



    case sl_bt_evt_gatt_server_user_read_request_id:
      switch(evt->data.evt_gatt_server_user_read_request.characteristic){
        case gattdb_temperature:
          int temp_send=temp_recup()*10;
          uint16_t len;
          app_log_info("%s: %dC\n", __FUNCTION__,(int)temp_recup()/10);
          sl_bt_gatt_server_send_user_read_response(evt->data.handle,
                                                    evt->data.evt_gatt_server_user_read_request.characteristic,
                                                    0,
                                                    sizeof(temp_send),
                                                    (uint8_t*) &temp_send,
                                                    (uint16_t*) &len);
          break;
        default:
          app_log_info("%s: default\n", __FUNCTION__);
      break;
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
