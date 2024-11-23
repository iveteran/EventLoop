#ifndef _SSL_SETUP_H
#define _SSL_SETUP_H

namespace evt_loop {

SSL_CTX* SetupSSLContext(bool is_server, const char* cert_file, const char* key_file, const char* ca_file);

} // evt_loop

#endif // _SSL_SETUP_H
