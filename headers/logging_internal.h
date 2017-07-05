#ifndef LOGGING_INTERNAL_H__
#define LOGGING_INTERNAL_H__

#include "stdint.h"
#include "stdbool.h"

#define LOG_LEVEL_OFF       0
#define LOG_LEVEL_ERROR     1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_INFO      3
#define LOG_LEVEL_DEBUG     4

#if LOG_ON_CONSOLE
#define NRF_LOG_ENABLED                 1
#define NRF_LOG_DEFAULT_LEVEL           3   // Log everything as INFO_RAW
#else
#define NRF_LOG_ENABLED                 0
#define NRF_LOG_DEFAULT_LEVEL           0
#endif

#include "nrf_log.h"
#include "sdk_config.h"
#include "sdk_errors.h" // NRF_SUCCESS

void reportEventWithPosition(const char* file, uint32_t line, uint8_t level, const char* message, ...);
void reportEvent(uint8_t level, const char* message, ...);

#if LOG_CODE_POSITION == 1

#define LOG_WITH(LEVEL, message, ...)  NRF_LOG_RAW_INFO("%s, line %d: ", (uint32_t) __FILE__, __LINE__); \
NRF_LOG_RAW_INFO(message, ##__VA_ARGS__);                                                  \
reportEventWithPosition(__FILE__, __LINE__, LEVEL, message, ##__VA_ARGS__)
#else

#define LOG_WITH(LEVEL, message, ...)  NRF_LOG_RAW_INFO(message, ##__VA_ARGS__); \
reportEvent(LEVEL, message, ##__VA_ARGS__)

#endif

#define LOG_CONSOLE(LEVEL, message, ...)    NRF_LOG_RAW_INFO()

#define LOG(LEVEL, message, ...) if (LOG_LEVEL <= LEVEL) { \
    LOG_WITH(LEVEL, message, ##__VA_ARGS__);               \
}

#define LOG_RAW(LEVEL, message, ...)    reportEvent(LEVEL, message, ##__VA_ARGS__)

#endif
