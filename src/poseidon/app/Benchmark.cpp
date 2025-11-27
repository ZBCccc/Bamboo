#include "Benchmark.h"
#include "../client/PoseidonClient.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <openssl/rand.h>
#include <unistd.h>
}

using namespace std;

double bench_clnt_time;
unsigned int bench_bandwidth;

Benchmark::Benchmark(const std::string &filename, const std::string &_name,
                     const std::string &_addr, int _port) {
  read_data_from_file_(filename);
  this->name = _name;
  this->server_addr = _addr;
  this->server_port = _port;
}

void Benchmark::read_data_from_file_(const std::string &filename) {
  global_filename = filename;
  std::ifstream file_in(filename);
  if (!file_in.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  size_t keyword_num;
  if (!(file_in >> keyword_num)) {
    throw std::runtime_error("Invalid file format: cannot read keyword count");
  }

  total_entry_num = 0;
  data_to_encrypt.clear();
  plane_db.clear();
  plane_db.reserve(keyword_num * 100); // Reserve if average known

  for (size_t i = 0; i < keyword_num; ++i) {
    std::string word;
    file_in >> word;

    size_t file_num;
    if (!(file_in >> file_num)) {
      throw std::runtime_error("Invalid file format for keyword: " + word);
    }

    auto &file_ids = data_to_encrypt[word]; // Single lookup
    file_ids.reserve(file_num);

    for (size_t j = 0; j < file_num; ++j) {
      std::string id;
      file_in >> id;
      file_ids.emplace_back(id);
      plane_db.emplace_back(word, id);
    }

    total_entry_num += file_num;
  }

  file_in.close();
}

void Benchmark::random_select_deleted_entries(
    std::vector<std::pair<std::string, std::string>> &entries,
    int data_to_delete) {
  size_t n = plane_db.size();
  if (n == 0 || data_to_delete <= 0)
    return;
  int k = std::min<int>(data_to_delete, static_cast<int>(n));
  entries.reserve(entries.size() + k);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::unordered_map<int, int> map;
  map.reserve(static_cast<size_t>(k) * 2);

  for (int i = 0; i < k; ++i) {
    std::uniform_int_distribution<int> dist(i, static_cast<int>(n) - 1);
    int r = dist(gen);
    int val_r = map.count(r) ? map[r] : r;
    int val_i = map.count(i) ? map[i] : i;
    map[r] = val_i;
    entries.emplace_back(plane_db[val_r]);
  }
}

void Benchmark::benchmark_test_DataUpdate() {
  Metadata meta;
  PoseidonClient poseidon_client;
  int loop_num = 1;
  chrono::steady_clock::time_point begin, end;
  chrono::duration<double, std::micro> elapsed;
  double total_add = 0, total_del = 0;
  vector<pair<string, string>> data_to_delete;
  int num_data_to_del = 50000;

  for (int i = 0; i < loop_num; i++) {
    poseidon_client.Setup();
    begin = chrono::steady_clock::now();
    for (auto &itr : data_to_encrypt) {
      for (auto &id : itr.second) {
        poseidon_client.DataUpdate(meta, Poseidon_add, itr.first, id);
      }
    }
    end = chrono::steady_clock::now();
    elapsed = end - begin;
    total_add += elapsed.count();

    // 删除
    random_select_deleted_entries(data_to_delete, num_data_to_del);

    begin = chrono::steady_clock::now();
    for (auto &itr : data_to_delete) {
      poseidon_client.DataUpdate(meta, Poseidon_del, itr.first, itr.second);
    }
    end = chrono::steady_clock::now();
    elapsed = end - begin;
    total_del += elapsed.count();
  }

  cout << "Encryption with op = Poseidon_add time cost: " << endl;
  cout << "\tTotally " << total_entry_num << " records, total "
       << total_add / loop_num << " us" << endl;
  cout << "\taverage time " << total_add / loop_num / total_entry_num << " us"
       << endl
       << endl;

  cout << "Encryption with op = Poseidon_del time cost: " << endl;
  cout << "\tTotally " << num_data_to_del << " records, total "
       << total_del / loop_num << " us" << endl;
  cout << "\taverage time " << total_del / loop_num / num_data_to_del << " us"
       << endl
       << endl;
}
