#pragma once

typedef struct {
  bool initialized;
  char driver_version[128];
  char nvml_version[128];
  char cuda_version[128];
  char name[128];
  char vbios_version[32];
  const char *architecture;
  int total_vram_mib;
  int usable_vram_mib;
  int used_vram_mib;
  int free_vram_mib;
  unsigned int encoder_util_percent;
  unsigned int decoder_util_percent;
  unsigned int gpu_util_percent;
  unsigned int mem_ctrl_util_percent;
  unsigned int core_clock_mhz;
  unsigned int memory_clock_mhz;
  int temperature_celsius;
  unsigned int fan_speed_percentage;
  unsigned int fan_speed_rpm;
  char performance_state[4]; // Ex: P0, P8, or ?
  unsigned int num_fan_controllers;
  unsigned int num_gpu_cores;
  unsigned int pcie_max_link_generation;
  unsigned int pcie_max_link_width;
  unsigned int pcie_max_link_speed_mbps;
  unsigned int tdp_milliwatts;
} GpuState;

typedef struct Gpu Gpu;

[[nodiscard]]
Gpu *gpu_init();

[[nodiscard]]
const GpuState *gpu_get_state(const Gpu *gpu);

void gpu_update_state(Gpu *gpu);

void gpu_destroy(Gpu *gpu);
