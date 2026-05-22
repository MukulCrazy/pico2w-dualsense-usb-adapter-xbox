#include "xbox_mode.h"
#include "pico/time.h"
#include "tusb.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include <string.h>

bool xbox_mode_active = false;

enum USB_REINIT_STATE {
    USB_REINIT_NONE,
    USB_REINIT_DISCONNECT,
};

static USB_REINIT_STATE reinit_state = USB_REINIT_NONE;
static uint64_t reinit_timestamp = 0;

// Extern linkages to your core systems
extern int reportSeqCounter;
extern void state_set(uint8_t *data, const uint8_t size);
extern void bt_write(const uint8_t *data, uint16_t len);

// Official 20-byte Input Report structure for a wired Xbox 360 Controller
static uint8_t xbox_report[20] = {0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Native Wired Xbox 360 Controller Configuration Descriptor (Exactly 49 bytes total)
uint8_t desc_xbox_configuration[] = {
    // Configuration Header (9 bytes - Total length = 0x0031 / 49 bytes)
    0x09, 0x02, 0x31, 0x00, 0x01, 0x01, 0x00, 0xC0, 0xFA, 
    
    // Interface Descriptor (9 bytes)
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x5D, 0x01, 0x00, 
    
    // Official XInput 17-byte Companion Sub-Descriptor (Maps 0x84 to EP4 IN, 0x03 to EP3 OUT)
    0x11, 0x21, 0x00, 0x01, 0x01, 0x25, 0x84, 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, 0x03, 0x08, 0x00, 0x00,
    
    // Endpoint IN EP4 (7 bytes - wMaxPacketSize = 32, 1ms polling)
    0x07, 0x05, 0x84, 0x03, 0x20, 0x00, 0x01,             
    
    // Endpoint OUT EP3 (7 bytes - wMaxPacketSize = 32, 8ms polling)
    0x07, 0x05, 0x03, 0x03, 0x20, 0x00, 0x08              
};

// Maps 0-255 axis to standard Xbox -32768 to 32767 range
static int16_t map_axis(uint8_t val) {
    return (int16_t)(((int32_t)val * 257) - 32768);
}

// Maps 0-255 axis and corrects orientation (Inverts Y-axis for Xbox standard)
static int16_t map_axis_y(uint8_t val) {
    return (int16_t)(32767 - ((int32_t)val * 257));
}

void xbox_mode_check_combo(const uint8_t *state_data) {
    bool create_pressed = (state_data[8] & 0x10) != 0;
    bool options_pressed = (state_data[8] & 0x20) != 0;

    static uint64_t combo_start_time = 0;
    static bool combo_triggered = false;

    if (create_pressed && options_pressed) {
        if (combo_start_time == 0) {
            combo_start_time = time_us_64();
        } else if (!combo_triggered && (time_us_64() - combo_start_time >= 5000000)) {
            combo_triggered = true;
            xbox_mode_request_reboot();
        }
    } else {
        combo_start_time = 0;
        combo_triggered = false;
    }
}

void xbox_mode_request_reboot(void) {
    xbox_mode_active = !xbox_mode_active;
    
    // Safety check: Kill vibration when exiting Xbox mode
    if (!xbox_mode_active) {
        uint8_t outputData[78]{};
        outputData[0] = 0x31;
        outputData[1] = reportSeqCounter << 4;
        if (++reportSeqCounter == 256) reportSeqCounter = 0;
        outputData[2] = 0x10;
        state_set(outputData + 3, 63);
        outputData[3] |= 0x03; 
        outputData[5] = 0;     
        outputData[6] = 0;     
        bt_write(outputData, sizeof(outputData));
    }

    tud_disconnect();
    reinit_timestamp = time_us_64();
    reinit_state = USB_REINIT_DISCONNECT;
}

void xbox_mode_handle_usb_reinit(void) {
    if (reinit_state == USB_REINIT_DISCONNECT) {
        if (time_us_64() - reinit_timestamp >= 200000) { 
            tud_connect();
            reinit_state = USB_REINIT_NONE;
        }
    }
}

void xbox_mode_store_report(const uint8_t *state_data) {
    uint8_t dpad = state_data[7] & 0x0F;
    uint8_t xbox_dpad = 0;
    if (dpad == 0 || dpad == 1 || dpad == 7) xbox_dpad |= 0x01; // Up
    if (dpad == 3 || dpad == 4 || dpad == 5) xbox_dpad |= 0x02; // Down
    if (dpad == 5 || dpad == 6 || dpad == 7) xbox_dpad |= 0x04; // Left
    if (dpad == 1 || dpad == 2 || dpad == 3) xbox_dpad |= 0x08; // Right
    
    bool square   = (state_data[7] & 0x10) != 0;
    bool cross    = (state_data[7] & 0x20) != 0;
    bool circle   = (state_data[7] & 0x40) != 0;
    bool triangle = (state_data[7] & 0x80) != 0;
    
    bool l1      = (state_data[8] & 0x01) != 0;
    bool r1      = (state_data[8] & 0x02) != 0;
    bool create  = (state_data[8] & 0x10) != 0;
    bool options = (state_data[8] & 0x20) != 0;
    bool l3      = (state_data[8] & 0x40) != 0;
    bool r3      = (state_data[8] & 0x80) != 0;
    bool ps_home = (state_data[9] & 0x01) != 0;

    uint8_t b2 = xbox_dpad;
    if (options) b2 |= 0x10; // Start
    if (create)  b2 |= 0x20; // Back
    if (l3)      b2 |= 0x40; // LS Click
    if (r3)      b2 |= 0x80; // RS Click

    uint8_t b3 = 0;
    if (l1)       b3 |= 0x01; // LB
    if (r1)       b3 |= 0x02; // RB
    if (ps_home)  b3 |= 0x04; // Guide
    if (cross)    b3 |= 0x10; // A
    if (circle)   b3 |= 0x20; // B
    if (square)   b3 |= 0x40; // X
    if (triangle) b3 |= 0x80; // Y

    xbox_report[0] = 0x00; 
    xbox_report[1] = 0x14; 
    xbox_report[2] = b2;   
    xbox_report[3] = b3;   
    xbox_report[4] = state_data[4]; // LT
    xbox_report[5] = state_data[5]; // RT
    
    int16_t lx_val = map_axis(state_data[0]);
    xbox_report[6] = lx_val & 0xFF;
    xbox_report[7] = (lx_val >> 8) & 0xFF;
    
    int16_t ly_val = map_axis_y(state_data[1]);
    xbox_report[8] = ly_val & 0xFF;
    xbox_report[9] = (ly_val >> 8) & 0xFF;
    
    int16_t rx_val = map_axis(state_data[2]);
    xbox_report[10] = rx_val & 0xFF;
    xbox_report[11] = (rx_val >> 8) & 0xFF;
    
    int16_t ry_val = map_axis_y(state_data[3]);
    xbox_report[12] = ry_val & 0xFF;
    xbox_report[13] = (ry_val >> 8) & 0xFF;
}

void xbox_mode_send_report(void) {
    if (tud_ready()) {
        if (usbd_edpt_claim(0, 0x84)) {
            usbd_edpt_xfer(0, 0x84, xbox_report, 20);
        }
    }
}

// Fixed translation hook using official TinyUSB Class Drivers
void xbox_mode_receive_rumble(void) {
    if (!xbox_mode_active || !tud_ready()) {
        return;
    }

    // Safely query the driver-level buffer instead of racing raw endpoints
    if (tud_vendor_available()) {
        uint8_t xbox_rx_buf[32];
        uint32_t count = tud_vendor_read(xbox_rx_buf, sizeof(xbox_rx_buf));

        // Wired Xbox 360 Rumble Report Structure Verification:
        // Byte 0: 0x00 (Report Type), Byte 1: 0x08 (Data Length)
        if (count >= 5 && xbox_rx_buf[0] == 0x00 && xbox_rx_buf[1] == 0x08) {
            uint8_t left_motor = xbox_rx_buf[3];  // Heavy / Low-frequency motor
            uint8_t right_motor = xbox_rx_buf[4]; // Light / High-frequency motor

            uint8_t outputData[78]{};
            outputData[0] = 0x31;
            outputData[1] = reportSeqCounter << 4;
            if (++reportSeqCounter == 256) {
                reportSeqCounter = 0;
            }
            outputData[2] = 0x10; // Trigger the state modification system flags
            
            state_set(outputData + 3, 63);

            // Modulating standard rumble commands onto DualSense haptics:
            // Force bits 0 & 1 high (EnableRumbleEmulation + UseRumbleNotHaptics)
            // This tricks the PS5 controller into mapping motor speed fields into haptic voice coils.
            outputData[3] |= 0x03; 
            outputData[5] = right_motor; // RumbleEmulationRight
            outputData[6] = left_motor;  // RumbleEmulationLeft

            bt_write(outputData, sizeof(outputData));
        }
    }
}

extern "C" void tud_mount_cb(void) {
    if (xbox_mode_active) {
        tusb_desc_endpoint_t desc_in = {};
        desc_in.bLength = 7;
        desc_in.bDescriptorType = TUSB_DESC_ENDPOINT;
        desc_in.bEndpointAddress = 0x84;
        desc_in.bmAttributes.xfer = TUSB_XFER_INTERRUPT;
        desc_in.wMaxPacketSize = 32;
        desc_in.bInterval = 1;
        usbd_edpt_open(0, &desc_in);

        // REMOVED manual usbd_edpt_open for 0x03.
        // Letting CFG_TUD_VENDOR handle it natively removes the endpoint collision.
    }
}

extern "C" bool tud_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    if (xbox_mode_active) {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR || 
            request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS) {
            if (stage == CONTROL_STAGE_SETUP) {
                tud_control_status(rhport, request);
            }
            return true;
        }
    }
    return false;
}