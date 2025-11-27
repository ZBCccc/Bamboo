#include "PoseidonServer.h"
#include "../../core/primitive.h"
#include "relic/relic_ep.h"
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <iostream>

extern "C" {
#include "unistd.h"
#include <relic/relic.h>
}

using std::string;
using std::vector;

static std::mutex lock_cells;

void PoseidonServer::Setup() { _storage->Clear(); }

void PoseidonServer::Save(const Metadata &meta) {
  EDBCell cell = {
      .tCell =
          {
              .addr = meta.addr,
              .val = meta.val,
              .lastAddr = meta.lastAddr,
              .alpha = meta.alpha,
          },
      .xCell =
          {
              .L = meta.L,
              .D = meta.D,
              .C = meta.C,
          },
  };

  _storage->Put(cell);
}

void PoseidonServer::SaveBatch(const std::vector<Metadata> &Metadatas) {
  for (const auto &meta : Metadatas) {
    Save(meta);
  }
}

void PoseidonServer::Search(std::vector<ResMetadata> &result,
                            const TrapdoorMetadata &td) {
  bn_t K, c, e, d, ord;
  ep_t e_L, e_TD, e_TC, e_C, e_D;
  unsigned char buf[128];

  core_init();
  ep_param_set(NIST_P256);

  bn_new(c);
  bn_new(e);
  bn_new(d);
  bn_new(ord);
  bn_new(K);

  ep_new(e_L);
  ep_new(e_TD);
  ep_new(e_TC);
  ep_new(e_C);
  ep_new(e_D);

  ep_curve_get_ord(ord);

  bn_read_bin(K, (const unsigned char *)td.K1.c_str(), 32);
  bn_gcd_ext(c, d, e, K, ord);

  string L, Td, Tc, rand, xtag;
  CDBCellX cell;
  for (const auto &tkl : td.TKL) {
    L = tkl.L;
    Td = tkl.TD;
    Tc = tkl.TC;
    try {
      ep_read_bin(e_TC, (const unsigned char *)Tc.c_str(), 33);
      ep_read_bin(e_TD, (const unsigned char *)Td.c_str(), 33);
    } catch (const std::exception &e) {
      std::cerr << "ep_read_bin failed!" << std::endl;
    }

    cell.L = L;
    while (this->_storage->GetX(cell)) {
      try {
        ep_read_bin(e_C, (const unsigned char *)cell.C.c_str(), 33);
        ep_read_bin(e_D, (const unsigned char *)cell.D.c_str(), 33);
      } catch (const std::exception &e) {
        std::cerr << "ep_read_bin failed!" << std::endl;
      }
      ep_sub(e_D, e_D, e_TD);
      ep_mul(e_D, e_D, d);
      pi_inv(rand, e_C);

      ep_sub(e_C, e_C, e_TC);
      ep_write_bin(buf, 33, e_C, 1);
      xtag.assign((const char *)buf, 33);

      _storage->PutXSet(xtag);
      _storage->PopX(cell);

      Hash_H1(e_L, rand);
      ep_mul(e_L, e_L, K);
      ep_write_bin(buf, 33, e_L, 1);
      L.assign((const char *)buf, 33);
      cell.L = L;

      Hash_H2(e_TD, rand);
      ep_mul(e_TD, e_TD, K);
      ep_write_bin(buf, 33, e_TD, 1);
      Td.assign((const char *)buf, 33);
      
      Hash_G1(e_TC, rand);
      ep_mul(e_TC, e_TC, K);
      ep_write_bin(buf, 33, e_TC, 1);
      Tc.assign((const char *)buf, 33);
    }
  }

  int j = td.XTKL.size();
  string addr, valTrap, alphaTrap, lastTrap;
  addr = td.STKL[0], 
  valTrap = td.STKL[1],
  alphaTrap = td.STKL[2],
  lastTrap = td.STKL[3];
  ep_t e_valTrap, e_alphaTrap, e_lastTrap;
  ep_new(e_valTrap);
  ep_new(e_alphaTrap);
  ep_new(e_lastTrap);

  try {
    ep_read_bin(e_valTrap, (const unsigned char *)valTrap.c_str(), 33);
    ep_read_bin(e_alphaTrap, (const unsigned char *)alphaTrap.c_str(), 33);
    ep_read_bin(e_lastTrap, (const unsigned char *)lastTrap.c_str(), 33);
  } catch (const std::exception &e) {
    std::cerr << "ep_read_bin failed!" << std::endl;
  }

  CDBCellT cellT;
  cellT.addr = addr;

  string val, lastAddr, alpha;
  ep_t e_addr, e_val, e_lastAddr, e_alpha;
  ep_new(e_addr);
  ep_new(e_val);
  ep_new(e_lastAddr);
  ep_new(e_alpha);

  int cnt;
  int n = td.XTKL.size()+1;
  while (_storage->GetT(cellT)) {
    val = cellT.val;
    lastAddr = cellT.lastAddr;
    alpha = cellT.alpha;

    try {
      ep_read_bin(e_val, (const unsigned char *)val.c_str(), 33);
      ep_read_bin(e_lastAddr, (const unsigned char *)lastAddr.c_str(), 33);
      ep_read_bin(e_alpha, (const unsigned char *)alpha.c_str(), 33);
    } catch (const std::exception &e) {
      std::cerr << "ep_read_bin failed!" << std::endl;
    }

    ep_sub(e_alpha, e_alpha, e_alphaTrap);
    cnt = 1;

    string xtk, xtagjk;
    ep_t e_xtk;
    ep_new(e_xtk);
    for (int k = 2; k <= n; k++) {
      xtk = td.XTKL[j-1][k-2];
      try {
        ep_read_bin(e_xtk, (const unsigned char *)xtk.c_str(), 33);
      } catch (const std::exception &e) {
        std::cerr << "ep_read_bin failed!" << std::endl;
      }
      ep_add(e_xtk, e_xtk, e_alpha);
      ep_write_bin(buf, 33, e_xtk, 1);
      xtagjk.assign((const char *)buf, 33);
      if (_storage->GetXSet(xtagjk)) {
        cnt++;
      }
    }

    ep_sub(e_val, e_val, e_valTrap);
    ep_write_bin(buf, 33, e_val, 1);
    val.assign((const char *)buf, 33);
    result.emplace_back(ResMetadata{.val = val, .cnt = cnt});

    string tk;
    ep_sub(e_lastAddr, e_lastAddr, e_lastTrap);
    ep_mul(e_lastAddr, e_lastAddr, d);
    pi_inv(tk, e_lastAddr);

    Hash_H1(e_addr, tk);
    ep_mul(e_addr, e_addr, K);
    ep_write_bin(buf, 33, e_addr, 1);
    addr.assign((const char *)buf, 33);
    cellT.addr = addr;

    Hash_G1(e_valTrap, tk);
    ep_mul(e_valTrap, e_valTrap, K);

    Hash_G2(e_alphaTrap, tk);
    ep_mul(e_alphaTrap, e_alphaTrap, K);

    Hash_H2(e_lastTrap, tk);
    ep_mul(e_lastTrap, e_lastTrap, K);

    j--;
  }

  bn_free(c);
  bn_free(e);
  bn_free(d);
  bn_free(ord);
  bn_free(K);
  ep_free(e_L);
  ep_free(e_TD);
  ep_free(e_TC);
  ep_free(e_C);
  ep_free(e_D);
  ep_free(e_val);
  ep_free(e_lastAddr);
  ep_free(e_alpha);
  ep_free(e_valTrap);
  ep_free(e_alphaTrap);
  ep_free(e_lastTrap);
  ep_free(e_addr);
  ep_free(e_xtk);

  return;
}

void PoseidonServer::KeyUpdate(const string &token) {
  vector<CDBCellT> ciphers;
  bn_t d;
  ep_t ele1;
  unsigned char buf[128];

  bn_new(d);
  ep_new(ele1);

  bn_read_bin(d, (const unsigned char *)token.c_str(), 32);

  _storage->PopAllT(ciphers);

  for (CDBCellT &cell : ciphers) {
    ep_read_bin(ele1, (const unsigned char *)cell.addr.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.addr.assign((const char *)buf, 33);

    ep_read_bin(ele1, (const unsigned char *)cell.val.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.val.assign((char *)buf, 33);

    ep_read_bin(ele1, (const unsigned char *)cell.lastAddr.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.lastAddr.assign((char *)buf, 33);

    ep_read_bin(ele1, (const unsigned char *)cell.alpha.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.alpha.assign((char *)buf, 33);
  }

  _storage->PushBatchT(ciphers);

  vector<CDBCellX> xCells;
  _storage->PopAllX(xCells);

  for (CDBCellX &cell : xCells) {
    ep_read_bin(ele1, (const unsigned char *)cell.L.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.L.assign((const char *)buf, 33);

    ep_read_bin(ele1, (const unsigned char *)cell.D.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.D.assign((const char *)buf, 33);

    ep_read_bin(ele1, (const unsigned char *)cell.C.c_str(), 33);
    ep_mul(ele1, ele1, d);
    ep_write_bin(buf, 33, ele1, 1);
    cell.C.assign((const char *)buf, 33);
  }

  _storage->PushBatchX(xCells);

  bn_free(d);
  ep_free(ele1);

  return;
}

void PoseidonServer::DumpData(const std::string &name) {
  _storage->DumpData(name);
}

void PoseidonServer::LoadData(const std::string &name) {
  _storage->LoadData(name);
}
