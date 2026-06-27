#include "gpu.h"
#include <nvml.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct Gpu {
  nvmlDevice_t handle;
  GpuState state;
};

// Return true if there was an error, otherwise false.
[[gnu::format(printf, 2, 3)]]
static bool check_error(nvmlReturn_t status, const char *message, ...) {
  if (status == NVML_SUCCESS) {
    return false;
  }
  va_list args;
  va_start(args);
  (void)vfprintf(stderr, message, args);
  va_end(args);
  (void)fprintf(stderr, ": %s\n", nvmlErrorString(status));
  return true;
}

static const char *arch_to_string(nvmlDeviceArchitecture_t arch) {
  switch (arch) {
  case NVML_DEVICE_ARCH_KEPLER:
    return "Kepler";
  case NVML_DEVICE_ARCH_MAXWELL:
    return "Maxwell";
  case NVML_DEVICE_ARCH_PASCAL:
    return "Pascal";
  case NVML_DEVICE_ARCH_VOLTA:
    return "Volta";
  case NVML_DEVICE_ARCH_TURING:
    return "Turing";
  case NVML_DEVICE_ARCH_AMPERE:
    return "Ampere";
  case NVML_DEVICE_ARCH_ADA:
    return "Ada";
  case NVML_DEVICE_ARCH_HOPPER:
    return "Hopper";
  case NVML_DEVICE_ARCH_BLACKWELL:
    return "Blackwell";
  case NVML_DEVICE_ARCH_T23X:
    return "Orin";
  case NVML_DEVICE_ARCH_UNKNOWN:
  default:
    return "Unknown";
  }
}

static int bytes_to_mib(unsigned long long bytes) {
  const int BYTES_PER_MIB = 1024 * 1024;
  return (int)(bytes / BYTES_PER_MIB);
}

static void gpu_update_static_state(Gpu *gpu) {
  auto state = &gpu->state;
  auto device = gpu->handle;
  auto last_status = NVML_SUCCESS;

  last_status =
      nvmlDeviceGetName(device, state->name, NVML_DEVICE_NAME_V2_BUFFER_SIZE);
  check_error(last_status, "Error retrieving device name");

  nvmlDeviceArchitecture_t arch = {0};
  last_status = nvmlDeviceGetArchitecture(device, &arch);
  check_error(last_status, "Error retrieving device architecture");
  state->architecture = arch_to_string(arch);

  nvmlMemory_v2_t memory = {.version = nvmlMemory_v2};
  last_status = nvmlDeviceGetMemoryInfo_v2(device, &memory);
  check_error(last_status, "Error retrieving device memory info");
  state->total_vram_mib = bytes_to_mib(memory.total);
  state->usable_vram_mib = bytes_to_mib(memory.total - memory.reserved);
}

static void gpu_update_dynamic_state(Gpu *gpu) {
  auto state = &gpu->state;
  auto device = gpu->handle;
  auto last_status = NVML_SUCCESS;

  nvmlMemory_v2_t memory = {.version = nvmlMemory_v2};
  last_status = nvmlDeviceGetMemoryInfo_v2(device, &memory);
  check_error(last_status, "Error retrieving device memory info");
  state->used_vram_mib = bytes_to_mib(memory.used);
  state->free_vram_mib = bytes_to_mib(memory.free);

  unsigned int unused;
  last_status = nvmlDeviceGetDecoderUtilization(
      device, &state->decoder_utilization, &unused);
  check_error(last_status, "Error retrieving device decoder utilization");
  last_status = nvmlDeviceGetEncoderUtilization(
      device, &state->encoder_utilization, &unused);
  check_error(last_status, "Error retrieving device encoder utilization");
}

Gpu *gpu_init() {
  auto last_status = nvmlInit();
  if (check_error(last_status, "Error during NVML initialization")) {
    exit(EXIT_FAILURE);
  }

  nvmlDevice_t device;
  last_status = nvmlDeviceGetHandleByIndex(0, &device);
  if (check_error(last_status, "Error obtaining NVML device")) {
    exit(EXIT_FAILURE);
  }

  //
  // last_status = nvmlDeviceGetNumFans(device, &gpu_info.num_fans);
  //
  // check_error(last_status, "Error retrieving fan count");
  //
  // last_status = nvmlDeviceGetNumGpuCores(device, &gpu_info.num_gpu_cores);
  // check_error(last_status, "Error retrieving gpu core count");

  Gpu *gpu = calloc(1, sizeof(Gpu));
  gpu->handle = device;
  gpu->state = (GpuState){};
  gpu_update_state(gpu);
  return gpu;
}

const GpuState *gpu_get_state(const Gpu *gpu) { return &gpu->state; }

void gpu_update_state(Gpu *gpu) {
  /** Splitting into static and dynamic updates allows us to not waste nvml
  calls on stuff doesn't change.*/
  if (!gpu->state.initialized) {
    gpu_update_static_state(gpu);
  }
  gpu_update_dynamic_state(gpu);
}

void gpu_destroy(Gpu *gpu) {
  // TODO close nvml device connection
  free(gpu);
}
