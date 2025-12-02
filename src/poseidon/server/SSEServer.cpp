#include "SSEServer.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
extern "C" {
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
}
#include "../../core/primitive.h"
#include "../storage/impl/ServerStorageMemory.h"

using std::cerr;
using std::endl;
using std::string;

SSEServer::SSEServer(const std::string &addr, int port) {
  server_addr = addr;
  server_port = port;
  auto storage = std::make_unique<ServerStorageMemory>();
  poseidon_server.SetStorage(std::move(storage));
}

int SSEServer::_ServerSockInit() {
  struct sockaddr_in srv_addr;
  int sock, flag;
  int buf_size = 1024 * 1024 * 10;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_size, sizeof(int));
  setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&buf_size, sizeof(int));
  buf_size = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&buf_size,
             sizeof(int));
  flag = 1;
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&flag, sizeof(int));
  flag = 3;
#ifdef __APPLE__
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE, (const char *)&flag,
             sizeof(int));
#else
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (const char *)&flag, sizeof(int));
#endif
  flag = 20;
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (const char *)&flag, sizeof(int));
  flag = 3;
  setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (const char *)&flag,
             sizeof(int));

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

void SSEServer::Run() {
  int sock, clnt_sock, op_num;
  unsigned char buf[256];
  struct sockaddr_in clnt_addr;
  socklen_t socklen;
  int flag = 1;

  sock = this->_ServerSockInit();
  cerr << "Poseidon Server Running..." << endl;
  while (1) {
    NetworkOp net_op;
    clnt_sock = accept(sock, (struct sockaddr *)&clnt_addr, &socklen);
    setsockopt(clnt_sock, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(int));
    recv_data(clnt_sock, (unsigned char *)&op_num, sizeof(int));
    net_op = static_cast<NetworkOp>(op_num);
    switch (net_op) {
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

void SSEServer::_Setup(int sock) {
  int stat = 0;
  poseidon_server.Setup();
  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SaveCipher(int sock) {
  Metadata meta;
  int stat = 0;

  recv_bytes(sock, meta.addr);
  recv_bytes(sock, meta.val);
  recv_bytes(sock, meta.lastAddr);
  recv_bytes(sock, meta.alpha);
  recv_bytes(sock, meta.L);
  recv_bytes(sock, meta.D);
  recv_bytes(sock, meta.C);

  poseidon_server.Save(meta);
  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SrchQry(int sock) {
  std::chrono::steady_clock::time_point begin, end;
  std::chrono::duration<double, std::micro> elapsed;
  TrapdoorMetadata td;
  int stat = 0;
  int len;

  recv_bytes(sock, td.K1);
  td.K1 = Decrypt_data(td.K1);
  recv(sock, &len, sizeof(int), 0);
  td.TKL.resize(len);
  for (int i = 0; i < len; i++) {
    recv_bytes(sock, td.TKL[i].L);
    recv_bytes(sock, td.TKL[i].TD);
    recv_bytes(sock, td.TKL[i].TC);
    td.TKL[i].L = Decrypt_data(td.TKL[i].L);
    td.TKL[i].TD = Decrypt_data(td.TKL[i].TD);
    td.TKL[i].TC = Decrypt_data(td.TKL[i].TC);
  }
  recv(sock, &len, sizeof(int), 0);
  td.STKL.resize(len);
  for (int i = 0; i < len; i++) {
    recv_bytes(sock, td.STKL[i]);
    td.STKL[i] = Decrypt_data(td.STKL[i]);
  }
  recv(sock, &len, sizeof(int), 0);
  td.XTKL.resize(len);
  for (int i = 0; i < len; i++) {
    int len2;
    recv(sock, &len2, sizeof(int), 0);
    td.XTKL[i].resize(len2);
    for (int j = 0; j < len2; j++) {
      recv_bytes(sock, td.XTKL[i][j]);
      td.XTKL[i][j] = Decrypt_data(td.XTKL[i][j]);
    }
  }

  std::vector<ResMetadata> result;
  begin = std::chrono::steady_clock::now();
  poseidon_server.Search(result, td);
  end = std::chrono::steady_clock::now();

  elapsed = end - begin;
  std::cerr << "Search operation took " << elapsed.count() << " microseconds."
            << std::endl;

  len = result.size();
  std::cerr << "Found " << len << " results." << std::endl;

  for (int i = 0; i < len; i++) {
    send_bytes(sock, Encrypt_data(result[i].val));
    send_bytes(sock, std::to_string(result[i].cnt));
  }

  const std::string padValue = result.empty() ? std::string() : result[0].val;
  for (int i = len; i < A_MAX; i++) {
    send_bytes(sock, Encrypt_data(padValue));
    send_bytes(sock, std::string("0"));
  }

  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_BackupEDB(int sock) {
  std::string name;
  int stat = 0;
  recv_bytes(sock, name);
  poseidon_server.DumpData(string("poseidon_seku_srv_") + name);
  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_LoadEDB(int sock) {
  std::string name;
  int stat = 0;
  recv_bytes(sock, name);
  poseidon_server.LoadData(string("poseidon_seku_srv_") + name);
  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_KeyUpdt(int sock) {
  int stat = 0;
  int thread_num;
  std::string token;

  recv_data(sock, (unsigned char *)&thread_num, sizeof(int));
  recv_bytes(sock, token);
  if (thread_num == 1)
    poseidon_server.KeyUpdate(Decrypt_data(token));

  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_SaveBatch(int sock) {
  int len, stat = 0;
  std::vector<Metadata> metas;
  std::string tmp;

  recv_data(sock, (unsigned char *)&len, sizeof(int));
  metas.resize(len);
  for (int i = 0; i < len; i++) {
    recv_bytes(sock, metas[i].addr);
    recv_bytes(sock, metas[i].val);
    recv_bytes(sock, metas[i].lastAddr);
    recv_bytes(sock, metas[i].alpha);
    recv_bytes(sock, metas[i].L);
    recv_bytes(sock, metas[i].D);
    recv_bytes(sock, metas[i].C);
  }
  poseidon_server.SaveBatch(metas);
  send(sock, &stat, sizeof(int), 0);
}

void SSEServer::_ecdh(int sock) {
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

std::string SSEServer::Encrypt_data(const std::string &data) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16];
  unsigned char plain[512];
  unsigned char ciphertext[512];
  int len;
  int ciphertext_len;
  std::string ret;

  // Generate IV
  RAND_bytes(iv, 16);

  // Prepare plaintext with size prepended
  *((int *)plain) = data.size();
  memcpy(plain + sizeof(int), data.c_str(), data.size());
  int plaintext_len = data.size() + sizeof(int);

  // Initialize encryption
  EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, this->session_key, iv);

  // Encrypt
  EVP_EncryptUpdate(ctx, ciphertext, &len, plain, plaintext_len);
  ciphertext_len = len;

  // Finalize
  EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
  ciphertext_len += len;

  // Prepare result: IV + ciphertext
  ret.reserve(16 + ciphertext_len);
  ret.append((const char *)iv, 16);
  ret.append((const char *)ciphertext, ciphertext_len);

  EVP_CIPHER_CTX_free(ctx);
  return ret;
}

std::string SSEServer::Decrypt_data(const std::string &data) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  unsigned char iv[16];
  unsigned char plaintext[512];
  int len;
  int plaintext_len;
  std::string ret;

  // Extract IV
  memcpy(iv, data.c_str(), 16);

  // Initialize decryption
  EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, this->session_key, iv);

  // Decrypt
  EVP_DecryptUpdate(ctx, plaintext, &len,
                    (const unsigned char *)data.c_str() + 16, data.size() - 16);
  plaintext_len = len;

  // Finalize
  EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
  plaintext_len += len;

  // Extract size and return plaintext
  int data_size = *((int *)plaintext);
  ret.assign((const char *)(plaintext + sizeof(int)), data_size);

  EVP_CIPHER_CTX_free(ctx);
  return ret;
}