#include "tls_connection.h"
#include "eventloop/logger.h"

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
            if (!r) { el_logger->error("SSL_library_init failed"); return -1; }
            g_sslCtx = SSL_CTX_new (SSLv23_method ());
            if (g_sslCtx == NULL) { el_logger->error("SSL_CTX_new failed"); return -1; }
            el_logger->debug("ssl library inited");
            return 0;
        }();
        (void)forinit;
    }
}

void TLSConnection::setSSLCertKey(const char* cert, const char* key, const char* ca_cert)
{
    safeSSLInit();
    el_logger->debug("ssl cert: {}, key: {}, ca cert: {}", cert, key, ca_cert);

    int r = SSL_CTX_use_certificate_file(g_sslCtx, cert, SSL_FILETYPE_PEM);
    if (r<=0) { el_logger->error("SSL_CTX_use_certificate_file {} failed", cert); ERR_print_errors_fp(stderr); return; }
    r = SSL_CTX_use_PrivateKey_file(g_sslCtx, key, SSL_FILETYPE_PEM);
    if (r<=0) { el_logger->error("SSL_CTX_use_PrivateKey_file {} failed", key); ERR_print_errors_fp(stderr); return; }
    r = SSL_CTX_check_private_key(g_sslCtx);
    if (!r) { el_logger->error("SSL_CTX_check_private_key failed"); ERR_print_errors_fp(stderr); return; }

    if (ca_cert && strlen(ca_cert) > 0) {
      r = SSL_CTX_load_verify_locations(g_sslCtx, ca_cert, NULL);
      if (!r) { el_logger->error("SSL_CTX_load_verify_locations failed"); ERR_print_errors_fp(stderr); return; }

      STACK_OF(X509_NAME) *ca_list = SSL_load_client_CA_file(ca_cert);
      if (ca_list == NULL) { el_logger->error("SSL_load_client_CA_file failed"); ERR_print_errors_fp(stderr); return; }
      SSL_CTX_set_client_CA_list(g_sslCtx, ca_list);

      SSL_CTX_set_verify_depth(g_sslCtx, 1);
      SSL_CTX_set_verify(g_sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    }
}

TLSConnection::TLSConnection(int fd, const IPAddress& local_addr, const IPAddress& peer_addr, const IPAddress& peer_real_addr,
            const OnClosedCallback& close_cb, TcpCallbacksPtr tcp_evt_cbs) :
    TcpConnection(fd, local_addr, peer_addr, peer_real_addr, close_cb, tcp_evt_cbs), ssl_(NULL)
{
    safeSSLInit();
}

TLSConnection::~TLSConnection()
{
  el_logger->debug("[TLSConnection::~TLSConnection]");
  if (ssl_) {
    SSL_shutdown (ssl_);
    SSL_free(ssl_);
  }
}

bool TLSConnection::OnHandshake()
{
    el_logger->debug("[TLSConnection::OnHandshake begin] fd: {}", fd_);
    int r = 0;
    if (ssl_ == NULL) {
        ssl_ = SSL_new(g_sslCtx);
        if (ssl_ == NULL) {
            el_logger->error("SSL_new failed errno {} errstr {}", errno, strerror(errno));
            state_ = State::FAILED;
            goto out;
        }
        int r = SSL_set_fd(ssl_, fd_);
        if (r == 0) {
            el_logger->error("SSL_set_fd failed errno {} errstr {}", errno, strerror(errno));
            state_ = State::FAILED;
            goto out;
        }
        if (IsClient()) {
            el_logger->debug("SSL_set_connect_state for fd: {}", fd_);
            SSL_set_connect_state(ssl_);
        } else {
            el_logger->debug("SSL_set_accept_state for fd: {}", fd_);
            SSL_set_accept_state(ssl_);
        }
    }
    r = SSL_do_handshake(ssl_);
    if (r == 1) {
        el_logger->debug("[TLSConnection::OnHandshake] ssl handshake success fd: {}", fd_);
        state_ = State::READY;
        OnReady();
    } else {
        int err = SSL_get_error(ssl_, r);
        if (err == SSL_ERROR_WANT_WRITE) {
            AddWriteEvent();
        } else if (err == SSL_ERROR_WANT_READ) {
            //AddReadEvent();
            DeleteWriteEvent();
        } else {
            el_logger->error("SSL_do_handshake return {} error {} errno {} msg {}", r, err, errno, strerror(errno));
            ERR_print_errors_fp(stderr);
            state_ = State::FAILED;
            goto out;
        }
    }
out:
    bool success = (state_ != State::FAILED);
    el_logger->debug("[TLSConnection::OnHandshake end] success: {}", success);
    return success;
}

int TLSConnection::OnRead(const void* buf, size_t bytes)
{
    el_logger->debug("[TLSConnection::OnRead]");
    int rd = SSL_read(ssl_, (void*)buf, bytes);
    int ssle = SSL_get_error(ssl_, rd);
    if (rd < 0 && ssle != SSL_ERROR_WANT_READ) {
        //AddReadEvent();
        el_logger->error("SSL_read return {} error {} errno {} msg {}", rd, ssle, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    return rd;
}

int TLSConnection::OnWrite(const void* buf, size_t bytes)
{
    el_logger->debug("[TLSConnection::OnWrite]");
    int wd = SSL_write(ssl_, buf, bytes);
    int ssle = SSL_get_error(ssl_, wd);
    if (wd < 0 && ssle != SSL_ERROR_WANT_WRITE) {
        //AddWriteEvent();
        el_logger->error("SSL_write return {} error {} errno {} msg {}", wd, ssle, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
    }
    return wd;
}

};
