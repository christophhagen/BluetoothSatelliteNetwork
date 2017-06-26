#ifndef ERROR_H__
#define ERROR_H__

#include "sdk_errors.h" // NRF_SUCCESS
#include "mod_log.h"
#include "stdbool.h"

#define RETURN_INTERNAL_ON(error, ...) if (error != NRF_SUCCESS) { PRINT_WITH_INFO(__VA_ARGS__); return error; }
#define RETURN_ERROR_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_ERROR(__VA_ARGS__); return error; }
#define RETURN_WARNING_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_WARNING(__VA_ARGS__); return error; }
#define RETURN_INFO_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_INFO(__VA_ARGS__); return error; }
#define RETURN_DEBUG_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_DEBUG(__VA_ARGS__); return error; }

#define RETURN_ERROR_IF(condition, error, ...) if (condition) { NRF_LOG_ERROR(__VA_ARGS__); return error; }
#define RETURN_WARNING_IF(condition, error, ...) if (condition) { NRF_LOG_WARNING(__VA_ARGS__); return error; }
#define RETURN_INFO_IF(condition, error, ...) if (condition) { NRF_LOG_INFO(__VA_ARGS__); return error; }
#define RETURN_DEBUG_IF(condition, error, ...) if (condition) { NRF_LOG_DEBUG(__VA_ARGS__); return error; }

#define PRINT_INTERNAL_ON(error, ...) if (error != NRF_SUCCESS) { PRINT_WITH_INFO(__VA_ARGS__); }
#define PRINT_ERROR_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_ERROR(__VA_ARGS__); }
#define PRINT_WARNING_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_WARNING(__VA_ARGS__); }
#define PRINT_INFO_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_INFO(__VA_ARGS__); }
#define PRINT_DEBUG_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_DEBUG(__VA_ARGS__); }

#define PRINT_ERROR_IF(condition, ...) if ((condition)) { NRF_LOG_ERROR(__VA_ARGS__); }

#define RETURN_WITH_ERROR_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_ERROR(__VA_ARGS__); return; }
#define RETURN_WITH_WARNING_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_WARNING(__VA_ARGS__); return; }
#define RETURN_WITH_INFO_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_INFO(__VA_ARGS__); return; }
#define RETURN_WITH_DEBUG_ON(error, ...) if (error != NRF_SUCCESS) { NRF_LOG_DEBUG(__VA_ARGS__); return; }

#define RETURN_WITH_ERROR_IF(condition, ...) if (condition) { NRF_LOG_ERROR(__VA_ARGS__); return; }
#define RETURN_WITH_WARNING_IF(condition, ...) if (condition) { NRF_LOG_WARNING(__VA_ARGS__); return; }
#define RETURN_WITH_INFO_IF(condition, ...) if (condition) { NRF_LOG_INFO(__VA_ARGS__); return; }
#define RETURN_WITH_DEBUG_IF(condition, ...) if (condition) { NRF_LOG_DEBUG(__VA_ARGS__); return; }

#endif
