#ifndef LOGGING_H__
#define LOGGING_H__

#include "config.h"
#include "logging_internal.h"

/** Standard logging macros, will produce output if LOG_LEVEL is high enough */
#define LOG_ERROR(message, ...)         LOG(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...)       LOG(LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define LOG_INFO(message, ...)          LOG(LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...)         LOG(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)

/** Raw logging macros, will always print without code position */
#define LOG_ERROR_RAW(message, ...)   LOG_RAW(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOG_WARNING_RAW(message, ...) LOG_RAW(LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define LOG_INFO_RAW(message, ...)    LOG_RAW(LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define LOG_DEBUG_RAW(message, ...)   LOG_RAW(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)

/** Return a function with the status and a logging message if the status is not success */
#define RETURN_ERROR_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_ERROR(__VA_ARGS__);   return status; }
#define RETURN_WARNING_ON(status, ...) if (status != NRF_SUCCESS) { LOG_WARNING(__VA_ARGS__); return status; }
#define RETURN_INFO_ON(status, ...)    if (status != NRF_SUCCESS) { LOG_INFO(__VA_ARGS__);    return status; }
#define RETURN_DEBUG_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_DEBUG(__VA_ARGS__);   return status; }

/** Return a function with a given error and a logging message if the condition fails */
#define RETURN_ERROR_IF(condition, error, ...)   if (condition) { LOG_ERROR(__VA_ARGS__);   return error; }
#define RETURN_WARNING_IF(condition, error, ...) if (condition) { LOG_WARNING(__VA_ARGS__); return error; }
#define RETURN_INFO_IF(condition, error, ...)    if (condition) { LOG_INFO(__VA_ARGS__);    return error; }
#define RETURN_DEBUG_IF(condition, error, ...)   if (condition) { LOG_DEBUG(__VA_ARGS__);   return error; }

/** Create a logging message if the status is not success */
#define PRINT_ERROR_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_ERROR(__VA_ARGS__); }
#define PRINT_WARNING_ON(status, ...) if (status != NRF_SUCCESS) { LOG_WARNING(__VA_ARGS__); }
#define PRINT_INFO_ON(status, ...)    if (status != NRF_SUCCESS) { LOG_INFO(__VA_ARGS__); }
#define PRINT_DEBUG_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_DEBUG(__VA_ARGS__); }

/** Print a logging message if the condition fails */
#define PRINT_ERROR_IF(condition, ...)   if ((condition)) { LOG_ERROR(__VA_ARGS__); }
#define PRINT_WARNING_IF(condition, ...) if ((condition)) { LOG_WARNING(__VA_ARGS__); }
#define PRINT_INFO_IF(condition, ...)    if ((condition)) { LOG_INFO(__VA_ARGS__); }
#define PRINT_DEBUG_IF(condition, ...)   if ((condition)) { LOG_DEBUG(__VA_ARGS__); }

/** Return a void function and log a message if the status is not success */
#define RETURN_WITH_ERROR_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_ERROR(__VA_ARGS__);   return; }
#define RETURN_WITH_WARNING_ON(status, ...) if (status != NRF_SUCCESS) { LOG_WARNING(__VA_ARGS__); return; }
#define RETURN_WITH_INFO_ON(status, ...)    if (status != NRF_SUCCESS) { LOG_INFO(__VA_ARGS__);    return; }
#define RETURN_WITH_DEBUG_ON(status, ...)   if (status != NRF_SUCCESS) { LOG_DEBUG(__VA_ARGS__);   return; }

/** Return a void function and log a message if the condition fails */
#define RETURN_WITH_ERROR_IF(condition, ...)   if (condition) { LOG_ERROR(__VA_ARGS__);   return; }
#define RETURN_WITH_WARNING_IF(condition, ...) if (condition) { LOG_WARNING(__VA_ARGS__); return; }
#define RETURN_WITH_INFO_IF(condition, ...)    if (condition) { LOG_INFO(__VA_ARGS__);    return; }
#define RETURN_WITH_DEBUG_IF(condition, ...)   if (condition) { LOG_DEBUG(__VA_ARGS__);   return; }

/** Return a given error and log a message if the status is not success */
#define RETURN_ERROR_ON_ERROR(status, error, ...)   if(status != NRF_SUCCESS) { LOG_ERROR(__VA_ARGS__);   return error; }
#define RETURN_WARNING_ON_ERROR(status, error, ...) if(status != NRF_SUCCESS) { LOG_WARNING(__VA_ARGS__); return error; }
#define RETURN_INFO_ON_ERROR(status, error, ...)    if(status != NRF_SUCCESS) { LOG_INFO(__VA_ARGS__);    return error; }
#define RETURN_DEBUG_ON_ERROR(status, error, ...)   if(status != NRF_SUCCESS) { LOG_DEBUG(__VA_ARGS__);   return error; }

#endif
