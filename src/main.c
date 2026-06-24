#include "gpu.h"
#include "tui.h"
#include <curses.h>
#include <nvml.h>
#include <stdio.h>

static int bytes_to_mib(unsigned long long bytes) {
  const int BYTES_PER_MIB = 1024 * 1024;
  return (int)(bytes / BYTES_PER_MIB);
}

int main() {
  auto gpu = gpu_init();
  auto gpu_info = gpu_get_state(gpu);
  printf("Name: %s\n", gpu_info.name);
  printf("Arch: %s\n", gpu_info.architecture);
  printf("Total VRAM: %'d MiB (Usable: %'d MiB)\n",
         bytes_to_mib(gpu_info.total_vram_bytes),
         bytes_to_mib(gpu_info.usable_vram_bytes));
  printf("Fans: %d\n", gpu_info.num_fans);
  printf("Fans: %d\n", gpu_info.num_gpu_cores);
  tui_init();
  tui_run();
  tui_shutdown();
  gpu_destroy(gpu);
  return 0;
}
