#include "ServerStorageMemory.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

ServerStorageMemory::ServerStorageMemory() {}

ServerStorageMemory::~ServerStorageMemory() {}

void ServerStorageMemory::Clear() {
  tStorage_.clear();
  xStorage_.clear();
  xSet_.clear();
}

bool ServerStorageMemory::GetT(CDBCellT &in_out) {
  auto it = tStorage_.find(in_out.addr);
  if (it != tStorage_.end()) {
    in_out.val = it->second.val;
    in_out.lastAddr = it->second.lastAddr;
    in_out.alpha = it->second.alpha;
    return true;
  }
  return false;
}

bool ServerStorageMemory::GetX(CDBCellX &in_out) {
  auto it = xStorage_.find(in_out.L);
  if (it != xStorage_.end()) {
    in_out.D = it->second.D;
    in_out.C = it->second.C;
    return true;
  }
  return false;
}

void ServerStorageMemory::Put(const EDBCell &in) {
  tStorage_[in.tCell.addr] = in.tCell;
  xStorage_[in.xCell.L] = in.xCell;
}

void ServerStorageMemory::PutXSet(const string &xtag) { xSet_.insert(xtag); }

void ServerStorageMemory::PopAllT(std::vector<CDBCellT> &cip_all) {
  cip_all.clear();
  cip_all.reserve(tStorage_.size());
  for (const auto &pair : tStorage_) {
    cip_all.emplace_back(pair.second);
  }
  tStorage_.clear();
}

void ServerStorageMemory::PopX(CDBCellX &in_out) {
  auto it = xStorage_.find(in_out.L);
  if (it != xStorage_.end()) {
    in_out.D = it->second.D;
    in_out.C = it->second.C;
    xStorage_.erase(it);
  }
}

bool ServerStorageMemory::GetXSet(const std::string &xtag) {
  return xSet_.find(xtag) != xSet_.end();
}

void ServerStorageMemory::PopAllX(std::vector<CDBCellX> &cip_all) {
  cip_all.clear();
  cip_all.reserve(xStorage_.size());
  for (const auto &pair : xStorage_) {
    cip_all.emplace_back(pair.second);
  }
  xStorage_.clear();
}

void ServerStorageMemory::PopAllXSet(std::vector<std::string> &xtags) {
  xtags.clear();
  xtags.reserve(xSet_.size());
  xtags.assign(xSet_.begin(), xSet_.end());
  xSet_.clear();
}

void ServerStorageMemory::PushBatchT(const std::vector<CDBCellT> &cip_all) {
  for (const auto &cell : cip_all) {
    tStorage_[cell.addr] = cell;
  }
}

void ServerStorageMemory::PushBatchX(const std::vector<CDBCellX> &cip_all) {
  for (const auto &cell : cip_all) {
    xStorage_[cell.L] = cell;
  }
}

void ServerStorageMemory::DumpData(const std::string &dname) {
  ofstream tfile(dname + ".t", ios::binary);
  ofstream xfile(dname + ".x", ios::binary);

  for (const auto &pair : tStorage_) {
    tfile.write(pair.second.addr.c_str(), 33);
    tfile.write(pair.second.val.c_str(), 33);
    tfile.write(pair.second.lastAddr.c_str(), 33);
    tfile.write(pair.second.alpha.c_str(), 33);
  }

  for (const auto &pair : xStorage_) {
    xfile.write(pair.second.L.c_str(), 33);
    xfile.write(pair.second.D.c_str(), 33);
    xfile.write(pair.second.C.c_str(), 33);
  }

  tfile.close();
  xfile.close();
}

void ServerStorageMemory::LoadData(const std::string &dname) {}