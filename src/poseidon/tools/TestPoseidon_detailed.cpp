#include "../client/PoseidonClient.h"
#include "../server/PoseidonServer.h"
#include "../storage/impl/ServerStorageMemory.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::chrono;

class PerformanceProfiler {
public:
  static void start(const std::string &name) {
    timers[name] = high_resolution_clock::now();
  }

  static double end(const std::string &name) {
    auto end = high_resolution_clock::now();
    auto start = timers[name];
    auto duration = duration_cast<microseconds>(end - start);
    results[name] += duration.count();
    counts[name]++;
    return duration.count();
  }

  static void printResults() {
    std::cout << "\n========== Detailed Performance Report ==========\n";
    std::cout << std::left << std::setw(40) << "Operation" << std::right
              << std::setw(15) << "Total (us)" << std::setw(10) << "Count"
              << std::setw(15) << "Avg (us)" << std::setw(12) << "Percentage"
              << std::endl;
    std::cout << std::string(92, '-') << std::endl;

    double total = 0;
    for (const auto &[name, time] : results) {
      total += time;
    }

    for (const auto &[name, time] : results) {
      double avg = counts[name] > 0 ? time / counts[name] : 0;
      std::cout << std::left << std::setw(40) << name << std::right
                << std::setw(15) << std::fixed << std::setprecision(2) << time
                << std::setw(10) << counts[name] << std::setw(15) << std::fixed
                << std::setprecision(2) << avg << std::setw(11)
                << std::setprecision(2)
                << (total > 0 ? (time / total * 100) : 0) << "%" << std::endl;
    }
    std::cout << std::string(92, '-') << std::endl;
    std::cout << std::left << std::setw(40) << "TOTAL" << std::right
              << std::setw(15) << std::fixed << std::setprecision(2) << total
              << std::endl;
    std::cout << "==================================================\n";
  }

private:
  static std::unordered_map<std::string, high_resolution_clock::time_point>
      timers;
  static std::unordered_map<std::string, double> results;
  static std::unordered_map<std::string, int> counts;
};

std::unordered_map<std::string, high_resolution_clock::time_point>
    PerformanceProfiler::timers;
std::unordered_map<std::string, double> PerformanceProfiler::results;
std::unordered_map<std::string, int> PerformanceProfiler::counts;

size_t getMetadataSize(const Metadata &meta) {
  return meta.addr.size() + meta.val.size() + meta.lastAddr.size() +
         meta.alpha.size() + meta.L.size() + meta.D.size() + meta.C.size();
}

int main() {
  core_init();
  ep_param_set(NIST_P256);

  std::cout << "Test starting with detailed profiling..." << std::endl;

  // Server setup
  PerformanceProfiler::start("Server::Setup");
  PoseidonServer server;
  auto storage = std::make_unique<ServerStorageMemory>();
  server.SetStorage(std::move(storage));
  server.Setup();
  PerformanceProfiler::end("Server::Setup");

  // Client setup
  PerformanceProfiler::start("Client::Setup");
  PoseidonClient client;
  client.Setup();
  PerformanceProfiler::end("Client::Setup");

  std::vector<Metadata> metas;
  Metadata meta;
  metas.reserve(200);

  // DataUpdate phase
  size_t total_meta_size = 0;
  PerformanceProfiler::start("DataUpdate (200 records)");
  for (int i = 0; i < 200; i++) {
    PerformanceProfiler::start("Client::DataUpdate (single)");
    client.DataUpdate(meta, Poseidon_add, "abc", "file-" + std::to_string(i));
    PerformanceProfiler::end("Client::DataUpdate (single)");

    total_meta_size += getMetadataSize(meta);
    metas.push_back(meta);
  }
  PerformanceProfiler::end("DataUpdate (200 records)");

  // SaveBatch
  PerformanceProfiler::start("Server::SaveBatch");
  server.SaveBatch(metas);
  PerformanceProfiler::end("Server::SaveBatch");

  std::vector<Metadata> def_metas;
  def_metas.reserve(100);

  PerformanceProfiler::start("DataUpdate additional (100 records)");
  for (int i = 0; i < 100; i++) {
    client.DataUpdate(meta, Poseidon_add, "def", "file-" + std::to_string(i));
    total_meta_size += getMetadataSize(meta);
    def_metas.push_back(meta);
  }
  PerformanceProfiler::end("DataUpdate additional (100 records)");

  PerformanceProfiler::start("Server::SaveBatch (additional)");
  server.SaveBatch(def_metas);
  PerformanceProfiler::end("Server::SaveBatch (additional)");

  // KeyUpdate phase
  std::string delta;
  PerformanceProfiler::start("Client::KeyUpdate");
  client.KeyUpdate(delta);
  PerformanceProfiler::end("Client::KeyUpdate");

  PerformanceProfiler::start("Server::KeyUpdate");
  server.KeyUpdate(delta);
  PerformanceProfiler::end("Server::KeyUpdate");

  // Search phase
  PerformanceProfiler::start("Client::Trapdoor");
  TrapdoorMetadata td;
  client.Trapdoor(td, std::vector<std::string>({"def", "abc"}));
  PerformanceProfiler::end("Client::Trapdoor");

  PerformanceProfiler::start("Server::Search");
  std::vector<ResMetadata> res;
  server.Search(res, td);
  PerformanceProfiler::end("Server::Search");

  PerformanceProfiler::start("Client::DecryptResult");
  std::vector<std::string> plain_out;
  client.DecryptResult(plain_out, res, "def", 2);
  PerformanceProfiler::end("Client::DecryptResult");

  // Print results
  std::cout << "\nSearch found " << plain_out.size() << " records" << std::endl;
  std::cout << "Total Metadata Size: " << total_meta_size << " bytes ("
            << std::fixed << std::setprecision(2)
            << (double)total_meta_size / 1024.0 << " KB)" << std::endl;
  std::cout << "Average Metadata Size: " << std::fixed << std::setprecision(2)
            << (double)total_meta_size / 300 << " bytes ("
            << (double)total_meta_size / 300 / 1024.0 << " KB)"
            << std::endl;
  PerformanceProfiler::printResults();

  core_clean();
  return 0;
}
