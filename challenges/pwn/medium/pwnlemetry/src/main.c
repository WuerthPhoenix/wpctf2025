#include "client.h"
#include "server.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
  bool run_server = false;
  const char *mon_data = "monitors.dat";
  uint16_t port = DEFAULT_PORT;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--server") == 0)
      run_server = true;
    else if (strncmp(argv[i], "--monitors=", 11) == 0)
      mon_data = argv[i] + 11;
    else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      port = (uint16_t)atoi(argv[++i]);
    } else {
      fprintf(stderr, "Unknown arg: %s\n", argv[i]);
    }
  }

  srand(time(NULL));

  if (run_server) {
    start_server(mon_data, port);
    return 0;
  }

  // default: client mode (interactive REPL) unless --no-client
  start_client(port);
  return 0;
}
