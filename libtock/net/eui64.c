#include "eui64.h"

returncode_t get_eui64(uint64_t *eui64) {
  return libtock_eui64_command_getter(eui64);
}
