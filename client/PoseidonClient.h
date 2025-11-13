#ifndef PoseidonCLIENT_H
#define PoseidonCLIENT_H

#include <string>
#include <vector>
#include <gmpxx.h>

extern "C"
{
#include <relic/relic.h>
};

#include "PoseidonClientState.h"

#define USINGCONSTPAD
#define Fpad 310000

enum PoseidonOp
{
    Poseidon_add = 0,
    Poseidon_del
};

class PoseidonClient
{
public:
    PoseidonClient();

    ~PoseidonClient();

    int Setup();

    int DataUpdate(std::string &L, std::string &D, std::string &C, PoseidonOp op,
                   const std::string &keyword, const std::string &id);

    int Trapdoor(std::string &K_out, std::string &L, std::string &MskD, std::string &MskC,
                 const std::string &keyword, int &cnt_w);

    int DecryptResult(std::vector<std::string> &plain_out, const std::vector<std::string> &cipher_in,
                      const std::string &keyword);

    int KeyUpdate(std::string &Delta);

    void DumpData(const std::string &filename = "bamboo_client_bak_dat.db");

    void LoadData(const std::string &filename = "bamboo_client_bak_dat.db");

    void BatchDataUpdate(std::vector<std::string> &Ls, std::vector<std::string> &Ds, std::vector<std::string> &Cs,
                         const std::string &keyword, const std::vector<std::string> &ids, PoseidonOp op = Poseidon_add);

private:
    bn_t K1, K2, Kx, Ky, Kz;
    ClientState state;

    constexpr int SrchPadConst() { return Fpad; }

    int SrchPadExp();
};

#endif
