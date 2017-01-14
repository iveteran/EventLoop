#include "tls_connection.h"

using namespace std;

namespace evt_loop
{
namespace {
    static SSL_CTX* g_sslCtx;

    void safeSSLInit() {
        static int forinit = [] {
            // Register the error strings for libcrypto & libssl
            SSL_load_error_strings ();
            // Register the available ciphers and digests
            int r = SSL_library_init ();
            if (!r) { printf("SSL_library_init failed\n"); return -1; }
            g_sslCtx = SSL_CTX_new (SSLv23_method ());
            if (g_sslCtx == NULL) { printf("SSL_CTX_new failed\n"); return -1; }
            printf("ssl library inited\n");
            return 0;
        }();
        (void)forinit;
    }
}

void TLSConnection::setSSLCertKey(const char* cert, const char* key, const char* ca_cert)
{
    safeSSLInit();
    printf("ssl cert: %s, key: %s, ca cert: %s\n", cert, key, ca_cert);

    int r = SSL_CTX_use_certificate_file(g_sslCtx, cert, SSL_FILETYPE_PEM);
    if (r<=0) { printf("SSL_CTX_use_certificate_file %s failed\n", cert); ERR_print_errors_fp(stderr); return; }
    r = SSL_CTX_use_PrivateKey_file(g_sslCtx, key, SSL_FILETYPE_PEM);
    if (r<=0) { printf("SSL_CTX_use_PrivateKey_file %s failed\n", key); ERR_print_errors_fp(stderr); return; }
    r = SSL_CTX_check_private_key(g_sslCtx);
    if (!r) { printf("SSL_CTX_check_private_key failed\n"); ERR_print_errors_fp(stderr); return; }

    if (ca_cert) {
      r = SSL_CTX_load_verify_locations(g_sslCtx, ca_cert, NULL);
      if (!r) { printf("SSL_CTX_load_verify_locations failed\n"); ERR_print_errors_fp(stderr); return; }

      STACK_OF(X509_NAME) *ca_list = SSL_load_client_CA_file(ca_cert);
      if (ca_list == NULL) { printf("SSL_load_client_CA_file failed\n"); ERR_print_errors_fp(stderr); return; }
      SSL_CTX_set_client_CA_list(g_sslCtx, ca_list);

      SSL_CTX_set_verify_depth(g_sslCtx, 1);
      SSL_CTX_set_verify(g_sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    }
}

TLSConnection::TLSConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr,
            const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
    TcpConnection(fd, local_addr, peer_addr, close_cb, tcp_evt_cbs), ssl_(NULL)
{
    safeSSLInit();
}

TLSConnection::~TLSConnection()
{
  printf("[TLSConnection::~TLSConnection]\n");
  if (ssl_) {
    SSL_shutdown (ssl_);
    SSL_free(ssl_);
  }
}

void TLSConnection::OnHandshake()
{
    //printf("[TLSConnection::OnHandshake]\n");
    if (ssl_ == NULL) {
        ssl_ = SSL_new(g_sslCtx);
        if (ssl_ == NULL) {
            printf("SSL_new failed errno %d errstr %s\n", errno, strerror(errno));
            Disconnect();
            return;
        }
        int r = SSL_set_fd(ssl_, fd_);
        if (r == 0) {
            printf("SSL_set_fd failed errno %d errstr %s\n", errno, strerror(errno));
            Disconnect();
            return;
        }
        if (IsClient()) {
            printf("SSL_set_connect_state for fd: %d\n", fd_);
            SSL_set_connect_state(ssl_);
        } else {
            printf("SSL_set_accept_state for fd: %d\n", fd_);
            SSL_set_accept_state(ssl_);
        }
    }
    int r = SSL_do_handshake(ssl_);
    if (r == 1) {
        printf("ssl handshake success fd: %d\n", fd_);
        state_ = State::READY;
        OnReady();
    } else {
        int err = SSL_get_error(ssl_, r);
        if (err == SSL_ERROR_WANT_WRITE) {
            AddWriteEvent();
        } else if (err == SSL_ERROR_WANT_READ) {
            AddReadEvent();
        } else {
            printf("SSL_do_handshake return %d error %d errno %d msg %s\n", r, err, errno, strerror(errno));
            ERR_print_errors_fp(stderr);
            Disconnect();
        }
    }
    return;
}

int TLSConnection::OnRead(const void* buf, size_t bytes)
{
    printf("[TLSConnection::OnRead]\n");
    int rd = SSL_read(ssl_, (void*)buf, bytes);
    int ssle = SSL_get_error(ssl_, rd);
    if (rd < 0 && ssle != SSL_ERROR_WANT_READ) {
        AddReadEvent();
        printf("SSL_read return %d error %d errno %d msg %s\n", rd, ssle, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    return rd;
}

int TLSConnection::OnWrite(const void* buf, size_t bytes)
{
    printf("[TLSConnection::OnWrite]\n");
    int wd = SSL_write(ssl_, buf, bytes);
    int ssle = SSL_get_error(ssl_, wd);
    if (wd < 0 && ssle != SSL_ERROR_WANT_WRITE) {
        AddWriteEvent();
        printf("SSL_write return %d error %d errno %d msg %s\n", wd, ssle, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    return wd;
}

};
