#include "create-gadget-hid.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/hidraw.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

usbg_state *gadget_state;
usbg_gadget *gadget;
usbg_config *gadget_config;
usbg_function *gadget_function;

static char report_desc[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)
    0x05, 0x07,       //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,       //   Usage Minimum (0xE0)
    0x29, 0xE7,       //   Usage Maximum (0xE7)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,       //   Report Count (3)
    0x75, 0x01,       //   Report Size (1)
    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (Num Lock)
    0x29, 0x03,       //   Usage Maximum (Scroll Lock)
    0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,       //   Report Count (5)
    0x75, 0x01,       //   Report Size (1)
    0x91, 0x01,       //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x05, 0x07,       //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,       //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00, //   Usage Maximum (0xFF)
    0x81, 0x00,       //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,             // End Collection

    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x02, //   Report ID (2)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x03, //     Usage Maximum (0x03)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x05, //     Report Size (5)
    0x95, 0x01, //     Report Count (1)
    0x81, 0x01, //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};

static char report_desc2[] = {
    // Keyboard
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06, // 6: Keyboard
    0xa1, 0x01, // Collection 1 Start
    0x85, 0x01, //     Break
    0x05, 0x07, //     Break
    0x19, 0xe0, //     Break
    0x29, 0xe7, //     Break
    0x15, 0x00, //     Break
    0x25, 0x01, //     Break
    0x75, 0x01, //     Break
    0x95, 0x08, //     Break
    0x81, 0x02, //     Break
    0x95, 0x05, //     Report Count (5)
    // ...
    // ...
    // ...
    // ...
    0x05, 0x08,       //     Usage Page (LEDs)
    0x19, 0x01,       //     Break
    0x29, 0x05,       //     Break
    0x91, 0x02,       //     Break
    0x95, 0x01,       //     Report Count (1)
    0x75, 0x03,       //     Report Size (3)
    0x91, 0x01,       //     Break
    0x95, 0x06,       //     Break
    0x75, 0x08,       //     Break
    0x15, 0x00,       //     Break
    0x26, 0xa4, 0x00, //     Break
    0x05, 0x07,       //     Break
    0x19, 0x00,       //     Break
    0x2a, 0xa4, 0x00, //     Break
    0x81, 0x00,       //     Break
    0xc0,             // Collection 1 End

    // Mouse
    0x05, 0x01, // Break
    0x09, 0x02, // 2: Mouse
    0xa1, 0x01, // Collection 2 Start
    0x85, 0x02, //    Break
    0x09, 0x01, //    Break
    0xa1, 0x00, //    Sub Collection 1 Start
    0x95, 0x10, //        Break
    0x75, 0x01, //        Break
    0x15, 0x00, //        Break
    0x25, 0x01, //        Break
    0x05, 0x09, //        Break
    0x19, 0x01, //        Break
    0x29, 0x10, //        Break
    0x81, 0x02, //        Break
    0x05, 0x01, //        Break
    0x16, 0x01, //        Break
    0xf8, 0x26, //        Break
    0xff, 0x07, //        Break
    0x75, 0x0c, //        Break
    0x95, 0x02, //        Break
    0x09, 0x30, //        Break
    0x09, 0x31, //        Break
    0x81, 0x06, //        Break
    0x15, 0x81, //        Break
    0x25, 0x7f, //        Break
    0x75, 0x08, //        Break
    0x95, 0x01, //        Break
    0x09, 0x38, //        Break
    0x81, 0x06, //        Break
    0x95, 0x01, //        Break
    0x05, 0x0c, //        Break
    0x0a, 0x38, //        Break
    0x02, 0x81, //        Break
    0x06,       //        Break
    0xc0,       //    Sub Collection 1 End
    0xc0,       // Collection 2 End

    // ?
    0x05, 0x0c, // Break
    0x09, 0x01, // Break
    0xa1, 0x01, // Collection 3 Start
    0x85, 0x03, //    Break
    0x75, 0x10, //    Break
    0x95, 0x02, //    Break
    0x15, 0x01, //    Break
    0x26, 0x8c, //    Break
    0x02, 0x19, //    Break
    0x01, 0x2a, //    Break
    0x8c, 0x02, //    Break
    0x81, 0x60, //    Break
    0xc0,       // Collection 3 End

    // ?
    0x05, 0x01, // Break
    0x09, 0x80, // Break
    0xa1, 0x01, // Collection 4 Start
    0x85, 0x04, //    Break
    0x75, 0x02, //    Break
    0x95, 0x01, //    Break
    0x15, 0x01, //    Break
    0x25, 0x03, //    Break
    0x09, 0x82, //    Break
    0x09, 0x81, //    Break
    0x09, 0x83, //    Break
    0x81, 0x60, //    Break
    0x75, 0x06, //    Break
    0x81, 0x03, //    Break
    0xc0,       // Collection 4 End

    // ?
    0x06, 0x43,       // Break
    0xff, 0x0a,       // Break
    0x02, 0x02,       // Break
    0xa1, 0x01,       // Collection 5 Start
    0x85, 0x11,       //    Break
    0x75, 0x08,       //    Break
    0x95, 0x13,       //    Break
    0x15, 0x00,       //    Break
    0x26, 0xff, 0x00, //    Break
    0x09, 0x02,       //    Break
    0x81, 0x00,       //    Break
    0x09, 0x02,       //    Break
    0x91, 0x00,       //    Break
    0xc0,             // Collection 5 End
};

int initUSB(uint16_t *device_vid, uint16_t *device_pid, struct hidraw_report_descriptor *report_desc)
{
    int usbg_ret = -EINVAL;

    struct usbg_gadget_attrs gadget_attrs = {
        .bcdUSB = 0x0200,
        .bDeviceClass = USB_CLASS_PER_INTERFACE,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
        .idVendor = *device_vid,
        .idProduct = *device_pid,
        .bcdDevice = 0x0001, /* Verson of device */
    };

    struct usbg_gadget_strs gadget_strs = {
        .serial = "0123456789",   /* Serial number */
        .manufacturer = "Dwan",   /* Manufacturer */
        .product = "PiKMReciever" /* Product string */
    };

    struct usbg_config_strs config_strs = {
        .configuration = "1xHID"};

    struct usbg_f_hid_attrs function_attrs = {
        .protocol = 1,
        .report_desc = {
            .desc = report_desc2,
            .len = sizeof(report_desc2),
        },
        .report_length = 16,
        .subclass = 0,
    };

    usbg_ret = usbg_init("/sys/kernel/config", &gadget_state);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error on usbg init\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out1;
    }

    usbg_ret = usbg_create_gadget(gadget_state, "TheKeyboardMouse", &gadget_attrs, &gadget_strs, &gadget);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error creating gadget\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_create_function(gadget, USBG_F_HID, "usb0", &function_attrs, &gadget_function);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error creating function: USBG_F_HID\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_create_config(gadget, 1, "config", NULL, &config_strs, &gadget_config);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error creating config\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_add_config_function(gadget_config, "keyboard", gadget_function);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error adding function: keyboard\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out2;
    }

    usbg_ret = usbg_enable_gadget(gadget, DEFAULT_UDC);
    if (usbg_ret != USBG_SUCCESS)
    {
        fprintf(stderr, "Error enabling gadget\n");
        fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
                usbg_strerror(usbg_ret));
        goto out2;
    }

out2:
    usbg_cleanup(gadget_state);
    gadget_state = NULL;

out1:
    printf("Device with vid: 0x%04x and pid: 0x%04x created\n", *device_vid, *device_pid);
    return usbg_ret;
}

int cleanupUSB()
{
    if (gadget)
    {
        usbg_disable_gadget(gadget);
        usbg_rm_gadget(gadget, USBG_RM_RECURSE);
    }
    if (gadget_state)
    {
        usbg_cleanup(gadget_state);
    }
    return 0;
}
