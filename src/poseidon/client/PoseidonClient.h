#ifndef PoseidonCLIENT_H
#define PoseidonCLIENT_H

#include <gmpxx.h>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
};

#include "../common/metadata.h"
#include "PoseidonClientState.h"

#define USINGCONSTPAD
#define Fpad 310000

enum PoseidonOp { Poseidon_add = 0, Poseidon_del };

class PoseidonClient {
public:
  PoseidonClient();

  ~PoseidonClient();

  int Setup();

  int DataUpdate(Metadata &meta, PoseidonOp op, const std::string &keyword,
                 const std::string &id);

  int Trapdoor(TrapdoorMetadata &td, const std::vector<std::string> &keywords);

  int DecryptResult(std::vector<std::string> &plain_out,
                    const std::vector<ResMetadata> &res_in,
                    const std::string &keyword, const int n);

  int KeyUpdate(std::string &Delta);

  void DumpData(const std::string &filename = "bamboo_client_bak_dat.db");

  void LoadData(const std::string &filename = "bamboo_client_bak_dat.db");

  void BatchDataUpdate(std::vector<Metadata> &Metadatas,
                       const std::string &keyword,
                       const std::vector<std::string> &ids,
                       PoseidonOp op = Poseidon_add);

private:
  bn_t K1, K2, Kx, Ky, Kz;
  PoseidonClientState state;

  constexpr int SrchPadConst() { return Fpad; }

  int SrchPadExp();
};

#endif
