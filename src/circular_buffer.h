#pragma once
#include <stddef.h>

constexpr int CIRCULAR_BUFFER_CAPACITY = 1024;

typedef struct CircularBuffer CircularBuffer;

CircularBuffer *circular_buffer_create();

// Insert a positive value into the buffer.
void circular_buffer_put(CircularBuffer *buffer, int value);

size_t circular_buffer_size(const CircularBuffer *buffer);

// Returns -1 if the buffer is empty
int circular_buffer_get(CircularBuffer *buffer);

void circular_buffer_destroy(CircularBuffer *buffer);
