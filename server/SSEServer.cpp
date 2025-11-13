#include <iostream>
#include <chrono>
#include <memory>
#include "SSEServer.h"
#include <string>
extern "C"
{
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
}
#include "../primitive.h"

using std::cerr;
using std::endl;
using std::string;

SSEServer::SSEServer(const std::string &addr, int port)
{
    server_addr = addr;
    server_port = port;
    auto storage = std::make_unique<ServerStorageMemory>();
    bamboo_server.SetStorage(std::move(storage));
}

int SSEServer::_ServerSockInit()
{
    struct sockaddr_in srv_addr;
    int sock, flag;
    int buf_size = 1024 * 1024 * 10;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_size, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&buf_size, sizeof(int));
    buf_size = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&buf_size, sizeof(int));
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

    int ret = bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret != 0)
        std::cout << "Init socket failed!" << std::endl;
    listen(sock, 3);
    return sock;
}

void SSEServer::Run()
{
    int sock, clnt_sock, op_num;
    unsigned char buf[256];
    struct sockaddr_in clnt_addr;
    socklen_t socklen;
    int flag = 1;

    sock = this->_ServerSockInit();
    cerr << "Bamboo Server Running..." << endl;
    while (1)
    {
        NetworkOp net_op;

        clnt_sock = accept(sock, (struct sockaddr *)&clnt_addr, &socklen);
        setsockopt(clnt_sock, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int));
        recv_data(clnt_sock, (unsigned char *)&op_num, sizeof(int));
        net_op = static_cast<NetworkOp>(op_num);
        switch (net_op)
        {
        case OP_SETUP:
            this->_Setup(clnt_sock);
            break;
        case OP_SAVE_CIPHER:
            this->_SaveCipher(clnt_sock);
            break;
        case OP_SRCH_QRY:
            this->_SrchQry(clnt_sock);
            break;
        case OP_KEY_UPDT:
            this->_KeyUpdt(clnt_sock);
            break;
        case OP_ECDH:
            this->_ecdh(clnt_sock);
            break;
        case OP_BACKUP_EDB:
            this->_BackupEDB(clnt_sock);
            break;
        case OP_LOAD_EDB:
            this->_LoadEDB(clnt_sock);
            break;
        case OP_SAVE_BATCH:
            this->_SaveBatch(clnt_sock);
            break;
        default:
            cerr << "Unknown operation num: " << op_num << endl;
            break;
        }
        close(clnt_sock);
    }
}

void SSEServer::_Setup(int sock)
{
    int stat = 0;
    bamboo_server.Setup();
    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SaveCipher(int sock)
{
    std::string l, d, c;
    int stat = 0;

    recv_bytes(sock, l);
    recv_bytes(sock, d);
    recv_bytes(sock, c);

    bamboo_server.Save(l, d, c);
    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SrchQry(int sock)
{
    std::chrono::steady_clock::time_point begin, end;
    std::chrono::duration<double, std::micro> elapsed;

    std::vector<std::string> result;
    std::string k, l, mskd, mskc, tmp;
    int stat = 0;
    int len;

    recv_bytes(sock, k);
    recv_bytes(sock, l);
    recv_bytes(sock, mskd);
    recv_bytes(sock, mskc);

    begin = std::chrono::steady_clock::now();
    bamboo_server.Search(result, Decrypt_data(k), Decrypt_data(l), Decrypt_data(mskd), Decrypt_data(mskc));
    end = std::chrono::steady_clock::now();

    elapsed = end - begin;
    std::cerr << "Search operation took " << elapsed.count() << " microseconds." << std::endl;

    len = result.size();
    std::cerr << "Found " << len << " results." << std::endl;

    for (int i = 0; i < len; i++)
    {
        send_bytes(sock, Encrypt_data(result[i]));
    }

    const std::string padValue = result.empty() ? std::string() : result[0];
    for (int i = len; i < A_MAX; i++)
    {
        send_bytes(sock, Encrypt_data(padValue));
    }

    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_BackupEDB(int sock)
{
    std::string name;
    int stat = 0;
    recv_bytes(sock, name);
    bamboo_server.DumpData(string("seku_srv_") + name);
    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_LoadEDB(int sock)
{
    std::string name;
    int stat = 0;
    recv_bytes(sock, name);
    bamboo_server.LoadData(string("seku_srv_") + name);
    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_KeyUpdt(int sock)
{
    int stat = 0;
    int thread_num;
    std::string token;

    recv_data(sock, (unsigned char *)&thread_num, sizeof(int));
    recv_bytes(sock, token);
    if (thread_num == 1)
        bamboo_server.KeyUpdate(Decrypt_data(token));
    else
        bamboo_server.KeyUpdate_Parallel(Decrypt_data(token), thread_num);

    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SaveBatch(int sock)
{
    int len, stat = 0;
    std::vector<std::string> Ls, Ds, Cs;
    std::string tmp;

    recv_data(sock, (unsigned char *)&len, sizeof(int));
    for (int i = 0; i < len; i++)
    {
        recv_bytes(sock, tmp);
        Ls.emplace_back(tmp);
        recv_bytes(sock, tmp);
        Ds.emplace_back(tmp);
        recv_bytes(sock, tmp);
        Cs.emplace_back(tmp);
    }
    bamboo_server.SaveBatch(Ls, Ds, Cs);
    send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_ecdh(int sock)
{
    ep_t ele1, ele_gen;
    bn_t bn, ord;
    unsigned char buf[256];
    std::string gen, c, s_ret;
    int stat = 0;

    core_init();
    ep_param_set(NIST_P256);

    ep_new(ep);
    ep_new(ele_gen);
    bn_new(bn);
    bn_new(ord);

    ep_curve_get_ord(ord);
    bn_rand_mod(bn, ord);

    recv_bytes(sock, gen);
    recv_bytes(sock, c);

    ep_read_bin(ele_gen, (const unsigned char *)gen.c_str(), gen.size());
    ep_mul(ele_gen, ele_gen, bn);
    ep_write_bin(buf, 256, ele_gen, 1);
    s_ret.assign((const char *)buf, ep_size_bin(ele_gen, 1));

    ep_read_bin(ele1, (const unsigned char *)c.c_str(), c.size());
    ep_mul(ele1, ele1, bn);
    ep_write_bin(buf, 256, ele1, 0);

    SHA256(buf, ep_size_bin(ele1, 0), session_key); // 得到session_key

    send_bytes(sock, s_ret);

    ep_free(ep);
    ep_free(ele_gen);
    bn_free(bn);
    bn_free(ord);

    send(sock, &stat, sizeof(int), 0);
}

std::string SSEServer::Encrypt_data(const std::string &data)
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

std::string SSEServer::Decrypt_data(const std::string &data)
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