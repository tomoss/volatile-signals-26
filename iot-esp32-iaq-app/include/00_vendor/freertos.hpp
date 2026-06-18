#ifndef VENDOR_FREERTOS_H
#define VENDOR_FREERTOS_H

// FreeRTOS's portmacro.h trips the strict -Wconversion/-Wsign-conversion/-Wattributes flags.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wattributes"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#pragma GCC diagnostic pop

#endif // VENDOR_FREERTOS_H
