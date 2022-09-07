#include <stdint.h>
#include <linux/hidraw.h>

#define KEYBOARD_HID_REPORT_SIZE 8
#define MOUSE_HID_REPORT_SIZE 4

struct hid_buf
{
    uint8_t report_id;
    unsigned char data[64];
} __attribute__((aligned(1)));

int initUSB(uint16_t *device_vid, uint16_t *device_pid, struct hidraw_report_descriptor *report_desc);
int cleanupUSB();
