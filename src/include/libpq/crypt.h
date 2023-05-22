/*-------------------------------------------------------------------------
 *
 * crypt.h
 *	  Interface to libpq/crypt.c
 *
 * Portions Copyright (c) 1996-2014, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/libpq/crypt.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CRYPT_H
#define PG_CRYPT_H

#include "c.h"

#include "libpq/libpq-be.h"
#include "libpq/md5.h"
#include "libpq/pg_sha2.h"

extern int password_hash_algorithm;
/*
 * Types of password hashes or verifiers that can be stored in
 * pg_authid.rolpassword.
 *
 * This is also used for the password_encryption GUC.
 */
typedef enum PasswordType
{
	PASSWORD_TYPE_PLAINTEXT = 0,
	PASSWORD_TYPE_MD5,
	PASSWORD_TYPE_SHA256,
	PASSWORD_TYPE_SCRAM_SHA_256
} PasswordType;

extern PasswordType get_password_type(const char *shadow_pass);
extern char *encrypt_password(PasswordType target_type, const char *role,
				 const char *password);

extern char *get_role_password(const char *role, char **logdetail);

extern int md5_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass, const char *md5_salt,
				 int md5_salt_len, char **logdetail);
extern int plain_crypt_verify(const char *role, const char *shadow_pass,
				   const char *client_pass, char **logdetail);

#endif
