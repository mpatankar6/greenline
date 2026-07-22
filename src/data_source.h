#pragma once
#include "gpu.h"
#include <stddef.h>
#include <string.h>

typedef enum { DS_UINT, DS_DOUBLE } DSType;

#define OFFSET_AND_TYPE(field)                                                 \
  offsetof(GpuState, field), _Generic((typeof((GpuState){0}.field)){0},        \
      unsigned int: DS_UINT,                                                   \
      double: DS_DOUBLE)

typedef struct {
  char name[32];
  char unit[4];
  int lower;
  int upper;
  size_t offset;
  DSType type;
} DataSource;

static constexpr DataSource SOURCES[] = {
    {"GPU Utilization", "%", 0, 100, OFFSET_AND_TYPE(gpu_util_percent)},
    {"GPU Temp", "°C", 0, 95, OFFSET_AND_TYPE(temperature_celsius)}};
static constexpr size_t SOURCE_COUNT = sizeof(SOURCES) / sizeof(DataSource);
static const DataSource NULL_SOURCE = {0};

static inline bool sources_equal(const DataSource SOURCE1,
                                 const DataSource SOURCE2) {
  // This handles NULL_SOURCE
  if (SOURCE1.name[0] == '\0' || SOURCE2.name[0] == '\0') {
    return false;
  }
  return (strcmp(SOURCE1.name, SOURCE2.name) == 0);
}
