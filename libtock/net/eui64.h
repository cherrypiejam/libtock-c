#pragma once

#include "tock.h"
#include "syscalls/eui64_syscalls.h"

#ifdef __cplusplus
extern "C" {
#endif

returncode_t get_eui64(uint64_t* eui64);

#ifdef __cplusplus
}
#endif
