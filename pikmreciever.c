#include "pikmreciever.h"
#include "include/create-gadget-hid.h"
#include "include/remove-gadget-hid.h"

#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <poll.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

// Submodule
#include <usbg/usbg.h>

// Library: apt install libhidapi
#include <hidapi/hidapi.h>

#define EVIOC_GRAB 1
#define EVIOC_UNGRAB 0

int hid_output;
volatile int running = 0;
volatile int grabbed = 0;

int res;
int device_fd;
int uinput_device_fd;
struct hid_buf device_buf;

uint16_t device_vid;
uint16_t device_pid;
struct hidraw_report_descriptor report_desc;
char device_dev[20];

const char *bus_str(int bus)
{
    switch (bus)
    {
    case BUS_USB:
        return "USB";
        break;
    case BUS_HIL:
        return "HIL";
        break;
    case BUS_BLUETOOTH:
        return "Bluetooth";
        break;
    case BUS_VIRTUAL:
        return "Virtual";
        break;
    default:
        return "Other";
        break;
    }
}

void signal_handler(int dummy)
{
    running = 0;
}

bool modprobe_libcomposite()
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
        return false;
    if (pid == 0)
    {
        char *const argv[] = {"modprobe", "libcomposite"};
        execv("/usr/sbin/modprobe", argv);
        exit(0);
    }
    waitpid(pid, NULL, 0);
}

bool trigger_hook()
{
    int ret;
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s %u", HOOK_PATH, grabbed ? 1u : 0u);
    ret = system(buf);
}

int find_hidraw_device()
{
    int fd;
    int res;
    int desc_size = 0;
    char path[20];
    char buf[256];
    struct hidraw_devinfo hidinfo;
    struct hidraw_report_descriptor rpt_desc;

#define MAX_STR 255
    wchar_t wstr[MAX_STR];
    hid_device *handle;

    for (int x = 0; x < 16; x++)
    {
        sprintf(path, "/dev/hidraw%d", x);

        if ((fd = open(path, O_RDWR | O_NONBLOCK)) == -1)
        {
            continue;
        }

        memset(&rpt_desc, 0x0, sizeof(rpt_desc));
        memset(&hidinfo, 0x0, sizeof(hidinfo));
        memset(buf, 0x0, sizeof(buf));

        /* Get Raw Info */
        res = ioctl(fd, HIDIOCGRAWINFO, &hidinfo);
        if (res < 0)
        {
            perror("HIDIOCGRAWINFO");
        }
        else
        {
            printf("Found a device at %s:\n", path);
            printf("\tRaw Info:\n");
            printf("\t\tbustype: %d (%s)\n", hidinfo.bustype, bus_str(hidinfo.bustype));
            printf("\t\tvid: 0x%04hx\n", hidinfo.vendor);
            printf("\t\tpid: 0x%04hx\n", hidinfo.product);

            device_vid = hidinfo.vendor;
            device_pid = hidinfo.product;
            sprintf(device_dev, "%s", path);

            /* Get Physical Location */
            res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
            if (res >= 0)
                printf("\t\tRaw Phys: %s\n", buf);

            /* Get Raw Name */
            res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
            if (res >= 0)
                printf("\t\tRaw Name: %s\n", buf);

            /* Get Report Descriptor Size */
            res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
            if (res >= 0)
            {
                printf("\t\tReport Descriptor Size: %d\n", desc_size);

                /* Get Report Descriptor */
                rpt_desc.size = desc_size;
                res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
                if (res >= 0)
                {
                    int new_line = 1;

                    printf("\t\tReport Descriptor:\n");
                    for (int i = 0; i < rpt_desc.size; i++)
                    {
                        printf("%d. 0x%02hx ", i, rpt_desc.value[i]);

                        new_line++;

                        if (new_line > 2 && rpt_desc.value[i + 1] == 0x00)
                            new_line--;
                        else if (rpt_desc.value[i + 1] == 0xc0)
                            new_line = 3;

                        if (new_line > 2)
                        {
                            printf("\n");
                            new_line = 1;
                        }
                    }
                    puts("\n");
                }

                report_desc = rpt_desc;
            }

            return fd;
        }

        close(fd);
    }

    return -1;
}

int grab(char *dev)
{
    printf("Grabbing: %s\n", dev);
    int fd = open(dev, O_RDONLY);
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    usleep(500000);
    ioctl(fd, EVIOCGRAB, EVIOC_GRAB);
    return fd;
}

void ungrab(int fd)
{
    ioctl(fd, EVIOCGRAB, EVIOC_UNGRAB);
    close(fd);
}

void printhex(unsigned char *buf, size_t len)
{
    for (int x = 0; x < len; x++)
    {
        printf("%x ", buf[x]);
    }
    printf("\n");
}

void ungrab_both()
{
    printf("Releasing Device\n");

    if (uinput_device_fd > -1)
    {
        ungrab(uinput_device_fd);
    }

    grabbed = 0;

    trigger_hook();
}

void grab_both()
{
    printf("Grabbing Device\n");

    if (device_fd > -1)
    {
        uinput_device_fd = grab(device_dev);
    }

    if (uinput_device_fd > -1)
    {
        grabbed = 1;
    }

    trigger_hook();
}

void send_empty_hid_reports_both()
{
    ssize_t res;

    if (device_fd > -1)
    {
#ifndef NO_OUTPUT
        memset(device_buf.data, 0, KEYBOARD_HID_REPORT_SIZE);
        res = write(hid_output, (unsigned char *)&device_buf, KEYBOARD_HID_REPORT_SIZE + 1);
#endif
    }
}

int main()
{
    modprobe_libcomposite();

    device_buf.report_id = 1;

    device_fd = find_hidraw_device();
    if (device_fd == -1)
    {
        printf("No device to forward, bailing out!\n");
        return 1;
    }

#ifndef NO_OUTPUT
    // Remove previous device if available
    res = remove_gadget(&device_vid, &device_pid);

    // Recreate device using new detail
    res = initUSB(&device_vid, &device_pid, &report_desc);
    if (res != USBG_SUCCESS && res != USBG_ERROR_EXIST && res != USBG_ERROR_BUSY)
    {
        return 1;
    }
#endif

    grab_both();

#ifndef NO_OUTPUT
    do
    {
        hid_output = open("/dev/hidg0", O_WRONLY | O_NDELAY);
    } while (hid_output == -1 && errno == EINTR);

    if (hid_output == -1)
    {
        printf("Error opening /dev/hidg0 for writing.\n");
        return 1;
    }
#endif

    printf("Running...\n");
    running = 1;
    signal(SIGINT, signal_handler);

    struct pollfd pollFd[1];
    pollFd[0].fd = device_fd;
    pollFd[0].events = POLLIN;

    while (running)
    {
        poll(pollFd, 1, -1);
        if (device_fd > -1)
        {
            int c = read(device_fd, device_buf.data, KEYBOARD_HID_REPORT_SIZE);

            if (c == KEYBOARD_HID_REPORT_SIZE)
            {
                printf("K:");
                printhex(device_buf.data, KEYBOARD_HID_REPORT_SIZE);

#ifndef NO_OUTPUT
                if (grabbed)
                {
                    res = write(hid_output, (unsigned char *)&device_buf, KEYBOARD_HID_REPORT_SIZE + 1);
                    usleep(1000);
                }
#endif

                // Trap Ctrl + Raspberry and toggle capture on/off
                if (device_buf.data[0] == 0x09)
                {
                    if (grabbed)
                    {
                        ungrab_both();
                        send_empty_hid_reports_both();
                    }
                    else
                    {
                        grab_both();
                    }
                }
                // Trap Ctrl + Shift + Raspberry and exit
                if (device_buf.data[0] == 0x0b)
                {
                    running = 0;
                    break;
                }
            }
        }
    }

    ungrab_both();
    send_empty_hid_reports_both();

#ifndef NO_OUTPUT
    printf("Cleanup USB\n");
    cleanupUSB();
#endif

    return 0;
}
