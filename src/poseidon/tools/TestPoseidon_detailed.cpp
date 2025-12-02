#include "../client/PoseidonClient.h"
#include "../server/PoseidonServer.h"
#include "../storage/impl/ServerStorageMemory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>

using namespace std::chrono;

class PerformanceProfiler {
public:
    static void start(const std::string& name) {
        timers[name] = high_resolution_clock::now();
    }
    
    static double end(const std::string& name) {
        auto end = high_resolution_clock::now();
        auto start = timers[name];
        auto duration = duration_cast<microseconds>(end - start);
        results[name] = duration.count();
        return duration.count();
    }
    
    static void printResults() {
        std::cout << "\n========== Detailed Performance Report ==========\n";
        std::cout << std::left << std::setw(40) << "Operation" 
                  << std::right << std::setw(15) << "Time (us)" 
                  << std::setw(15) << "Percentage" << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        double total = 0;
        for (const auto& [name, time] : results) {
            total += time;
        }
        
        for (const auto& [name, time] : results) {
            std::cout << std::left << std::setw(40) << name 
                      << std::right << std::setw(15) << std::fixed << std::setprecision(2) << time
                      << std::setw(14) << std::setprecision(2) << (time / total * 100) << "%" << std::endl;
        }
        std::cout << std::string(70, '-') << std::endl;
        std::cout << std::left << std::setw(40) << "TOTAL" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << total << std::endl;
        std::cout << "==================================================\n";
    }

private:
    static std::map<std::string, high_resolution_clock::time_point> timers;
    static std::map<std::string, double> results;
};

std::map<std::string, high_resolution_clock::time_point> PerformanceProfiler::timers;
std::map<std::string, double> PerformanceProfiler::results;

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
    PerformanceProfiler::start("DataUpdate (200 records)");
    for (int i = 0; i < 200; i++) {
        PerformanceProfiler::start("Client::DataUpdate (single)");
        client.DataUpdate(meta, Poseidon_add, "abc", "file-" + std::to_string(i));
        PerformanceProfiler::end("Client::DataUpdate (single)");
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
        def_metas.push_back(meta);
    }
    PerformanceProfiler::end("DataUpdate additional (100 records)");
    
    PerformanceProfiler::start("Server::SaveBatch (additional)");
    server.SaveBatch(def_metas);
    PerformanceProfiler::end("Server::SaveBatch (additional)");

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
    
    PerformanceProfiler::printResults();
    
    core_clean();
    return 0;
}
