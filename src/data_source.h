#pragma once
#include "gpu.h"
#include <stddef.h>
#include <string.h>

#define VALUE_OFFSET(field)                                                    \
  _Generic((typeof((GpuState){0}.field)){0},                                   \
      unsigned int: offsetof(GpuState, field))

#define OFFSET_BOUND(field)                                                    \
  _Generic((typeof((GpuState){0}.field)){0},                                   \
      unsigned int: (Bound){.kind = BOUND_STRUCT_OFFSET,                       \
                            .offset = offsetof(GpuState, field)})

#define CONST_BOUND(value)                                                     \
  (Bound) { .kind = BOUND_CONSTANT, .constant = (value) }
#define PERCENT_BOUNDS CONST_BOUND(0), CONST_BOUND(100)

typedef enum { BOUND_CONSTANT, BOUND_STRUCT_OFFSET } BoundKind;
typedef struct {
  BoundKind kind;
  union {
    int constant;
    size_t offset;
  };
} Bound;

typedef struct {
  char name[32];
  char unit[4];
  Bound lower;
  Bound upper;
  size_t offset;
} DataSource;

static inline bool sources_equal(const DataSource SOURCE1,
                                 const DataSource SOURCE2) {
  return strcmp(SOURCE1.name, SOURCE2.name) == 0;
}

static inline int resolve_bound(Bound bound, const GpuState *state) {
  switch (bound.kind) {
  case BOUND_CONSTANT:
    return bound.constant;
  case BOUND_STRUCT_OFFSET:
    return (int)*(unsigned int *)((char *)state + bound.offset);
  }
}

static inline int data_source_lower(DataSource source, const GpuState *state) {
  return resolve_bound(source.lower, state);
}

static inline int data_source_upper(DataSource source, const GpuState *state) {
  return resolve_bound(source.upper, state);
}

static inline int data_source_value(DataSource source, const GpuState *state) {
  return (int)*(unsigned int *)((char *)state + source.offset);
}

static constexpr DataSource SOURCES[] = {
    {"GPU Utilization", "%", PERCENT_BOUNDS, VALUE_OFFSET(gpu_util_percent)},
    {"GPU Temp", "°C", CONST_BOUND(0), CONST_BOUND(95),
     VALUE_OFFSET(temperature_celsius)},
    {"Core Clock", "MHz", CONST_BOUND(0), OFFSET_BOUND(max_core_clock_mhz),
     VALUE_OFFSET(core_clock_mhz)},
    {"Fan Speed", "%", PERCENT_BOUNDS, VALUE_OFFSET(fan_speed_percentage)},
    {"Fan Target", "%", PERCENT_BOUNDS, VALUE_OFFSET(fan_target_percent)},
    {"Mem Clock", "MHz", CONST_BOUND(0), OFFSET_BOUND(max_memory_clock_mhz),
     VALUE_OFFSET(memory_clock_mhz)},
    {"VRAM Used", "MiB", CONST_BOUND(0), OFFSET_BOUND(total_vram_mib),
     VALUE_OFFSET(used_vram_mib)},
    {"Mem Ctrl Util", "%", PERCENT_BOUNDS, VALUE_OFFSET(mem_ctrl_util_percent)},
    {"Encoder Util", "%", PERCENT_BOUNDS, VALUE_OFFSET(encoder_util_percent)},
    {"Decoder Util", "%", PERCENT_BOUNDS, VALUE_OFFSET(decoder_util_percent)}};
static constexpr size_t SOURCE_COUNT = sizeof(SOURCES) / sizeof(DataSource);
static const DataSource NULL_SOURCE = {0};
