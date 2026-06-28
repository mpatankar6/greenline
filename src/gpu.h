#pragma once

typedef struct [[nodiscard]] {
  bool initialized;
  char name[128];
  const char *architecture;
  int total_vram_mib;
  int usable_vram_mib;
  int used_vram_mib;
  int free_vram_mib;
  unsigned int encoder_utilization;
  unsigned int decoder_utilization;
  unsigned int core_clock_mhz;
  unsigned int memory_clock_mhz;
  char performance_state[4]; // Ex: P0, P8, or ?
  unsigned int num_fans;
  unsigned int num_gpu_cores;
} GpuState;

typedef struct Gpu Gpu;

[[nodiscard]]
Gpu *gpu_init();

[[nodiscard]]
const GpuState *gpu_get_state(const Gpu *gpu);

void gpu_update_state(Gpu *gpu);

void gpu_destroy(Gpu *gpu);
