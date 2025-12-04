#include "../client/BambooClient.h"
#include "../server/BambooServer.h"
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
              << "\n";
    std::cout << std::string(92, '-') << "\n";

    double total = 0;
    for (const auto &[name, time] : results) {
      // Only sum up "Client::" and "Server::" operations to avoid double
      // counting parent/child scopes or just sum everything. The user's
      // original code summed everything. Let's stick to summing everything for
      // percentage calculation to match previous behavior, although it might be
      // misleading if scopes overlap.
      total += time;
    }

    for (const auto &[name, time] : results) {
      double avg = counts[name] > 0 ? time / counts[name] : 0;
      std::cout << std::left << std::setw(40) << name << std::right
                << std::setw(15) << std::fixed << std::setprecision(2) << time
                << std::setw(10) << counts[name] << std::setw(15) << std::fixed
                << std::setprecision(2) << avg << std::setw(11)
                << std::setprecision(2)
                << (total > 0 ? (time / total * 100) : 0) << "%" << "\n";
    }
    std::cout << std::string(92, '-') << "\n";
    std::cout << std::left << std::setw(40) << "TOTAL" << std::right
              << std::setw(15) << std::fixed << std::setprecision(2) << total
              << "\n";
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

int main() {
  core_init();
  ep_param_set(NIST_P256);

  std::cout << "Test starting with detailed profiling..." << std::endl;

  // Server setup
  PerformanceProfiler::start("Server::Setup");
  BambooServer server;
  auto storage = std::make_unique<ServerStorageMemory>();
  server.SetStorage(std::move(storage));
  server.Setup();
  PerformanceProfiler::end("Server::Setup");

  // Client setup
  PerformanceProfiler::start("Client::Setup");
  BambooClient client;
  client.Setup();
  PerformanceProfiler::end("Client::Setup");

  std::vector<std::string> Ls, Ds, Cs;
  Ls.reserve(300);
  Ds.reserve(300);
  Cs.reserve(300);

  // DataUpdate phase
  PerformanceProfiler::start("DataUpdate (200 records)");
  for (int i = 0; i < 200; i++) {
    std::string L, D, C;
    PerformanceProfiler::start("Client::DataUpdate (single)");
    client.DataUpdate(L, D, C, Bamboo_add, "abc", "file-" + std::to_string(i));
    PerformanceProfiler::end("Client::DataUpdate (single)");
    Ls.emplace_back(std::move(L));
    Ds.emplace_back(std::move(D));
    Cs.emplace_back(std::move(C));
  }
  PerformanceProfiler::end("DataUpdate (200 records)");

  // SaveBatch
  PerformanceProfiler::start("Server::SaveBatch");
  server.SaveBatch(Ls, Ds, Cs);
  PerformanceProfiler::end("Server::SaveBatch");

  Ls.clear();
  Ds.clear();
  Cs.clear();

  PerformanceProfiler::start("DataUpdate additional (100 records)");
  for (int i = 0; i < 100; i++) {
    std::string L, D, C;
    PerformanceProfiler::start("Client::DataUpdate (single)");
    client.DataUpdate(L, D, C, Bamboo_add, "def", "file-" + std::to_string(i));
    PerformanceProfiler::end("Client::DataUpdate (single)");
    Ls.emplace_back(std::move(L));
    Ds.emplace_back(std::move(D));
    Cs.emplace_back(std::move(C));
  }
  PerformanceProfiler::end("DataUpdate additional (100 records)");

  PerformanceProfiler::start("Server::SaveBatch (additional)");
  server.SaveBatch(Ls, Ds, Cs);
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
  std::string K_out, L, MskD, MskC;
  int cnt_w = 0;
  client.Trapdoor(K_out, L, MskD, MskC, "def", cnt_w);
  PerformanceProfiler::end("Client::Trapdoor");

  PerformanceProfiler::start("Server::Search");
  std::vector<std::string> res;
  server.Search(res, K_out, L, MskD, MskC);
  PerformanceProfiler::end("Server::Search");

  PerformanceProfiler::start("Client::DecryptResult");
  std::vector<std::string> plain_out;
  client.DecryptResult(plain_out, res, "def");
  PerformanceProfiler::end("Client::DecryptResult");

  // Print results
  std::cout << "\nSearch found " << plain_out.size() << " records" << std::endl;

  PerformanceProfiler::printResults();

  core_clean();
  return 0;
}
