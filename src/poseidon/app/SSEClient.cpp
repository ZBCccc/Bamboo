#include "SSEClient.h"

#include <string>
#include <iostream>
#include <chrono>

extern "C"
{
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
}

#include "../../core/primitive.h"

using std::endl;

extern double bench_clnt_time;
extern unsigned int bench_bandwidth;

SSEClient::SSEClient(const std::string &srv_addr, int srv_port)
{
    server_addr = srv_addr;
    server_port = srv_port;
}

int SSEClient::_ConnectToServer()
{
    struct sockaddr_in srv_addr;
    int sock;
    int buf_size = 1024 * 1024 * 10;
    int flag;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_size, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&buf_size, sizeof(int));
    flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&flag, sizeof(int));
    flag = 3;
#ifdef __APPLE__
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, (const char *)&flag, sizeof(int));
#else
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (const char *)&flag, sizeof(int));
#endif
    flag = 20;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (const char *)&flag, sizeof(int));
    flag = 3;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (const char *)&flag, sizeof(int));

    memset(&srv_addr, 0, sizeof(srv_addr));

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(server_port);
    srv_addr.sin_addr.s_addr = inet_addr(server_addr.c_str());

    int ret = connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret != 0)
        std::cerr << "Connect to the serve failed!" << std::endl;
    return sock;
}

void SSEClient::Setup()
{
    int sock = _ConnectToServer();
    int net_op = static_cast<int>(OP_SETUP);
    int stat;

    send(sock, &net_op, sizeof(int), 0);
    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);

    poseidon_client.Setup();
}

void SSEClient::prepare_dataset(const std::map<std::string, std::vector<std::string>> &data_to_encrypt, PoseidonOp op)
{
    std::vector<std::string> labels, ciphers, Ds;
    int net_op = static_cast<int>(OP_SAVE_BATCH);
    int len;
    int sock;
    int stat;

    for (const auto &itr : data_to_encrypt)
    {
        bamboo_client.BatchDataUpdate(labels, Ds, ciphers, itr.first, itr.second, op);
        std::cerr << "prepared keyword " << itr.first << endl;
        sock = _ConnectToServer();
        send(sock, &net_op, sizeof(int), 0);
        len = labels.size();
        send(sock, &len, sizeof(int), 0);
        for (int i = 0; i < labels.size(); i++)
        {
            send_bytes(sock, labels[i]);
            send_bytes(sock, Ds[i]);
            send_bytes(sock, ciphers[i]);
        }
        recv_data(sock, (unsigned char *)&stat, sizeof(int));
        close(sock);
        labels.clear();
        Ds.clear();
        ciphers.clear();
    }
}

void SSEClient::DataUpdate(const std::string &keyword, const std::string &id, BambooOp op)
{
    std::string L, D, C;
    int net_op = static_cast<int>(OP_SAVE_CIPHER);
    int sock, stat;

    bamboo_client.DataUpdate(L, D, C, op, keyword, id);

    sock = _ConnectToServer();

    send(sock, &net_op, sizeof(int), 0);
    send_bytes(sock, L);
    send_bytes(sock, D);
    send_bytes(sock, C);
    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
}

void SSEClient::Search(std::vector<std::string> &result, const std::string &keyword)
{
    std::string K, L, MskD, MskC, _tmp;
    int cnt_pad;
    std::vector<std::string> cipher;
    int cip_cnt, cnt_w, sock, stat;
    std::chrono::steady_clock::time_point begin, end;
    std::chrono::duration<double, std::micro> elapsed;
    int net_op = static_cast<int>(OP_SRCH_QRY);

    InitializeKey();

    begin = std::chrono::steady_clock::now();
    bamboo_client.Trapdoor(K, L, MskD, MskC, keyword, cnt_w);
    end = std::chrono::steady_clock::now();
    elapsed = end - begin;
    bench_clnt_time = elapsed.count();

    sock = _ConnectToServer();
    send(sock, &net_op, sizeof(int), 0);

    K = Encrypt_data(K);
    L = Encrypt_data(L);
    MskD = Encrypt_data(MskD);
    MskC = Encrypt_data(MskC);

    send_bytes(sock, K);
    send_bytes(sock, L);
    send_bytes(sock, MskD);
    send_bytes(sock, MskC);

    bench_bandwidth += K.size() + L.size() + MskD.size() + MskC.size();

    cip_cnt = 0;

    for (int i = 0; i < A_MAX; i++)
    {
        recv_bytes(sock, _tmp);
        bench_bandwidth += _tmp.size();
        if (cip_cnt < cnt_w)
        {
            cipher.emplace_back(Decrypt_data(_tmp));
            cip_cnt++;
        }
    }

    result.reserve(cnt_w);

    begin = std::chrono::steady_clock::now();
    bamboo_client.DecryptResult(result, cipher, keyword);
    end = std::chrono::steady_clock::now();
    elapsed = end - begin;
    bench_clnt_time += elapsed.count();

    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
    return;
}

void SSEClient::KeyUpdate(int thread_num)
{
    std::string utk;
    std::chrono::steady_clock::time_point begin, end;
    std::chrono::duration<double, std::micro> elapsed;
    int stat, sock;
    int net_op = static_cast<int>(OP_KEY_UPDT);

    InitializeKey();

    begin = std::chrono::steady_clock::now();
    bamboo_client.KeyUpdate(utk);
    end = std::chrono::steady_clock::now();
    elapsed = end - begin;
    bench_clnt_time = elapsed.count();

    sock = _ConnectToServer();
    send(sock, &net_op, sizeof(int), 0);
    send(sock, &thread_num, sizeof(int), 0);
    send_bytes(sock, Encrypt_data(utk));

    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
}

void SSEClient::BackupDB(const std::string &name)
{
    int sock, net_op;
    int stat;

    sock = _ConnectToServer();
    net_op = static_cast<int>(OP_BACKUP_EDB);
    send(sock, &net_op, sizeof(int), 0);
    send_bytes(sock, name);

    bamboo_client.DumpData(std::string("SEKU_clnt_data_") + name);
    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
}

void SSEClient::LoadEDB(const std::string &name)
{
    int sock, net_op;
    int stat;

    sock = _ConnectToServer();
    net_op = static_cast<int>(OP_LOAD_EDB);
    send(sock, &net_op, sizeof(int), 0);
    send_bytes(sock, name);

    bamboo_client.LoadData(std::string("SEKU_clnt_data_") + name);
    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
}

void SSEClient::InitializeKey()
{
    ep_t ele1, ele_gen;
    bn_t bn, ord;
    unsigned char buf[256];
    std::string s_c, s_gen, s_ret;
    int net_op = static_cast<int>(OP_ECDH);
    int sock, stat;

    core_init();
    ep_param_set(NIST_P256);

    ep_new(ep);
    ep_new(ele_gen);
    bn_new(bn);
    bn_new(ord);

    ep_curve_get_gen(ele_gen);
    ep_curve_get_ord(ord);
    bn_rand_mod(bn, ord);

    ep_mul(ele1, ele_gen, bn);
    ep_write_bin(buf, 256, ele1, 1);
    s_c.assign((const char *)buf, ep_size_bin(ele1, 1));

    ep_write_bin(buf, 256, ele_gen, 1);
    s_gen.assign((const char *)buf, ep_size_bin(ele_gen, 1));

    sock = _ConnectToServer();
    send(sock, &net_op, sizeof(int), 0);
    send_bytes(sock, s_gen);
    send_bytes(sock, s_c);

    recv_bytes(sock, s_ret);

    ep_read_bin(ele1, (const unsigned char *)s_ret.c_str(), s_ret.size());
    ep_mul(ele1, ele1, bn);
    ep_write_bin(buf, 256, ele1, 0);

    bench_bandwidth = s_c.size() + s_gen.size() + s_ret.size();

    SHA256(buf, ep_size_bin(ele1, 0), session_key);
    ep_free(ep);
    ep_free(ele_gen);
    bn_free(bn);
    bn_free(ord);

    recv_data(sock, (unsigned char *)&stat, sizeof(int));
    close(sock);
}

std::string SSEClient::Encrypt_data(const std::string &data)
{
    unsigned char buf[512];
    unsigned char plain[512];
    unsigned char IV[16];
    AES_KEY aes_key;
    int cipher_len = AES_BLOCK_SIZE * ((data.size() + sizeof(int)) / AES_BLOCK_SIZE + 1);
    std::string ret;

    RAND_bytes(IV, 16);
    memcpy(buf, IV, 16);

    *((int *)plain) = data.size();
    memcpy(plain + sizeof(int), data.c_str(), data.size());
    AES_set_encrypt_key(this->session_key, 128, &aes_key);
    AES_cbc_encrypt(plain, buf + 16, data.size() + sizeof(int), &aes_key, IV, AES_ENCRYPT);

    ret.assign((const char *)buf, 16 + cipher_len);
    return ret;
}

std::string SSEClient::Decrypt_data(const std::string &data)
{
    unsigned char buf[512];
    unsigned char IV[16];
    AES_KEY aes_key;
    std::string ret;

    memcpy(IV, data.c_str(), 16);
    memset(buf, 0, 512);

    AES_set_decrypt_key(this->session_key, 128, &aes_key);
    AES_cbc_encrypt((const unsigned char *)(data.c_str() + 16), buf, data.size() - 16, &aes_key, IV, AES_DECRYPT);

    ret.assign((const char *)(buf + sizeof(int)), *((int *)buf));
    return ret;
}
