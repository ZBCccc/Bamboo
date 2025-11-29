#include "./Benchmark.h"
#include "SSEClient.h"
#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::vector;

void test_client() {
  SSEClient sse_client("127.0.0.1", 54324);
  vector<std::string> result;

  sse_client.Setup();

  for (int i = 0; i < 200; i++) {
    sse_client.DataUpdate("abc", "file-" + std::to_string(i), Bamboo_add);
  }
  cout << "Encrypted " << 200 << " ciphers for abc" << endl;

  for (int i = 0; i < 100; i++) {
    sse_client.DataUpdate("def", "file-" + std::to_string(i), Bamboo_add);
  }
  cout << "Encrypted " << 100 << " ciphers for def" << endl;
  result.clear();
  sse_client.Search(result, "abc");
  for (auto &id : result)
    cout << id << endl;
  cout << "totally find " << result.size() << " ciphertexts" << endl;

  for (int i = 50; i < 100; i++) {
    sse_client.DataUpdate("def", "file-" + std::to_string(i), Bamboo_del);
  }

  cout << "Deleted " << 50 << " ciphers for def" << endl;

  sse_client.KeyUpdate();
  result.clear();
  sse_client.Search(result, "def");
  for (auto &id : result)
    cout << id << endl;
  cout << "totally find " << result.size() << " ciphertexts" << endl;

  for (int i = 50; i < 200; i++) {
    sse_client.DataUpdate("abc", "file-" + std::to_string(i), Bamboo_del);
  }
  cout << "Deleted " << 150 << " ciphers for abc" << endl;
  result.clear();
  sse_client.Search(result, "abc");
  for (auto &id : result)
    cout << id << endl;
  cout << "totally find " << result.size() << " ciphertexts" << endl;
}

void run_Benchmark() {
  Benchmark benchmark("sse_data", "sse_data", "127.0.0.1", 54324);

  cout << "Preparing data..." << endl;
  benchmark.prepare_dataset();

  cout << "Bamboo DataUpdate" << endl;
  benchmark.benchmark_test_DataUpdate();
  // for(int i=0;i<3;i++){//*******
  cout << "Bamboo Search" << endl;
  benchmark.benchmark_test_Search();
  cout << "Bamboo delete" << endl;
  // benchmark.benchmark_test_delete("fifty");
  // cout << "Bamboo KeyUpdate" << endl;
  // benchmark.benchmark_test_keyUpdate();
  //}
}

int main(int argc, char *argv[]) {
  core_init();
  ep_param_set(NIST_P256);
  // test_client();

  run_Benchmark();
  core_clean();
  return 0;
}
