#include "BambooServer.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <mutex>
#include "../primitive.h"
#include <iostream>

extern "C"
{
#include <relic/relic.h>
#include "unistd.h"
}

using std::string;
using std::vector;

static std::mutex lock_cells;

void BambooServer::Setup()
{
    srv_store.Clear();
}

void BambooServer::Save(const std::string &L, const std::string &D, const std::string &C)
{
    EDBCell cell = {.L = L,
                    .D = D,
                    .C = C};

    srv_store.Put(cell);
}

void BambooServer::Search(std::vector<std::string> &result, const std::string &K_in, const std::string &L_in,
                          const std::string &MskD_in, const std::string &MskC_in)
{
    string L, MskD, MskC, tk, cip;
    bn_t K, c, e, d, ord;
    ep_t ele1, ele2;
    unsigned char buf[128];
    EDBCell cell;

    core_init();
    ep_param_set(NIST_P256);

    bn_new(c);
    bn_new(e);
    bn_new(d);
    bn_new(ord);

    bn_new(K);
    ep_new(ele1);
    ep_new(ele2);

    ep_curve_get_ord(ord);

    bn_read_bin(K, (const unsigned char *)K_in.c_str(), 32);
    bn_gcd_ext(c, d, e, K, ord);

    L = L_in;
    MskD = MskD_in;
    MskC = MskC_in;

    cell.L = L_in;

    while (srv_store.Get(cell))
    {
        try
        {
            ep_read_bin(ele1, (const unsigned char *)cell.D.c_str(), 33);
            ep_read_bin(ele2, (const unsigned char *)MskD.c_str(), 33);
        }
        catch (const std::exception &e)
        {
            std::cerr << "ep_read_bin failed!" << std::endl;
        }

        ep_sub(ele1, ele1, ele2);
        ep_mul(ele1, ele1, d);
        pi_inv(tk, ele1);

        ep_read_bin(ele1, (const unsigned char *)cell.C.c_str(), 33);
        ep_read_bin(ele2, (const unsigned char *)MskC.c_str(), 33);
        ep_sub(ele1, ele1, ele2);
        ep_write_bin(buf, 33, ele1, 1);
        cip.assign((const char *)buf, 33);
        result.emplace_back(cip);

        cip = tk;

        Hash_H1(ele1, cip);
        ep_mul(ele1, ele1, K);
        ep_write_bin(buf, 33, ele1, 1);
        L.assign((const char *)buf, 33);

        Hash_H2(ele1, cip);
        ep_mul(ele1, ele1, K);
        ep_write_bin(buf, 33, ele1, 1);
        MskD.assign((const char *)buf, 33);

        Hash_G(ele1, cip);
        ep_mul(ele1, ele1, K);
        ep_write_bin(buf, 33, ele1, 1);
        MskC.assign((const char *)buf, 33);

        cell.L = L;
    }

    bn_free(c);
    bn_free(e);
    bn_free(d);
    bn_free(ord);
    bn_free(K);
    ep_free(ele1);
    ep_free(ele2);

    return;
}

void BambooServer::KeyUpdate(const string &token)
{
    vector<EDBCell> ciphers;
    bn_t d;
    ep_t ele1;
    unsigned char buf[128];

    bn_new(d);
    ep_new(ele1);

    bn_read_bin(d, (const unsigned char *)token.c_str(), 32);

    srv_store.PopAll(ciphers);

    for (EDBCell &cell : ciphers)
    {
        ep_read_bin(ele1, (const unsigned char *)cell.L.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf, 33, ele1, 1);
        cell.L.assign((const char *)buf, 33);

        ep_read_bin(ele1, (const unsigned char *)cell.D.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf, 33, ele1, 1);
        cell.D.assign((char *)buf, 33);

        ep_read_bin(ele1, (const unsigned char *)cell.C.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf, 33, ele1, 1);
        cell.C.assign((char *)buf, 33);
    }

    srv_store.PushBatch(ciphers);

    bn_free(d);
    ep_free(ele1);

    return;
}

void BambooServer::DumpData(const std::string &name)
{
    srv_store.DumpData(name);
}

void BambooServer::LoadData(const std::string &name)
{
    srv_store.LoadData(name);
}

void BambooServer::KeyUpdate_Parallel(const string &token, int num_threads)
{
    std::thread threads[64];
    vector<EDBCell> cells;
    string x = token;
    int _num_threads = 0;

    srv_store.PopAll(cells);

    _num_threads = num_threads > (int)cells.size() ? (int)cells.size() : num_threads;

    for (int i = 0; i < _num_threads; i++)
    {
        threads[i] = std::thread(do_KeyUpdate_in_parallel,
                                 std::ref(cells),
                                 i,
                                 std::ref(x),
                                 _num_threads);
    }

    usleep(1000);

    for (int i = 0; i < _num_threads; i++)
        threads[i].join();

    srv_store.PushBatch(cells);
}

void BambooServer::SaveBatch(const vector<std::string> &Ls, const vector<std::string> &Ds, const vector<std::string> &Cs)
{
    vector<EDBCell> cells;

    for (int i = 0; i < Ls.size(); i++)
    {
        EDBCell cell;
        cell.L = Ls[i];
        cell.D = Ds[i];
        cell.C = Cs[i];
        cells.emplace_back(cell);
    }
    this->srv_store.PushBatch(cells);
}

void do_KeyUpdate_in_parallel(vector<EDBCell> &cells, int number, string &delta, int num_threads)
{
    bn_t d;
    ep_t ele1;
    unsigned char buf_L[128], buf_D[128], buf_C[128];
    vector<string> label_old;

    core_init();
    ep_param_set(NIST_P256);

    bn_new(d);
    ep_new(ele1);

    bn_read_bin(d, (const unsigned char *)delta.c_str(), 32);

    int cur_num = number;
    auto itr = cells.begin() + number;

    while (1 == 1)
    {
        if (cur_num >= cells.size())
            break;

        EDBCell &cell = *itr;

        ep_read_bin(ele1, (const unsigned char *)cell.L.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf_L, 33, ele1, 1);

        ep_read_bin(ele1, (const unsigned char *)cell.D.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf_D, 33, ele1, 1);

        ep_read_bin(ele1, (const unsigned char *)cell.C.c_str(), 33);
        ep_mul(ele1, ele1, d);
        ep_write_bin(buf_C, 33, ele1, 1);

        lock_cells.lock();
        cell.L.assign((char *)buf_L, 33);
        cell.D.assign((char *)buf_D, 33);
        cell.C.assign((char *)buf_C, 33);
        lock_cells.unlock();

        cur_num += num_threads;
        itr = itr + num_threads;
    }

    bn_free(d);
    ep_free(ele1);
}
