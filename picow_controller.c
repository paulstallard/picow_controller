/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "hid_keyboard_demo.c"
 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "btstack.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "btstack_run_loop.h"
#include "pico/cyw43_arch.h"

#define BUTTON_I 14
#define BUTTON_K 15

// LED status state
static btstack_timer_source_t led_timer;
static bool led_on = false;
static bool led_override = false;   // used during keypress flash

// When not set to 0xffff, sniff and sniff subrating are enabled
static uint16_t host_max_latency = 1600;
static uint16_t host_min_timeout = 3200;

#define REPORT_ID 0x01

// close to USB HID Specification 1.1, Appendix B.1
const uint8_t hid_descriptor_keyboard[] = {

    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)

    // Report ID

    0x85, REPORT_ID,               // Report ID

    // Modifier byte (input)

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maximum (Keyboard Right GUI)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)

    // Reserved byte (input)

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

    // LED report + padding (output)

    0x95, 0x05,                    //   Report Count (5)
    0x75, 0x01,                    //   Report Size (1)
    0x05, 0x08,                    //   Usage Page (LEDs)
    0x19, 0x01,                    //   Usage Minimum (Num Lock)
    0x29, 0x05,                    //   Usage Maximum (Kana)
    0x91, 0x02,                    //   Output (Data, Variable, Absolute)

    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x03,                    //   Report Size (3)
    0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

    // Keycodes (input)

    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0xff,                    //   Logical Maximum (1)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff,                    //   Usage Maximum (Reserved)
    0x81, 0x00,                    //   Input (Data, Array)

    0xc0,                          // End collection
};

// STATE

static uint8_t hid_service_buffer[300];
static uint8_t device_id_sdp_service_buffer[100];
static const char hid_device_name[] = "BTstack HID Keyboard";
static btstack_packet_callback_registration_t hci_event_callback_registration;
static uint16_t hid_cid;
static uint8_t hid_boot_device = 0;

static enum {
    APP_BOOTING,
    APP_NOT_CONNECTED,
    APP_CONNECTING,
    APP_CONNECTED
} app_state = APP_BOOTING;

static void send_report(int modifier, int keycode){
    uint8_t report[] = { 0xa1, REPORT_ID, modifier, 0, keycode, 0, 0, 0, 0, 0};
    hid_device_send_interrupt_message(hid_cid, &report[0], sizeof(report));
}

// LED heartbeat / pairing blink
static void led_timer_handler(btstack_timer_source_t *ts) {
    if (!led_override) {
        // Normal blinking based on connection state
        led_on = !led_on;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    }

    // Choose blink speed
    int interval_ms = (app_state == APP_CONNECTED) ? 1000 : 200;

    btstack_run_loop_set_timer(ts, interval_ms);
    btstack_run_loop_add_timer(ts);
}

// Send single key with LED flash
static void send_single_key(uint8_t modifier, uint8_t keycode){
    if (!hid_cid) return;

    // Override LED blinking
    led_override = true;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    send_report(modifier, keycode);
    sleep_ms(30);
    send_report(0, 0);

    // Short flash
    sleep_ms(50);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Resume blinking
    led_override = false;
}

// Packet handler
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t * packet, uint16_t packet_size){
    UNUSED(channel);
    UNUSED(packet_size);
    uint8_t status;
    switch (packet_type){
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)){
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
                    app_state = APP_NOT_CONNECTED;
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // ssp: inform about user confirmation request
                    log_info("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", hci_event_user_confirmation_request_get_numeric_value(packet));
                    log_info("SSP User Confirmation Auto accept\n");                   
                    break; 

                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)){
                        case HID_SUBEVENT_CONNECTION_OPENED:
                            status = hid_subevent_connection_opened_get_status(packet);
                            if (status != ERROR_CODE_SUCCESS) {
                                // outgoing connection failed
                                printf("Connection failed, status 0x%x\n", status);
                                app_state = APP_NOT_CONNECTED;
                                hid_cid = 0;
                                return;
                            }
                            app_state = APP_CONNECTED;
                            hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                            printf("HID Connected...\n");
                            break;
                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            printf("HID Disconnected\n");
                            app_state = APP_NOT_CONNECTED;
                            hid_cid = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// Button state for edge detection
static bool last_i_state = true;   // true = released (because of pull-up)
static bool last_k_state = true;

static btstack_timer_source_t gpio_timer;
static void gpio_timer_handler(btstack_timer_source_t *ts) {
    if (app_state == APP_CONNECTED) {

        bool i_now = gpio_get(BUTTON_I);   // true = released, false = pressed
        bool k_now = gpio_get(BUTTON_K);

        // Detect press of I (transition: released -> pressed)
        if (last_i_state && !i_now) {
            send_single_key(0, 0x0C);   // 'I'
        }

        // Detect press of K
        if (last_k_state && !k_now) {
            send_single_key(0, 0x0E);   // 'K'
        }

        // Update state
        last_i_state = i_now;
        last_k_state = k_now;
    }

    // Re-arm timer
    btstack_run_loop_set_timer(ts, 10);
    btstack_run_loop_add_timer(ts);
}

/* @section Main Application Setup
 *
 * @text Listing MainConfiguration shows main application code. 
 * To run a HID Device service you need to initialize the SDP, and to create and register HID Device record with it. 
 * At the end the Bluetooth stack is started.
 */

int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){
    (void)argc;
    (void)argv;

    // allow to get found by inquiry
    gap_discoverable_control(1);
    // use Limited Discoverable Mode; Peripheral; Keyboard as CoD
    gap_set_class_of_device(0x2540);
    // set local name to be identified - zeroes will be replaced by actual BD ADDR
    gap_set_local_name("PICO-W Controller");
    // allow for role switch in general and sniff mode
    gap_set_default_link_policy_settings( LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE );
    // allow for role switch on outgoing connections - this allow HID Host to become master when we re-connect to it
    gap_set_allow_role_switch(true);

    // L2CAP
    l2cap_init();

#ifdef ENABLE_BLE
    // Initialize LE Security Manager. Needed for cross-transport key derivation
    sm_init();
#endif

    // SDP Server
    sdp_init();
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));

    uint8_t hid_virtual_cable = 0;
    uint8_t hid_remote_wake = 1;
    uint8_t hid_reconnect_initiate = 1;
    uint8_t hid_normally_connectable = 1;

    hid_sdp_record_t hid_params = {
        // hid sevice subclass 2540 Keyboard, hid counntry code 33 US
        0x2540, 33, 
        hid_virtual_cable, hid_remote_wake, 
        hid_reconnect_initiate, hid_normally_connectable,
        hid_boot_device,
        host_max_latency, host_min_timeout,
        3200,
        hid_descriptor_keyboard,
        sizeof(hid_descriptor_keyboard),
        hid_device_name
    };
    
    hid_create_sdp_record(hid_service_buffer, 0x10001, &hid_params);

    printf("HID service record size: %u\n", de_get_len( hid_service_buffer));
    sdp_register_service(hid_service_buffer);

    // device info: BlueKitchen GmbH, product 1, version 1
    device_id_create_sdp_record(device_id_sdp_service_buffer, 0x10003,
        DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH,
        BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH,
        1, 1);
    printf("Device ID SDP service record size: %u\n", de_get_len((uint8_t*)device_id_sdp_service_buffer));
    sdp_register_service(device_id_sdp_service_buffer);

    // HID Device
    hid_device_init(hid_boot_device, sizeof(hid_descriptor_keyboard), hid_descriptor_keyboard);
       
    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for HID events
    hid_device_register_packet_handler(&packet_handler);

    // Button GPIOs
    gpio_init(BUTTON_I);
    gpio_set_dir(BUTTON_I, GPIO_IN);
    gpio_pull_up(BUTTON_I);

    gpio_init(BUTTON_K);
    gpio_set_dir(BUTTON_K, GPIO_IN);
    gpio_pull_up(BUTTON_K);

    // ensure LED starts off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Start LED status blinking
    btstack_run_loop_set_timer_handler(&led_timer, led_timer_handler);
    btstack_run_loop_set_timer(&led_timer, 200);   // start in pairing mode
    btstack_run_loop_add_timer(&led_timer);

    // Start GPIO polling timer
    btstack_run_loop_set_timer_handler(&gpio_timer, gpio_timer_handler);
    btstack_run_loop_set_timer(&gpio_timer, 10);
    btstack_run_loop_add_timer(&gpio_timer);

    // turn on!
    hci_power_control(HCI_POWER_ON);
}
