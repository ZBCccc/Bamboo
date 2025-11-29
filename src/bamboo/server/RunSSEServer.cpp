#include "SSEServer.h"
#include <gmpxx.h>
#include <iostream>
extern "C" {
#include <relic/relic.h>
}

using std::cout;
using std::endl;

void RunServer() {
  SSEServer sse_server("127.0.0.1", 54324);
  sse_server.Run();
}

int main(int argc, char *argv[]) {
  core_init();
  ep_param_set(NIST_P256);

  RunServer();

  core_clean();
  return 0;
}