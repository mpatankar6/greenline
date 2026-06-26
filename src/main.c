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
  tui_init();
  tui_run(gpu);
  tui_shutdown();
  gpu_destroy(gpu);
  return 0;
}
