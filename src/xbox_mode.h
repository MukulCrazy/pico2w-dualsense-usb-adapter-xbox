#ifndef XBOX_MODE_H
#define XBOX_MODE_H

#include <stdint.h>
#include <stdbool.h>

extern bool xbox_mode_active;
extern uint8_t desc_xbox_configuration[];

void xbox_mode_check_combo(const uint8_t *state_data);
void xbox_mode_store_report(const uint8_t *state_data);
void xbox_mode_send_report(void);
void xbox_mode_request_reboot(void);
void xbox_mode_handle_usb_reinit(void);
void xbox_mode_receive_rumble(void); // <-- Added function declaration

#endif // XBOX_MODE_H