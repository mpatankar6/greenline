#include "gpu.h"
#include <assert.h>
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

  static_assert(sizeof(state->driver_version) >=
                NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE);
  last_status = nvmlSystemGetDriverVersion(state->driver_version,
                                           sizeof(state->driver_version));
  check_error(last_status, "Error retrieving driver version");
  static_assert(sizeof(state->nvml_version) >=
                NVML_SYSTEM_NVML_VERSION_BUFFER_SIZE);
  last_status = nvmlSystemGetNVMLVersion(state->nvml_version,
                                         sizeof(state->nvml_version));
  check_error(last_status, "Error retrieving nvml version");
  int cuda_version = 0;
  last_status = nvmlSystemGetCudaDriverVersion(&cuda_version);
  if (!check_error(last_status, "Error retrieving CUDA version")) {
    (void)snprintf(state->cuda_version, sizeof(state->cuda_version), "%d.%d",
                   NVML_CUDA_DRIVER_VERSION_MAJOR(cuda_version),
                   NVML_CUDA_DRIVER_VERSION_MINOR(cuda_version));
  }

  static_assert(sizeof(state->name) >= NVML_DEVICE_NAME_V2_BUFFER_SIZE);
  last_status = nvmlDeviceGetName(device, state->name, sizeof(state->name));
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

  last_status = nvmlDeviceGetMaxPcieLinkGeneration(
      device, &state->pcie_max_link_generation);
  check_error(last_status, "Error retrieving device pcie max link gen");
  last_status =
      nvmlDeviceGetMaxPcieLinkWidth(device, &state->pcie_max_link_width);
  check_error(last_status, "Error retrieving device pcie max link width");
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

  last_status = nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS,
                                       &state->core_clock_mhz);
  check_error(last_status, "Error retrieving device graphics clock");
  last_status =
      nvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &state->memory_clock_mhz);
  check_error(last_status, "Error retrieving device graphics clock");

  // These two calls assume a fan index of 0. This is okay because consumer GPUs
  // typically report all fans under index 0 in NVML.
  last_status =
      nvmlDeviceGetFanSpeed_v2(device, 0, &state->fan_speed_percentage);
  check_error(last_status, "Error retrieving device fan speed percentage");
  nvmlFanSpeedInfo_t fan_speed_info = {.version = nvmlFanSpeedInfo_v1,
                                       .fan = 0};
  last_status = nvmlDeviceGetFanSpeedRPM(device, &fan_speed_info);
  check_error(last_status, "Error retrieving device fan speed RPM");
  state->fan_speed_rpm = fan_speed_info.speed;

  nvmlTemperature_t temperature_info = {
      .version = nvmlTemperature_v1,
      .sensorType = NVML_TEMPERATURE_GPU,
  };
  last_status = nvmlDeviceGetTemperatureV(device, &temperature_info);
  check_error(last_status, "Error retrieving device temperature");
  state->temperature_celsius = temperature_info.temperature;

  nvmlPstates_t pstate = NVML_PSTATE_UNKNOWN;
  last_status = nvmlDeviceGetPerformanceState(device, &pstate);
  check_error(last_status, "Error retrieving device pstate");
  (void)snprintf(state->performance_state, sizeof(state->performance_state),
                 pstate == NVML_PSTATE_UNKNOWN ? "?" : "P%d", pstate);
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
    gpu->state.initialized = true;
  }
  gpu_update_dynamic_state(gpu);
}

void gpu_destroy(Gpu *gpu) {
  auto shutdown_status = nvmlShutdown();
  check_error(shutdown_status, "Error shutting down NVML");
  free(gpu);
}
