#include "ServerStorageMemory.h"
#include <fstream>
#include <iostream>

using namespace std;

ServerStorageMemory::ServerStorageMemory() {}

ServerStorageMemory::~ServerStorageMemory() {}

void ServerStorageMemory::Clear() { storage_.clear(); }

bool ServerStorageMemory::Get(EDBCell &in_out) {
  auto it = storage_.find(in_out.L);
  if (it != storage_.end()) {
    in_out.D = it->second.D;
    in_out.C = it->second.C;
    return true;
  }
  return false;
}

void ServerStorageMemory::Put(const EDBCell &in) { storage_[in.L] = in; }

void ServerStorageMemory::PopAll(std::vector<EDBCell> &cip_all) {
  for (const auto &pair : storage_) {
    cip_all.push_back(pair.second);
  }
  storage_.clear();
}

void ServerStorageMemory::PushBatch(const std::vector<EDBCell> &cip_all) {
  for (const auto &cell : cip_all) {
    storage_[cell.L] = cell;
  }
}

void ServerStorageMemory::DumpData(const std::string &dname) {
  ofstream file(dname + ".mem", ios::binary);
  if (!file.is_open()) {
    cout << "DumpData error: cannot open file " << dname << ".mem" << endl;
    return;
  }

  for (const auto &pair : storage_) {
    const EDBCell &cell = pair.second;
    uint32_t l_size = static_cast<uint32_t>(cell.L.size());
    uint32_t d_size = static_cast<uint32_t>(cell.D.size());
    uint32_t c_size = static_cast<uint32_t>(cell.C.size());
    file.write(reinterpret_cast<const char *>(&l_size), sizeof(l_size));
    file.write(reinterpret_cast<const char *>(&d_size), sizeof(d_size));
    file.write(reinterpret_cast<const char *>(&c_size), sizeof(c_size));
    if (l_size > 0)
      file.write(cell.L.data(), l_size);
    if (d_size > 0)
      file.write(cell.D.data(), d_size);
    if (c_size > 0)
      file.write(cell.C.data(), c_size);
  }
  file.close();
}

void ServerStorageMemory::LoadData(const std::string &dname) {
  ifstream file(dname + ".mem", ios::binary);
  if (!file.is_open()) {
    cout << "LoadData error: cannot open file " << dname << ".mem" << endl;
    return;
  }

  storage_.clear();
  while (file) {
    uint32_t l_size = 0, d_size = 0, c_size = 0;
    file.read(reinterpret_cast<char *>(&l_size), sizeof(l_size));
    if (file.fail())
      break;
    file.read(reinterpret_cast<char *>(&d_size), sizeof(d_size));
    if (file.fail())
      break;
    file.read(reinterpret_cast<char *>(&c_size), sizeof(c_size));
    if (file.fail())
      break;

    EDBCell cell;
    cell.L.resize(l_size);
    cell.D.resize(d_size);
    cell.C.resize(c_size);

    if (l_size > 0)
      file.read(cell.L.data(), l_size);
    if (d_size > 0)
      file.read(cell.D.data(), d_size);
    if (c_size > 0)
      file.read(cell.C.data(), c_size);

    if (file.fail())
      break;

    storage_[cell.L] = cell;
  }
  file.close();
}