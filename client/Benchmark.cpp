#include "Benchmark.h"
#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <vector>
#include <iostream>

#include "SSEClient.h"

extern "C"
{
#include <openssl/rand.h>
#include <unistd.h>
}

using namespace std;

double bench_clnt_time;
unsigned int bench_bandwidth;

Benchmark::Benchmark(const std::string &filename, const std::string &_name, const std::string &_addr, int _port)
{
    read_data_from_file_(filename);
    this->name = _name;
    this->server_addr = _addr;
    this->server_port = _port;
}

void Benchmark::read_data_from_file_(const std::string &filename)
{
    global_filename = filename;
    FILE *f_in = fopen(filename.c_str(), "r");
    int keyword_num;
    char word[256], id[256];

    total_entry_num = 0;
    data_to_encrypt.clear();

    fscanf(f_in, "%d\n", &keyword_num);
    for (int i = 0; i < keyword_num; i++)
    {
        fscanf(f_in, "%s\n", word);
        if (data_to_encrypt.find(word) == data_to_encrypt.end())
        {
            vector<string> _t;
            data_to_encrypt[word] = _t;
        }

        vector<string> &_v = data_to_encrypt[word];

        int file_num = 0;
        fscanf(f_in, "%d\n", &file_num);
        for (int j = 0; j < file_num; j++)
        {
            fscanf(f_in, "%s\n", id);
            _v.emplace_back(string(id));
            plane_db.emplace_back(pair<string, string>(string(word), string(id)));
            total_entry_num += 1;
        }
    }
    fclose(f_in);
}

void Benchmark::prepare_dataset()
{
    SSEClient sse_client(this->server_addr, this->server_port);

    int count = 0;

    sse_client.Setup();
    sse_client.prepare_dataset(data_to_encrypt);
    sse_client.BackupDB(this->name);
}

void Benchmark::benchmark_test_DataUpdate()
{
    string L, D, C;
    BambooClient bamboo_client;
    int loop_num = 1;
    chrono::steady_clock::time_point begin, end;
    chrono::duration<double, std::micro> elapsed;
    double total_add = 0, total_del = 0;
    vector<pair<string, string>> data_to_delete;
    int num_data_to_del = 50000;

    for (int i = 0; i < loop_num; i++)
    {
        bamboo_client.Setup();

        begin = chrono::steady_clock::now();
        for (auto &itr : data_to_encrypt)
        {
            for (auto &id : itr.second)
            {
                bamboo_client.DataUpdate(L, D, C, Bamboo_add, itr.first, id);
            }
        }
        end = chrono::steady_clock::now();
        elapsed = end - begin;
        total_add += elapsed.count();

        random_select_delected_entries(data_to_delete, num_data_to_del);

        begin = chrono::steady_clock::now();
        for (auto &itr : data_to_delete)
        {
            bamboo_client.DataUpdate(L, D, C, Bamboo_del, itr.first, itr.second);
        }
        end = chrono::steady_clock::now();
        elapsed = end - begin;
        total_del += elapsed.count();
    }

    cout << "Encryption with op = Bamboo_add time cost: " << endl;
    cout << "\tTotally " << total_entry_num << " records, total " << total_add / loop_num << " us" << endl;
    cout << "\taverage time " << total_add / loop_num / total_entry_num << " us" << endl
         << endl;

    cout << "Encryption with op = Bamboo_Del time cost: " << endl;
    cout << "\tTotally " << num_data_to_del << " records, total " << total_del / loop_num << " us" << endl;
    cout << "\taverage time " << total_del / loop_num / num_data_to_del << " us" << endl
         << endl;
}

void Benchmark::random_select_file_identifiers_(vector<std::string> &ids, set<int> &found_index, const string &keyword,
                                                int num_of_id)
{
    int cur_number = 0, index;

    ids.clear();

    if (num_of_id >= data_to_encrypt[keyword].size())
    {
        for (int i = 0; i < total_entry_num; i++)
            ids.emplace_back(data_to_encrypt[keyword][i]);
    }
    else
    {
        while (cur_number < num_of_id)
        {
            RAND_bytes((unsigned char *)&index, sizeof(int));
            index = index % data_to_encrypt[keyword].size();
            if (index < 0)
                index = -index;
            if (found_index.find(index) == found_index.end())
            {
                found_index.emplace(index);
                cur_number++;
                ids.emplace_back(data_to_encrypt[keyword][index]);
            }
        }
    }
}

void Benchmark::random_select_delected_entries(std::vector<pair<string, string>> &entries, int num_data_to_delete)
{
    int cur_number = 0, index;
    set<int> found_index;  //除重

    entries.clear();

    if (num_data_to_delete >= total_entry_num)
    {
        for (int i = 0; i < total_entry_num; i++)
            entries.emplace_back(plane_db[i]);
    }
    else
    {
        while (cur_number < num_data_to_delete)
        {
            RAND_bytes((unsigned char *)&index, sizeof(int));
            index = index % total_entry_num;
            if (index < 0)
                index = -index;
            if (found_index.find(index) == found_index.end())  //除重
            {
                found_index.emplace(index);
                cur_number++;
                entries.emplace_back(plane_db[index]);
            }
        }
    }
}

void Benchmark::benchmark_test_Search()
{
    chrono::steady_clock::time_point begin, end;
    chrono::duration<double, std::micro> elapsed;
    double total_time = 0;
    vector<string> plaintexts;

    cout << "Start searching..." << endl;

    for (auto &itr : data_to_encrypt)
    {
        SSEClient sse_client(this->server_addr, this->server_port);
        sse_client.Setup();
        sse_client.LoadEDB(this->name);

        plaintexts.clear();
        plaintexts.reserve(150000);
        bench_clnt_time = 0;
        total_time = 0;
        bench_bandwidth = 0;

        begin = chrono::steady_clock::now();
        sse_client.Search(plaintexts, itr.first);
        end = chrono::steady_clock::now();
        elapsed = end - begin;
        total_time = elapsed.count();

        // cout << "Searching with Bamboo for keyword: " << itr.first << endl;
        // cout << "\tTotally find " << plaintexts.size() << " records and the last file ID is "
        //      << plaintexts[plaintexts.size() - 1] << endl;
        // cout << "\tTime cost of client is " << std::fixed << bench_clnt_time << " us, average is "
        //      << bench_clnt_time / plaintexts.size() << " us" << endl;
        // cout << "\tTime cost of the whole search phase is " << fixed << total_time << " us" << endl;
        // cout << "\tAverage time cost is " << fixed << total_time / plaintexts.size() << " us" << endl;
        // cout << "\tBandwidth cost is " << bench_bandwidth << " Bytes"<< endl;
        cout << "Searching with Bamboo for keyword: " << itr.first << endl;
        cout << "\tTotally find " << plaintexts.size() << " records" << endl;
        cout <<  std::fixed << bench_clnt_time << endl;
        cout <<  fixed << total_time << endl;
        // cout <<  fixed << total_time / plaintexts.size() << endl;
    }
}

void Benchmark::benchmark_test_delete(const std::string &keyword_to_delete)
{
    chrono::steady_clock::time_point begin, end;
    chrono::duration<double, std::micro> elapsed;
    vector<string> plaintexts, id_to_del;
    set<int> id_index;
    double total_time = 0;

    cout << "Start Deleting..." << endl;

    for (int por = 0; por < 91; por += 10)
    {
        SSEClient sse_client(this->server_addr, this->server_port);
        sse_client.Setup();
        sse_client.LoadEDB(this->name);

        id_to_del.clear();
        id_index.clear();

        if (por != 0)
        {
            std::map<std::string, std::vector<std::string>> data_to_del;
            random_select_file_identifiers_(id_to_del, id_index, keyword_to_delete,
                                            int(por / 100.0 * data_to_encrypt[keyword_to_delete].size()));

            data_to_del[keyword_to_delete] = id_to_del;
            sse_client.prepare_dataset(data_to_del, Bamboo_del);
        }

        plaintexts.clear();
        plaintexts.reserve(150000);
        total_time = 0;
        bench_clnt_time = 0;
        bench_bandwidth = 0;

        begin = chrono::steady_clock::now();
        sse_client.Search(plaintexts, keyword_to_delete);
        end = chrono::steady_clock::now();
        elapsed = end - begin;
        total_time = elapsed.count();

        // cout << "Searching with Deletion with Bamboo for keyword: " << keyword_to_delete << endl;
        // cout << "Delete portion: " << por * 0.01 << ", deleted "
        //      << id_to_del.size() << endl;
        // cout << "\tTotally find " << plaintexts.size() << " records and the last file ID is "
        //      << plaintexts[plaintexts.size() - 1] << endl;
        // cout << "\tTime cost of client is " << std::fixed << bench_clnt_time << " us, average is "
        //      << bench_clnt_time / plaintexts.size() << " us" << endl;
        // cout << "\tTime cost of the whole search phase is " << fixed << total_time << " us" << endl;
        // cout << "\tAverage time cost is " << fixed << total_time / plaintexts.size() << " us" << endl;
        // cout << "\tBandwidth cost is " << bench_bandwidth << " Bytes"<< endl;
        cout << "Searching with Deletion with Bamboo for keyword: " << keyword_to_delete << endl;
        cout << "Delete portion: " << por * 0.01 << ", deleted "
             << id_to_del.size() << endl;
        cout << "\tTotally find " << plaintexts.size() << " records" << endl;
        cout <<  std::fixed << bench_clnt_time << endl;
        cout <<  fixed << total_time << endl;
        // cout <<  fixed << total_time / plaintexts.size() << endl;
    }
}

void Benchmark::benchmark_test_keyUpdate(int thread_num)
{
    chrono::steady_clock::time_point begin, end;
    chrono::duration<double, std::micro> elapsed;
    double total_time;

    int loop_num = 1;

    cout << "Start KeyUpdating..." << endl;

    bench_clnt_time = 0;
    total_time = 0;
    for (int i = 0; i < loop_num; i++)
    {
        SSEClient sse_client(this->server_addr, this->server_port);
        sse_client.Setup();
        sse_client.LoadEDB(this->name);

        begin = chrono::steady_clock::now();
        sse_client.KeyUpdate(thread_num);
        end = chrono::steady_clock::now();
        elapsed = end - begin;
        total_time = elapsed.count();

        // cout << "Performing the KeyUpdate with Deletion with Bamboo: " << endl;
        // cout << "\tTime cost of client is " << std::fixed << bench_clnt_time / loop_num << " us" << endl;
        // cout << "\tTime cost of the whole KeyUpdate phase is " << fixed << total_time / loop_num << " us" << endl;
        cout << "Performing the KeyUpdate with Deletion with Bamboo: " << endl;
        cout <<  std::fixed << bench_clnt_time / loop_num << endl;
        cout <<  fixed << total_time / loop_num << endl;
    }
}