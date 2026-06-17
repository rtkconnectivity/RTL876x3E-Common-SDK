/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Realtek-5-Clause
 */

#ifndef MBEDTLS_CONFIG_TLS_H
#define MBEDTLS_CONFIG_TLS_H


#include "string.h"
#include <stdio.h>
#include <stddef.h>
#include "trace.h"

void *mbedtls_platform_calloc(size_t n, size_t size);
void mbedtls_platform_free(void *ptr);

int mbedtls_platform_printf(const char *format, ...);
void mbedtls_platform_debug(void *ctx, int level, const char *file, int line, const char *str);

#define MBEDTLS_CONFIG_FILE "mbedtls_config_wifi.h"

#undef __GNUC__

/*Do not use built-in platform entropy functions.*/
#define MBEDTLS_NO_PLATFORM_ENTROPY
/* Enable the platform-specific entropy code. */
#define MBEDTLS_ENTROPY_C
/* Random number generation method(CRT/HMAC) */
//#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_CTR_DRBG_C

/* Encryption mode (CBC\CFB\CTR\OFB\XTS) */
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CFB
#define MBEDTLS_CIPHER_MODE_CTR
//#define MBEDTLS_CIPHER_MODE_OFB
//#define MBEDTLS_CIPHER_MODE_XTS

/* Padding mode */
#define MBEDTLS_CIPHER_PADDING_PKCS7
//#define MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS
//#define MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN
//#define MBEDTLS_CIPHER_PADDING_ZEROS

/* ---- Digital signatures ---- */
//#define MBEDTLS_ECDSA_DETERMINISTIC
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_PKCS1_V15

/* ---- Key exchange ---- */
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
//#define MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED
#define MBEDTLS_DHM_C
/* This module is used by the following key exchanges: CDHE-ECDSA, ECDHE-RSA, DHE-PSK */
#define MBEDTLS_ECDH_C
/* This module is used by the following key exchanges: ECDHE-ECDSA */
#define MBEDTLS_ECDSA_C
/* Enable the generic ASN1 parser. */
#define MBEDTLS_ASN1_PARSE_C
/* Enable the generic ASN1 writer. */
#define MBEDTLS_ASN1_WRITE_C


/* To enable hardware entropy source, it is necessary to implement mbedtls_hardware_poll() */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* Enable the CMAC (Cipher-based Message Authentication Code) mode for block. */
#define MBEDTLS_CMAC_C

/* Enable the generic cipher layer */
#define MBEDTLS_CIPHER_C

/* Enable the generic layer for message digest (hashing) and HMAC */
#define MBEDTLS_MD_C

/* Enable the elliptic curve over GF(p) library. */
#define MBEDTLS_ECP_C
/* Enables specific curves within the Elliptic Curve */
#define MBEDTLS_ECP_DP_SECP192R1_ENABLED
#define MBEDTLS_ECP_DP_SECP224R1_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
/* Enable NIST curves optimisation */
#define MBEDTLS_ECP_NIST_OPTIM

/* Enable the multi-precision integer library */
#define MBEDTLS_BIGNUM_C

/* Enable the Base64 module (required by X.509) */
#define MBEDTLS_BASE64_C


/* ---- SSL configuration ---- */
/* Enable sending of all alert messages */
//#define MBEDTLS_SSL_ALL_ALERT_MESSAGES
/* Enable support for RFC 7627 */
#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET
//#define MBEDTLS_SSL_FALLBACK_SCSV
/* Enable storing the peer's certificate */
#define MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
/* Enable support for TLS renegotiation */
#define MBEDTLS_SSL_RENEGOTIATION
/* Enable support for RFC 5077 session tickets in SSL */
#define MBEDTLS_SSL_SESSION_TICKETS
/* Enable support for RFC 6066 max_fragment_length extension in SSL */
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
/* Enable support for RFC 7301 Application Layer Protocol Negotiation */
#define MBEDTLS_SSL_ALPN
/* Enable support for TLS 1.x */
#define MBEDTLS_SSL_PROTO_TLS1_2
//#define MBEDTLS_SSL_PROTO_TLS1_3
/* Enable support for RFC 6066 server name indication (SNI) in SSL */
//#define MBEDTLS_SSL_SERVER_NAME_INDICATION
/* The default maximum amount of 0-RTT data */
#define MBEDTLS_SSL_MAX_EARLY_DATA_SIZE        1024
/*  Enable simple SSL cache implementation. */
#define MBEDTLS_SSL_CACHE_C
/* Enable the SSL/TLS client code. */
#define MBEDTLS_SSL_CLI_C
/* Enable the generic SSL/TLS code. */
#define MBEDTLS_SSL_TLS_C

//#define MBEDTLS_SSL_EXPORT_KEYS

/* Enable the AES block cipher */
#define MBEDTLS_AES_C
/* Enable the Counter with CBC-MAC (CCM) mode for 128-bit block cipher. */
#define MBEDTLS_CCM_C
/* Enable the generic cipher layer. */
#define MBEDTLS_CIPHER_C







/* Enable the Galois/Counter Mode (GCM). */
#define MBEDTLS_GCM_C
/* Enable PEM decoding / parsing. */
#define MBEDTLS_PEM_PARSE_C
/* Enable generic public key parse functions. */
#define MBEDTLS_PK_PARSE_C
/* Enable generic public key wrappers. */
#define MBEDTLS_PK_C
/* Enable the RSA public-key cryptosystem. */
#define MBEDTLS_RSA_C
//#define MBEDTLS_PK_HAVE_ECC_KEYS

/* Enable PKCS#5 functions. */
#define MBEDTLS_PKCS5_C

/* Enable the SHA1 cryptographic hash algorithm. */
#define MBEDTLS_SHA1_C
/* Enable the SHA-224 cryptographic hash algorithm. */
#define MBEDTLS_SHA224_C
/* Enable the SHA-256 cryptographic hash algorithm. */
#define MBEDTLS_SHA256_C
/* Enable the SHA-384 cryptographic hash algorithm. */
#define MBEDTLS_SHA384_C
/* Enable the SHA-512 cryptographic hash algorithm. */
#define MBEDTLS_SHA512_C

/* This modules translates between OIDs and internal values. */
#define MBEDTLS_OID_C

/* This module is required for the X.509 parsing modules. */
#define MBEDTLS_X509_USE_C
/* Enable X.509 certificate parsing. */
#define MBEDTLS_X509_CRT_PARSE_C
/* Enable X.509 CRL parsing. */
#define MBEDTLS_X509_CRL_PARSE_C
/* Enable X.509 Certificate Signing Request (CSR) parsing. */
#define MBEDTLS_X509_CSR_PARSE_C
/* This module is the basis for creating X.509 certificates and CSRs. */
//#define MBEDTLS_X509_CREATE_C
/* Enable creating X.509 certificates. */
//#define MBEDTLS_X509_CRT_WRITE_C
/* Enable creating X.509 Certificate Signing Requests (CSR). */
//#define MBEDTLS_X509_CSR_WRITE_C
/* Enable support for RSASSA-PSS signature scheme. */
#define MBEDTLS_X509_RSASSA_PSS_SUPPORT


/* Enable error code to error string conversion. */
#define MBEDTLS_ERROR_C
/* Enable the debug functions. */
#define MBEDTLS_DEBUG_C
/* This module enables abstraction of common (libc) functions. */
#define MBEDTLS_PLATFORM_C
/* Enable this layer to allow use of alternative memory allocators. */
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_PRINTF_MACRO       mbedtls_platform_printf
#define MBEDTLS_PLATFORM_FREE_MACRO         mbedtls_platform_free
#define MBEDTLS_PLATFORM_CALLOC_MACRO       mbedtls_platform_calloc

#endif /* MBEDTLS_CONFIG_TLS_H */
