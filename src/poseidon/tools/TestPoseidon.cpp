#include "../client/PoseidonClient.h"
#include "../server/PoseidonServer.h"
#include "../storage/impl/ServerStorageMemory.h"
#include <chrono>
#include <iostream>

int main() {
  std::chrono::steady_clock::time_point begin, end;
  core_init();
  ep_param_set(NIST_P256);

  std::cout << "Test starting..." << std::endl;
  PoseidonServer server;
  auto storage = std::make_unique<ServerStorageMemory>();
  server.SetStorage(std::move(storage));

  server.Setup();

  PoseidonClient client;
  client.Setup();

  std::vector<Metadata> metas;
  Metadata meta;
  metas.reserve(200);
  
  begin = std::chrono::steady_clock::now();
  for (int i = 0; i < 200; i++) {
    client.DataUpdate(meta, Poseidon_add, "abc", "file-" + std::to_string(i));
    metas.push_back(meta);
  }
  end = std::chrono::steady_clock::now();

  std::chrono::duration<double, std::micro> elapsed = end - begin;
  std::cout << "Encryption with op = Poseidon_add time cost: " << std::endl;
  std::cout << "\tTotally " << 200 << " records, total " << elapsed.count()
            << " us" << std::endl;
  std::cout << "\taverage time " << elapsed.count() / 200 << " us" << std::endl
            << std::endl;

  server.SaveBatch(metas);

  std::vector<Metadata> def_metas;
  def_metas.reserve(100);

  for (int i = 0; i < 100; i++) {
    client.DataUpdate(meta, Poseidon_add, "def", "file-" + std::to_string(i));
    def_metas.push_back(meta);
  }
  server.SaveBatch(def_metas);

  // def_metas.clear();
  // def_metas.reserve(50);
  // for (int i = 0; i < 50; i++) {
  //   client.DataUpdate(meta, Poseidon_del, "def", "file-" +
  //   std::to_string(i)); def_metas.push_back(meta);
  // }
  // server.SaveBatch(def_metas);
  // std::cout << "Encrypted " << 50 << " ciphers for def" << std::endl;

  begin = std::chrono::steady_clock::now();
  TrapdoorMetadata td;
  client.Trapdoor(td, std::vector<std::string>({"def", "abc"}));
  end = std::chrono::steady_clock::now();
  auto elapsed1 = end - begin;

  begin = std::chrono::steady_clock::now();
  std::vector<ResMetadata> res;
  server.Search(res, td);
  end = std::chrono::steady_clock::now();
  auto elapsed2 = end - begin;

  std::vector<std::string> plain_out;
  begin = std::chrono::steady_clock::now();
  client.DecryptResult(plain_out, res, "def", 2);
  end = std::chrono::steady_clock::now();
  auto elapsed3 = end - begin;

  auto bench_clnt_time = elapsed1.count() + elapsed3.count();
  auto total_time = bench_clnt_time + elapsed2.count();
  std::cout << "Searching with Poseidon for keyword: " << "abc and def"
            << std::endl;
  std::cout << "\tTotally find " << plain_out.size()
            << " records and the last file ID is "
            << plain_out[plain_out.size() - 1] << std::endl;
  std::cout << "\tTime cost of client is " << std::fixed << bench_clnt_time
            << " us, average is " << bench_clnt_time / plain_out.size() << " us"
            << std::endl;
  std::cout << "\tTime cost of the whole search phase is " << std::fixed
            << total_time << " us" << std::endl;
  std::cout << "\tAverage time cost is " << std::fixed
            << total_time / plain_out.size() << " us" << std::endl;
  core_clean();
  return 0;
}
