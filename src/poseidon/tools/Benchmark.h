#ifndef BENCHMARK_H
#define BENCHMARK_H
#include <map>
#include <set>
#include <string>
#include <vector>

class Benchmark {
public:
  Benchmark() = delete;
  ~Benchmark() = default;

  explicit Benchmark(const std::string &filename,
                     const std::string &_name = "dataset",
                     const std::string &_addr = "127.0.0.1", int _port = 54324);

  void prepare_dataset();

  void benchmark_test_DataUpdate();

  void benchmark_test_Search();

  void benchmark_test_delete(const std::string &keyword_to_delete);

  void benchmark_test_keyUpdate(int thread_num = 1);

private:
  void read_data_from_file_(const std::string &filename);

  void random_select_deleted_entries(
      std::vector<std::pair<std::string, std::string>> &entries,
      int data_to_delete);

  void random_select_file_identifiers_(std::vector<std::string> &ids,
                                       std::set<int> &found_index,
                                       const std::string &keyword,
                                       int num_of_id);

  void init_encrypted_database_();

  std::string global_filename;
  std::map<std::string, std::vector<std::string>> data_to_encrypt;
  std::vector<std::pair<std::string, std::string>> plane_db;
  int total_entry_num = 0;
  std::string name, server_addr;
  int server_port;
};

#endif
