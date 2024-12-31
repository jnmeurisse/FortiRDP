#pragma once
#include <ctime>

#ifdef __cplusplus
extern "C"
{
#endif

char* strptime(const char* buf, const char* fmt, struct tm* tm);

#ifdef __cplusplus
}
#endif
