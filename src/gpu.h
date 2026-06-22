#pragma once
#include <nvml.h>
#include <stdint.h>

typedef struct [[nodiscard]] {
  char name[NVML_DEVICE_NAME_V2_BUFFER_SIZE];
  const char *architecture;
  uint64_t total_vram_bytes;
  uint64_t usable_vram_bytes;
  unsigned int num_fans;
  unsigned int num_gpu_cores;
} GpuInfo;

typedef struct Gpu Gpu;

[[nodiscard]]
Gpu *gpu_init();

GpuInfo gpu_get_info(const Gpu *gpu);

void gpu_destroy(Gpu *gpu);
