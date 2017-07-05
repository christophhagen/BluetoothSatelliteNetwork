#include "logging.h"

#if LOG_LEVEL > LOG_LEVEL_OFF

#if LOG_CODE_POSITION == 1
void reportEventWithPosition(const char* file, uint32_t line, uint8_t level, const char* message, ...)  {
    // TODO: Add a persistent log (write to flash), or send debug info somewhere
}
#endif

void reportEvent(uint8_t level, const char* message, ...) {
    // TODO: Add a persistent log (write to flash), or send debug info somewhere
}

#endif // Logging enabled
