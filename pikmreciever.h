#include <pthread.h>
#include <stdint.h>

#ifndef HOOK_PATH
#define HOOK_PATH "/home/pi/pikmreciever/hook.sh"
#endif

int initUSB(uint16_t device_vid, uint16_t device_pid);
int main();
void sendHIDReport();
