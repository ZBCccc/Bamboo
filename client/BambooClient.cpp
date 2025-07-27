#include "BambooClient.h"
#include "../primitive.h"
#include <set>
#include <iostream>

extern "C"
{
#include <openssl/rand.h>
}

using namespace std;

BambooClient::BambooClient()
{
    bn_new(K);
    bn_new(K1);
}

BambooClient::~BambooClient()
{
    bn_clean(K);
    bn_clean(K1);
}

int BambooClient::Setup()
{
    bn_t ord;   // 大数类型，椭圆曲线的阶

    bn_new(ord);
    ep_curve_get_ord(ord);  // 获取当前椭圆曲线的阶
    bn_rand_mod(K, ord);
    bn_rand_mod(K1, ord);

    this->state.Clear();

    bn_clean(ord);

    return 0;
}

int BambooClient::DataUpdate(std::string &L, std::string &D, std::string &C, BambooOp op, const std::string &keyword,
                             const std::string &id)
{
    StateCell cell;
    unsigned char buf1[64];
    string tk1, s;
    ep_t e_L, e_D, e_C, e_tmp;

    ep_new(e_L);
    ep_new(e_D);
    ep_new(e_C);
    ep_new(e_tmp);

    if (!this->state.Get(cell, keyword))
    {
        RAND_bytes(buf1, 16);
        cell.tk.assign((char *)buf1, 16);
        cell.cntw = 0;
    }
    cell.cntw += 1;

    RAND_bytes(buf1, 16);
    tk1.assign((char *)buf1, 16);

    Hash_H1(e_L, tk1);
    ep_mul(e_L, e_L, this->K);

    pi(e_D, cell.tk);
    Hash_H2(e_tmp, tk1);
    ep_add(e_D, e_tmp, e_D);
    ep_mul(e_D, e_D, this->K);

    Hash_G(e_C, tk1);
    if (op == Bamboo_add)
        s = "1" + id;
    else
        s = "0" + id;
    pi(e_tmp, s);
    ep_mul(e_tmp, e_tmp, this->K1);
    ep_mul(e_C, e_C, this->K);
    ep_add(e_C, e_C, e_tmp);

    ep_write_bin(buf1, 33, e_L, 1);
    L.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_D, 1);
    D.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_C, 1);
    C.assign((char *)buf1, 33);

    cell.tk = tk1;
    this->state.Put(cell, keyword);
    ep_free(e_L);
    ep_free(e_D);
    ep_free(e_C);
    ep_free(e_tmp);

    return 0;
}

int BambooClient::Trapdoor(std::string &K_out, std::string &L, std::string &MskD, std::string &MskC,
                           const std::string &keyword, int &cnt_w)
{
    unsigned char buf[64];
    StateCell cell;
    string s;
    int mid;
    ep_t e_L, e_MskC, e_tmp, e_MskD;

    if (!this->state.Get(cell, keyword))
        return -1;

    ep_new(e_L);
    ep_new(e_MskC);
    ep_new(e_tmp);
    ep_new(e_MskD);

    s = cell.tk;
    Hash_H1(e_L, s);
    ep_mul(e_L, e_L, this->K);

    Hash_H2(e_MskD, s);
    ep_mul(e_MskD, e_MskD, this->K);

    Hash_G(e_MskC, s);
    ep_mul(e_MskC, e_MskC, this->K);

    bn_write_bin(buf, 32, this->K);
    K_out.assign((char *)buf, 32);

    ep_write_bin(buf, 33, e_L, 1);
    L.assign((char *)buf, 33);

    ep_write_bin(buf, 33, e_MskD, 1);
    MskD.assign((char *)buf, 33);

    ep_write_bin(buf, 33, e_MskC, 1);
    MskC.assign((char *)buf, 33);

    cnt_w = cell.cntw;

    ep_free(e_L);
    ep_free(e_MskC);
    ep_free(e_tmp);
    ep_free(e_MskD);

    return 0;
}

int BambooClient::DecryptResult(std::vector<std::string> &plain_out, const std::vector<std::string> &cipher_in,
                                const std::string &keyword)
{
    ep_t ele;
    bn_t c, d, e, ord;
    unsigned char buf[64];
    string s1, s2;
    set<string> tmp;
    StateCell cell;

    if (!this->state.Get(cell, keyword))
        return -1;

    ep_new(ele);
    bn_new(c);
    bn_new(e);
    bn_new(d);
    bn_new(ord);

    ep_curve_get_ord(ord);
    bn_gcd_ext(c, d, e, this->K1, ord);

    for (int i = cell.cntw - 1; i >= 0; i--)
    {
        ep_read_bin(ele, (const unsigned char *)cipher_in[i].c_str(), 33);
        ep_mul(ele, ele, d);
        pi_inv(s1, ele);

        // 检查pi_inv是否成功返回有效字符串
        if (s1.empty() || s1.size() < 1)
        {
            cerr << "Warning: pi_inv returned invalid string, skipping entry" << endl;
            continue;
        }

        s2.assign(s1.begin() + 1, s1.end());
        if (s1[0] == '1')
            tmp.emplace(s2);
        else
            tmp.erase(s2);
    }

    for (auto &itr : tmp)
        plain_out.emplace_back(itr);

    ep_free(ele);
    bn_free(c);
    bn_free(e);
    bn_free(d);
    bn_free(ord);

    return 0;
}

int BambooClient::KeyUpdate(std::string &Delta)
{
    bn_t delta, ord;
    unsigned char buf[64];

    bn_new(delta);
    bn_new(ord);

    ep_curve_get_ord(ord);
    bn_rand_mod(delta, ord);

    bn_write_bin(buf, 32, delta);
    Delta.assign((char *)buf, 32);

    bn_mul(this->K, delta, this->K);
    bn_mod(this->K, this->K, ord);

    bn_mul(this->K1, delta, this->K1);
    bn_mod(this->K1, this->K1, ord);

    bn_free(delta);
    bn_free(ord);

    return 0;
}

void BambooClient::DumpData(const std::string &filename)
{
    unsigned char buf[64];
    FILE *fkey;

    this->state.DumpData(filename + ".db");

    fkey = fopen((filename + "_priv_key").c_str(), "wb");
    bn_write_bin(buf, 32, K);
    fwrite(buf, sizeof(char), 32, fkey);
    bn_write_bin(buf, 32, K1);
    fwrite(buf, sizeof(char), 32, fkey);
    fclose(fkey);
}

void BambooClient::LoadData(const std::string &filename)
{
    unsigned char buf[64];
    FILE *fkey;

    this->state.LoadData(filename + ".db");

    fkey = fopen((filename + "_priv_key").c_str(), "rb");
    fread(buf, sizeof(char), 32, fkey);
    bn_read_bin(K, buf, 32);
    fread(buf, sizeof(char), 32, fkey);
    bn_read_bin(K1, buf, 32);

    fclose(fkey);
}

void BambooClient::BatchDataUpdate(vector<std::string> &Ls, vector<std::string> &Ds, vector<std::string> &Cs,
                                   const string &keyword, const vector<std::string> &ids, BambooOp op)
{
    StateCell cell;
    unsigned char buf1[64];
    string tk1, s, L, D, C;
    ep_t e_L, e_D, e_C, e_tmp;

    ep_new(e_L);
    ep_new(e_D);
    ep_new(e_C);
    ep_new(e_tmp);

    if (!this->state.Get(cell, keyword))
    {
        RAND_bytes(buf1, 16);
        cell.tk.assign((char *)buf1, 16);
        cell.cntw = 0;
    }

    for (const auto &itr : ids)
    {
        RAND_bytes(buf1, 16);
        tk1.assign((char *)buf1, 16);

        Hash_H1(e_L, tk1);
        ep_mul(e_L, e_L, this->K);

        pi(e_D, cell.tk);
        Hash_H2(e_tmp, tk1);
        ep_add(e_D, e_tmp, e_D);
        ep_mul(e_D, e_D, this->K);

        Hash_G(e_C, tk1);
        if (op == Bamboo_add)
            s = "1" + itr;
        else
            s = "0" + itr;
        pi(e_tmp, s);
        ep_mul(e_tmp, e_tmp, this->K1);
        ep_mul(e_C, e_C, this->K);
        ep_add(e_C, e_C, e_tmp);

        ep_write_bin(buf1, 33, e_L, 1);
        L.assign((char *)buf1, 33);

        ep_write_bin(buf1, 33, e_D, 1);
        D.assign((char *)buf1, 33);

        ep_write_bin(buf1, 33, e_C, 1);
        C.assign((char *)buf1, 33);

        Ls.emplace_back(L);
        Ds.emplace_back(D);
        Cs.emplace_back(C);

        cell.tk = tk1;
        cell.cntw += 1;
    }

    this->state.Put(cell, keyword);

    ep_free(e_L);
    ep_free(e_D);
    ep_free(e_C);
    ep_free(e_tmp);
}
