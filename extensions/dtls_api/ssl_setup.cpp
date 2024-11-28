#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "eventloop/logger.h"

#define COOKIE_SECRET_LENGTH 16

namespace evt_loop {

#if 0
// Cookie callbacks for DTLS
static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {
    *cookie_len = 16;  // Use a fixed length for simplicity
    memset(cookie, 0x01, *cookie_len);  // Fill with dummy data
    return 1;
}

static int verify_cookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len) {
    return 1;  // Accept all cookies for simplicity
}
#else

unsigned char cookie_secret[COOKIE_SECRET_LENGTH];
int cookie_initialized = 0;
unsigned char more_secret[16];

static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len)
	{
	unsigned char *buffer, result[EVP_MAX_MD_SIZE];
	unsigned int length = 0, resultlength;
	union {
		struct sockaddr_storage ss;
		struct sockaddr_in6 s6;
		struct sockaddr_in s4;
	} peer;

	/* Initialize a random secret */
	if (!cookie_initialized)
		{
		if (!RAND_bytes(cookie_secret, COOKIE_SECRET_LENGTH))
			{
			printf("error setting random cookie secret\n");
			return 0;
			}
		cookie_initialized = 1;
		}

	/* Read peer information */
	(void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

	/* Create buffer with peer's address and port */
	length = 0;
	switch (peer.ss.ss_family) {
		case AF_INET:
            length += sizeof(in_port_t);
			length += sizeof(struct in_addr);
			break;
		case AF_INET6:
            length += sizeof(in_port_t);
			length += sizeof(struct in6_addr);
			break;
		default:
		    length += sizeof(more_secret);
			break;
	}
	buffer = (unsigned char*) OPENSSL_malloc(length);

	if (buffer == NULL)
    {
		printf("out of memory\n");
		return 0;
    }

	switch (peer.ss.ss_family) {
		case AF_INET:
			memcpy(buffer,
				   &peer.s4.sin_port,
				   sizeof(in_port_t));
			memcpy(buffer + sizeof(peer.s4.sin_port),
				   &peer.s4.sin_addr,
				   sizeof(struct in_addr));
			break;
		case AF_INET6:
			memcpy(buffer,
				   &peer.s6.sin6_port,
				   sizeof(in_port_t));
			memcpy(buffer + sizeof(in_port_t),
				   &peer.s6.sin6_addr,
				   sizeof(struct in6_addr));
			break;
		default:
			memcpy(buffer, more_secret, sizeof(more_secret));
			break;
	}

	/* Calculate HMAC of buffer using the secret */
	HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH,
		 (const unsigned char*) buffer, length, result, &resultlength);
	OPENSSL_free(buffer);

	memcpy(cookie, result, resultlength);
	*cookie_len = resultlength;

	return 1;
}

static int verify_cookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len)
{
	unsigned char *buffer, result[EVP_MAX_MD_SIZE];
	unsigned int length = 0, resultlength;
	union {
		struct sockaddr_storage ss;
		struct sockaddr_in6 s6;
		struct sockaddr_in s4;
	} peer;

	/* If secret isn't initialized yet, the cookie can't be valid */
	if (!cookie_initialized)
		return 0;

	/* Read peer information */
	(void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

	/* Create buffer with peer's address and port */
	length = 0;
	switch (peer.ss.ss_family) {
		case AF_INET:
            length += sizeof(in_port_t);
			length += sizeof(struct in_addr);
			break;
		case AF_INET6:
            length += sizeof(in_port_t);
			length += sizeof(struct in6_addr);
			break;
		default:
		    length += sizeof(more_secret);
			break;
	}
	buffer = (unsigned char*) OPENSSL_malloc(length);

	if (buffer == NULL)
		{
		printf("out of memory\n");
		return 0;
		}

	switch (peer.ss.ss_family) {
		case AF_INET:
			memcpy(buffer,
				   &peer.s4.sin_port,
				   sizeof(in_port_t));
			memcpy(buffer + sizeof(in_port_t),
				   &peer.s4.sin_addr,
				   sizeof(struct in_addr));
			break;
		case AF_INET6:
			memcpy(buffer,
				   &peer.s6.sin6_port,
				   sizeof(in_port_t));
			memcpy(buffer + sizeof(in_port_t),
				   &peer.s6.sin6_addr,
				   sizeof(struct in6_addr));
			break;
		default:
			memcpy(buffer, more_secret, sizeof(more_secret));
			break;
	}

	/* Calculate HMAC of buffer using the secret */
	HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH,
		 (const unsigned char*) buffer, length, result, &resultlength);
	OPENSSL_free(buffer);

	if (cookie_len == resultlength && memcmp(result, cookie, resultlength) == 0)
		return 1;

	return 0;
}
#endif

SSL_CTX* SetupSSLContext(bool is_server, const char* cert_file, const char* key_file, const char* ca_file)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    const SSL_METHOD* method = is_server ? DTLS_server_method() : DTLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Set DTLS 1.2
    SSL_CTX_set_min_proto_version(ctx, DTLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(ctx, DTLS1_2_VERSION);

    if (is_server) {
        // Set DTLS cookie generation and verification
        SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie);
        SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie);
    }

    el_logger->debug("ssl cert: {}, key: {}, ca: {}", cert_file, key_file, ca_file);
    if (cert_file && strlen(cert_file) > 0) {
        if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }

    if (key_file && strlen(key_file) > 0) {
        if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }

    if (ca_file && strlen(ca_file) > 0) {
        if (!SSL_CTX_load_verify_locations(ctx, ca_file, nullptr)) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }

    if (! is_server) {
        // is client: MUST verify server side certificate for client
        SSL_CTX_set_verify_depth(ctx, 3);
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    }

    return ctx;
}

}  // evt_loop
