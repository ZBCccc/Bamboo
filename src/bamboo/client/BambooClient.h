#ifndef BambooCLIENT_H
#define BambooCLIENT_H

#include <gmpxx.h>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
};

#include "ClientState.h"

#define USINGCONSTPAD
#define Fpad 310000

enum BambooOp { Bamboo_add = 0, Bamboo_del };

class BambooClient {
public:
  BambooClient();

  ~BambooClient();

  int Setup();

  int DataUpdate(std::string &L, std::string &D, std::string &C, BambooOp op,
                 const std::string &keyword, const std::string &id);

  int Trapdoor(std::string &K_out, std::string &L, std::string &MskD,
               std::string &MskC, const std::string &keyword, int &cnt_w);

  int DecryptResult(std::vector<std::string> &plain_out,
                    const std::vector<std::string> &cipher_in,
                    const std::string &keyword);

  int KeyUpdate(std::string &Delta);

  void DumpData(const std::string &filename = "bamboo_client_bak_dat.db");

  void LoadData(const std::string &filename = "bamboo_client_bak_dat.db");

  void BatchDataUpdate(std::vector<std::string> &Ls,
                       std::vector<std::string> &Ds,
                       std::vector<std::string> &Cs, const std::string &keyword,
                       const std::vector<std::string> &ids,
                       BambooOp op = Bamboo_add);

private:
  bn_t K, K1;
  std::unique_ptr<ClientState> state;

  constexpr int SrchPadConst() { return Fpad; }

  int SrchPadExp();
};

#endif
