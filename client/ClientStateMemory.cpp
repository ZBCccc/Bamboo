#include "ClientStateMemory.h"
#include <fstream>

using namespace std;

bool ClientStateMemory::Get(StateCell &out, const string &keyword)
{
    auto it = store_.find(keyword);
    if (it == store_.end()) return false;
    out = it->second;
    return true;
}

void ClientStateMemory::Put(const StateCell &in, const string &keyword)
{
    store_[keyword] = in;
}

void ClientStateMemory::Clear()
{
    store_.clear();
}

void ClientStateMemory::DumpData(const string &dname)
{
    ofstream out(dname + ".mem", ios::binary);
    if (!out.is_open()) return;
    uint32_t n = static_cast<uint32_t>(store_.size());
    out.write(reinterpret_cast<const char *>(&n), sizeof(n));
    for (const auto &kv : store_)
    {
        const string &key = kv.first;
        const StateCell &cell = kv.second;
        uint32_t ksz = static_cast<uint32_t>(key.size());
        uint32_t tksz = static_cast<uint32_t>(cell.tk.size());
        out.write(reinterpret_cast<const char *>(&ksz), sizeof(ksz));
        out.write(key.data(), ksz);
        out.write(reinterpret_cast<const char *>(&cell.cntw), sizeof(cell.cntw));
        out.write(reinterpret_cast<const char *>(&tksz), sizeof(tksz));
        out.write(cell.tk.data(), tksz);
    }
}

void ClientStateMemory::LoadData(const string &dname)
{
    ifstream in(dname + ".mem", ios::binary);
    if (!in.is_open()) return;
    store_.clear();
    uint32_t n = 0;
    in.read(reinterpret_cast<char *>(&n), sizeof(n));
    for (uint32_t i = 0; i < n; ++i)
    {
        uint32_t ksz = 0, tksz = 0;
        string key, tk;
        int cntw = 0;
        in.read(reinterpret_cast<char *>(&ksz), sizeof(ksz));
        key.resize(ksz);
        in.read(&key[0], ksz);
        in.read(reinterpret_cast<char *>(&cntw), sizeof(cntw));
        in.read(reinterpret_cast<char *>(&tksz), sizeof(tksz));
        tk.resize(tksz);
        in.read(&tk[0], tksz);
        store_[key] = StateCell{tk, cntw};
    }
}

void ClientStateMemory::GetKeywordsCnt(vector<int> &cnt)
{
    for (const auto &kv : store_)
    {
        cnt.emplace_back(kv.second.cntw);
    }
}