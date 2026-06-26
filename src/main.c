#include "gpu.h"
#include "tui.h"
#include <curses.h>
#include <nvml.h>
#include <stdio.h>
#include <string.h>

static int bytes_to_mib(unsigned long long bytes) {
  const int BYTES_PER_MIB = 1024 * 1024;
  return (int)(bytes / BYTES_PER_MIB);
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
      puts("greenline version: " VERSION_STRING);
      return 0;
    }
    (void)fprintf(stderr, "\033[1;31merror:\033[0m Unknown argument '%s'\n\n",
                  argv[i]);
    (void)fprintf(stderr, "Usage: greenline [OPTIONS]\n");
    return 2;
  }
  auto gpu = gpu_init();
  tui_init();
  tui_run(gpu);
  tui_shutdown();
  gpu_destroy(gpu);
  return 0;
}
