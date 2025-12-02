#include "PoseidonClientStateMemory.h"
#include <cstdint>
#include <fstream>

using namespace std;

bool PoseidonClientStateMemory::Get(StateCell &out, const string &keyword) {
  auto it = state_map.find(keyword);
  if (it == state_map.end())
    return false;
  out = it->second;
  return true;
}

void PoseidonClientStateMemory::Put(const StateCell &in,
                                    const string &keyword) {
  state_map[keyword] = in;
}

void PoseidonClientStateMemory::Clear() { state_map.clear(); }

void PoseidonClientStateMemory::DumpData(const string &dname) {
  ofstream out(dname + ".mem", ios::binary);
  if (!out.is_open())
    return;
  uint32_t n = static_cast<uint32_t>(state_map.size());
  out.write(reinterpret_cast<const char *>(&n), sizeof(n));
  for (const auto &kv : state_map) {
    const string &key = kv.first;
    const StateCell &cell = kv.second;
    uint32_t ksz = static_cast<uint32_t>(key.size());
    uint32_t tksz = static_cast<uint32_t>(cell.tk.size());
    uint32_t randsz = static_cast<uint32_t>(cell.rand.size());
    out.write(reinterpret_cast<const char *>(&ksz), sizeof(ksz));
    out.write(key.data(), ksz);
    out.write(reinterpret_cast<const char *>(&cell.cntw), sizeof(cell.cntw));
    out.write(reinterpret_cast<const char *>(&tksz), sizeof(tksz));
    out.write(cell.tk.data(), tksz);
    out.write(reinterpret_cast<const char *>(&randsz), sizeof(randsz));
    out.write(cell.rand.data(), randsz);
  }
}

void PoseidonClientStateMemory::LoadData(const string &dname) {
  ifstream in(dname + ".mem", ios::binary);
  if (!in.is_open())
    return;
  state_map.clear();
  uint32_t n = 0;
  in.read(reinterpret_cast<char *>(&n), sizeof(n));
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t ksz = 0, tksz = 0, randsz = 0;
    string key, tk, rand;
    int cntw = 0;
    in.read(reinterpret_cast<char *>(&ksz), sizeof(ksz));
    key.resize(ksz);
    in.read(&key[0], ksz);
    in.read(reinterpret_cast<char *>(&cntw), sizeof(cntw));
    in.read(reinterpret_cast<char *>(&tksz), sizeof(tksz));
    tk.resize(tksz);
    in.read(&tk[0], tksz);
    in.read(reinterpret_cast<char *>(&randsz), sizeof(randsz));
    rand.resize(randsz);
    in.read(&rand[0], randsz);
    state_map[key] = StateCell{tk, rand, cntw};
  }
}

void PoseidonClientStateMemory::GetKeywordsCnt(vector<int> &cnt) {
  for (const auto &kv : state_map) {
    cnt.emplace_back(kv.second.cntw);
  }
}