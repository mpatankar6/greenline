#include "circular_buffer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct CircularBuffer {
  int buffer[CIRCULAR_BUFFER_CAPACITY];
  size_t size;
  size_t writeIndex;
  size_t readIndex;
};

CircularBuffer *circular_buffer_create() {
  CircularBuffer *buffer = calloc(1, sizeof(CircularBuffer));
  memset(buffer->buffer, -1, sizeof(buffer->buffer));
  return buffer;
}

void circular_buffer_put(CircularBuffer *buffer, int value) {
  assert(value >= 0);
  bool at_capacity = buffer->size >= CIRCULAR_BUFFER_CAPACITY;
  if (at_capacity && buffer->writeIndex == buffer->readIndex) {
    ++buffer->readIndex;
    buffer->readIndex %= CIRCULAR_BUFFER_CAPACITY;
  }
  buffer->buffer[buffer->writeIndex++] = value;
  buffer->size = (int)at_capacity ? CIRCULAR_BUFFER_CAPACITY : buffer->size + 1;
  buffer->writeIndex %= CIRCULAR_BUFFER_CAPACITY;
}

size_t circular_buffer_size(const CircularBuffer *buffer) {
  return buffer->size;
}

int circular_buffer_peek(const CircularBuffer *buffer,
                         size_t offset_from_newest) {
  auto modulus = CIRCULAR_BUFFER_CAPACITY;
  auto read_index = (int)buffer->writeIndex - 1 - (int)offset_from_newest;
  auto wrapped_read_index = ((read_index % modulus) + modulus) % modulus;
  assert(wrapped_read_index >= 0 &&
         wrapped_read_index < CIRCULAR_BUFFER_CAPACITY);
  return buffer->buffer[wrapped_read_index];
}

int circular_buffer_get(CircularBuffer *buffer) {
  auto value = buffer->buffer[buffer->readIndex++];
  buffer->readIndex %= CIRCULAR_BUFFER_CAPACITY;
  return value;
}

void circular_buffer_destroy(CircularBuffer *buffer) { free(buffer); }
