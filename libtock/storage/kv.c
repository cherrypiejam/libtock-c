#include "kv.h"

#define DRIVER_NUM_KV_SYSTEM 0x50003

#define TOCK_KV_CB           0

#define TOCK_KV_KEY_BUF      0
#define TOCK_KV_INPUT_BUF    1
#define TOCK_KV_OUTPUT_BUF   0

#define TOCK_KV_CHECK_PRESENT   0
#define TOCK_KV_GET             1
#define TOCK_KV_SET             2
#define TOCK_KV_DELETE          3
#define TOCK_KV_ADD             4
#define TOCK_KV_UPDATE          5

int kv_set_callback(subscribe_upcall callback, void* callback_args) {
  subscribe_return_t sval = subscribe(DRIVER_NUM_KV_SYSTEM, TOCK_KV_CB, callback, callback_args);
  return tock_subscribe_return_to_returncode(sval);
}

int kv_set_key_buffer(const uint8_t* buffer, uint32_t len) {
  allow_ro_return_t aval = allow_readonly(DRIVER_NUM_KV_SYSTEM, TOCK_KV_KEY_BUF, (void*) buffer, len);
  return tock_allow_ro_return_to_returncode(aval);
}

int kv_set_input_buffer(const uint8_t* buffer, uint32_t len) {
  allow_ro_return_t aval = allow_readonly(DRIVER_NUM_KV_SYSTEM, TOCK_KV_INPUT_BUF, (void*) buffer, len);
  return tock_allow_ro_return_to_returncode(aval);
}

int kv_set_output_buffer(uint8_t* buffer, uint32_t len) {
  allow_rw_return_t aval = allow_readwrite(DRIVER_NUM_KV_SYSTEM, TOCK_KV_OUTPUT_BUF, (void*) buffer, len);
  return tock_allow_rw_return_to_returncode(aval);
}

int kv_check_status(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_CHECK_PRESENT, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

int kv_get(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_GET, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

int kv_set(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_SET, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

int kv_delete(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_DELETE, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

int kv_add(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_ADD, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

int kv_update(void) {
  syscall_return_t cval = command(DRIVER_NUM_KV_SYSTEM, TOCK_KV_UPDATE, 0, 0);
  return tock_command_return_novalue_to_returncode(cval);
}

struct kv_data {
  bool fired;
  int err;
  int length;
};

static struct kv_data result = { .fired = false };

// Internal callback for faking synchronous reads
static void kv_upcall( int                          err,
                       int                          length,
                       __attribute__ ((unused)) int unused2,
                       void*                        ud) {
  struct kv_data* data = (struct kv_data*) ud;
  data->fired  = true;
  data->err    = tock_status_to_returncode(err);
  data->length = length;
}

int kv_get_sync(const uint8_t* key_buffer, uint32_t key_len, uint8_t* ret_buffer, uint32_t ret_len,
                uint32_t* value_len) {
  int err;
  result.fired = false;

  err = kv_set_callback(kv_upcall, (void*) &result);
  if (err < 0) return err;

  err = kv_set_key_buffer(key_buffer, key_len);
  if (err < 0) return err;

  err = kv_set_output_buffer(ret_buffer, ret_len);
  if (err < 0) return err;

  err = kv_get();
  if (err < 0) return err;

  // Wait for the callback.
  yield_for(&result.fired);

  // Retrieve the buffers.
  err = kv_set_output_buffer(NULL, 0);
  if (err < 0) return err;

  err = kv_set_key_buffer(NULL, 0);
  if (err < 0) return err;

  if (result.err < 0) {
    return result.err;
  }

  // Return the length of the retrieved value.
  *value_len = result.length;

  return RETURNCODE_SUCCESS;
}

static int kv_insert_sync(const uint8_t* key_buffer, uint32_t key_len, const uint8_t* val_buffer,
                          uint32_t val_len, int (*op_fn)(void)){
  int err;
  result.fired = false;

  err = kv_set_callback(kv_upcall, (void*) &result);
  if (err < 0) return err;

  err = kv_set_key_buffer(key_buffer, key_len);
  if (err < 0) return err;

  err = kv_set_input_buffer(val_buffer, val_len);
  if (err < 0) return err;

  // Do the requested set/add/update operation.
  err = op_fn();
  if (err < 0) return err;

  // Wait for the callback.
  yield_for(&result.fired);

  // Retrieve the buffers.
  err = kv_set_output_buffer(NULL, 0);
  if (err < 0) return err;

  err = kv_set_key_buffer(NULL, 0);
  if (err < 0) return err;

  if (result.err < 0) {
    return result.err;
  }

  return RETURNCODE_SUCCESS;
}

int kv_set_sync(const uint8_t* key_buffer, uint32_t key_len, const uint8_t* val_buffer, uint32_t val_len) {
  return kv_insert_sync(key_buffer, key_len, val_buffer, val_len, kv_set);
}

int kv_add_sync(const uint8_t* key_buffer, uint32_t key_len, const uint8_t* val_buffer, uint32_t val_len) {
  return kv_insert_sync(key_buffer, key_len, val_buffer, val_len, kv_add);
}

int kv_update_sync(const uint8_t* key_buffer, uint32_t key_len, const uint8_t* val_buffer, uint32_t val_len) {
  return kv_insert_sync(key_buffer, key_len, val_buffer, val_len, kv_update);
}

int kv_delete_sync(const uint8_t* key_buffer, uint32_t key_len) {
  int err;
  result.fired = false;

  err = kv_set_callback(kv_upcall, (void*) &result);
  if (err < 0) return err;

  err = kv_set_key_buffer(key_buffer, key_len);
  if (err < 0) return err;

  err = kv_delete();
  if (err < 0) return err;

  // Wait for the callback.
  yield_for(&result.fired);

  // Retrieve the buffers.
  err = kv_set_key_buffer(NULL, 0);
  if (err < 0) return err;

  if (result.err < 0) {
    return result.err;
  }

  return RETURNCODE_SUCCESS;
}
