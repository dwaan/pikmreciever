#include <pthread.h>
#include <stdint.h>

int initUSB(uint16_t device_vid, uint16_t device_pid);
int main();
void sendHIDReport();
