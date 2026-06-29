#include "circular_buffer.h"
#include <assert.h>

#define TEST_VALUE 42

static void test_new_buffer_has_size_zero() {
  auto buffer = circular_buffer_create();
  assert(circular_buffer_size(buffer) == 0);
  circular_buffer_destroy(buffer);
}

static void test_get_on_empty_returns_sentinel() {
  auto buffer = circular_buffer_create();
  circular_buffer_get(buffer);
  assert(circular_buffer_get(buffer) == -1);
  circular_buffer_destroy(buffer);
}

static void test_put_increases_size() {
  auto buffer = circular_buffer_create();
  circular_buffer_put(buffer, TEST_VALUE);
  assert(circular_buffer_size(buffer) == 1);
  circular_buffer_destroy(buffer);
}

static void test_get_does_not_decrease_size() {
  auto buffer = circular_buffer_create();
  circular_buffer_put(buffer, TEST_VALUE);
  circular_buffer_get(buffer);
  assert(circular_buffer_size(buffer) == 1);
  circular_buffer_destroy(buffer);
}

static void test_gets_come_out_in_fifo_order() {
  auto buffer = circular_buffer_create();
  circular_buffer_put(buffer, 1);
  circular_buffer_put(buffer, 2);
  circular_buffer_put(buffer, 3);
  assert(circular_buffer_get(buffer) == 1);
  assert(circular_buffer_get(buffer) == 2);
  assert(circular_buffer_get(buffer) == 3);
  circular_buffer_destroy(buffer);
}

static void test_size_tracks_puts() {
  auto buffer = circular_buffer_create();
  circular_buffer_put(buffer, 1);
  circular_buffer_put(buffer, 2);
  circular_buffer_put(buffer, 3);
  assert(circular_buffer_size(buffer) == 3);
  circular_buffer_destroy(buffer);
}

static void test_size_caps_at_capacity() {
  auto buffer = circular_buffer_create();
  for (int i = 0; i < CIRCULAR_BUFFER_CAPACITY + 1; i++) {
    circular_buffer_put(buffer, i);
  }
  assert(circular_buffer_size(buffer) == CIRCULAR_BUFFER_CAPACITY);
  circular_buffer_destroy(buffer);
}

static void test_oldest_value_overwritten_on_overflow() {
  auto buffer = circular_buffer_create();
  for (int i = 0; i < CIRCULAR_BUFFER_CAPACITY + 1; i++) {
    circular_buffer_put(buffer, i);
  }
  // index 0 was overwritten, oldest remaining is index 1
  assert(circular_buffer_get(buffer) == 1);
  circular_buffer_destroy(buffer);
}

static void test_wrap_around_twice() {
  auto buffer = circular_buffer_create();
  for (int i = 0; i < (2 * CIRCULAR_BUFFER_CAPACITY) + 1; i++) {
    circular_buffer_put(buffer, i);
  }
  // At this point get should have wrapped around
  assert(circular_buffer_get(buffer) == CIRCULAR_BUFFER_CAPACITY + 1);
  circular_buffer_destroy(buffer);
}

int main(void) {
  test_new_buffer_has_size_zero();
  test_get_on_empty_returns_sentinel();
  test_put_increases_size();
  test_gets_come_out_in_fifo_order();
  test_size_tracks_puts();
  test_size_caps_at_capacity();
  test_oldest_value_overwritten_on_overflow();
  test_wrap_around_twice();
  return 0;
}
