/* keydb.h - Key database
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef G10_KEYDB_H
#define G10_KEYDB_H

#include "types.h"
#include "global.h"
#include "packet.h"
#include "cipher.h"


#define IS_KEY_SIG(s)    ((s)->sig_class == 0x1f)
#define IS_UID_SIG(s)    (((s)->sig_class & ~3) == 0x10)
#define IS_SUBKEY_SIG(s) ((s)->sig_class == 0x18)
#define IS_KEY_REV(s)    ((s)->sig_class == 0x20)
#define IS_UID_REV(s)    ((s)->sig_class == 0x30)
#define IS_SUBKEY_REV(s) ((s)->sig_class == 0x28)


struct getkey_ctx_s;
typedef struct getkey_ctx_s *GETKEY_CTX;

/****************
 * A Keyblock is all packets which form an entire certificate;
 * i.e. the public key, certificate, trust packets, user ids,
 * signatures, and subkey.
 *
 * This structure is also used to bind arbitrary packets together.
 */

struct kbnode_struct {
    KBNODE next;
    PACKET *pkt;
    int flag;
    int private_flag;
    ulong recno;  /* used while updating the trustdb */
};

#define is_deleted_kbnode(a)  ((a)->private_flag & 1)
#define is_cloned_kbnode(a)   ((a)->private_flag & 2)


enum resource_type {
    rt_UNKNOWN = 0,
    rt_RING = 1
};


/****************
 * A data structre to hold information about the external position
 * of a keyblock.
 */
struct keyblock_pos_struct {
    int   resno;     /* resource number */
    enum resource_type rt;
    off_t offset;    /* position information */
    unsigned count;  /* length of the keyblock in packets */
    IOBUF  fp;	     /* used by enum_keyblocks */
    int secret;      /* working on a secret keyring */
    PACKET *pkt;     /* ditto */
    int valid;
};
typedef struct keyblock_pos_struct KBPOS;

/* structure to hold a couple of public key certificates */
typedef struct pk_list *PK_LIST;
struct pk_list {
    PK_LIST next;
    PKT_public_key *pk;
    int mark;
};

/* structure to hold a couple of secret key certificates */
typedef struct sk_list *SK_LIST;
struct sk_list {
    SK_LIST next;
    PKT_secret_key *sk;
    int mark;
};

/* structure to collect all information which can be used to
 * identify a public key */
typedef struct pubkey_find_info *PUBKEY_FIND_INFO;
struct pubkey_find_info {
    u32  keyid[2];
    unsigned nbits;
    byte pubkey_algo;
    byte fingerprint[MAX_FINGERPRINT_LEN];
    char userid[1];
};


typedef struct keydb_handle *KEYDB_HANDLE;

typedef enum {
    KEYDB_SEARCH_MODE_NONE,
    KEYDB_SEARCH_MODE_EXACT,
    KEYDB_SEARCH_MODE_SUBSTR,
    KEYDB_SEARCH_MODE_MAIL,
    KEYDB_SEARCH_MODE_MAILSUB,
    KEYDB_SEARCH_MODE_MAILEND,
    KEYDB_SEARCH_MODE_WORDS,
    KEYDB_SEARCH_MODE_SHORT_KID,
    KEYDB_SEARCH_MODE_LONG_KID,
    KEYDB_SEARCH_MODE_FPR16,
    KEYDB_SEARCH_MODE_FPR20,
    KEYDB_SEARCH_MODE_FPR,
    KEYDB_SEARCH_MODE_FIRST,
    KEYDB_SEARCH_MODE_NEXT
} KeydbSearchMode;

struct keydb_search_desc {
    KeydbSearchMode mode;
    int (*skipfnc)(void *,u32*);
    void *skipfncvalue;
    union {
        const char *name;
        char fpr[MAX_FINGERPRINT_LEN];
        u32  kid[2];
    } u;
};

/*-- keydb.c --*/
int keydb_add_resource (const char *url, int force, int secret);
KEYDB_HANDLE keydb_new (int secret);
void keydb_release (KEYDB_HANDLE hd);
const char *keydb_get_resource_name (KEYDB_HANDLE hd);
int keydb_get_keyblock (KEYDB_HANDLE hd, KBNODE *ret_kb);
int keydb_update_keyblock (KEYDB_HANDLE hd, KBNODE kb);
int keydb_insert_keyblock (KEYDB_HANDLE hd, KBNODE kb);
int keydb_delete_keyblock (KEYDB_HANDLE hd);
int keydb_locate_writable (KEYDB_HANDLE hd, const char *reserved);
void keydb_rebuild_caches (void);
int keydb_search_reset (KEYDB_HANDLE hd);
int keydb_search (KEYDB_HANDLE hd, KEYDB_SEARCH_DESC *desc, size_t ndesc);
int keydb_search_first (KEYDB_HANDLE hd);
int keydb_search_next (KEYDB_HANDLE hd);
int keydb_search_kid (KEYDB_HANDLE hd, u32 *kid);
int keydb_search_fpr (KEYDB_HANDLE hd, const byte *fpr);


/*-- pkclist.c --*/
int  check_signatures_trust( PKT_signature *sig );
void release_pk_list( PK_LIST pk_list );
int  build_pk_list( STRLIST remusr, PK_LIST *ret_pk_list, unsigned use );
int  select_algo_from_prefs( PK_LIST pk_list, int preftype );
int  select_mdc_from_pklist (PK_LIST pk_list);

/*-- skclist.c --*/
void release_sk_list( SK_LIST sk_list );
int  build_sk_list( STRLIST locusr, SK_LIST *ret_sk_list,
					    int unlock, unsigned use );

/*-- passphrase.h --*/
int  have_static_passphrase(void);
void read_passphrase_from_fd( int fd );
void passphrase_clear_cache ( u32 *keyid, int algo );
DEK *passphrase_to_dek( u32 *keyid, int pubkey_algo,
			int cipher_algo, STRING2KEY *s2k, int mode);
void set_next_passphrase( const char *s );
char *get_last_passphrase(void);

/*-- getkey.c --*/
int classify_user_id( const char *name, KEYDB_SEARCH_DESC *desc);
void cache_public_key( PKT_public_key *pk );
void getkey_disable_caches(void);
int get_pubkey( PKT_public_key *pk, u32 *keyid );
KBNODE get_pubkeyblock( u32 *keyid );
int get_pubkey_byname( PKT_public_key *pk,  const char *name,
                       KBNODE *ret_keyblock, KEYDB_HANDLE *ret_kdbhd);
int get_pubkey_bynames( GETKEY_CTX *rx, PKT_public_key *pk,
			STRLIST names, KBNODE *ret_keyblock );
int get_pubkey_next( GETKEY_CTX ctx, PKT_public_key *pk, KBNODE *ret_keyblock );
void get_pubkey_end( GETKEY_CTX ctx );
int get_seckey( PKT_secret_key *sk, u32 *keyid );
int get_primary_seckey( PKT_secret_key *sk, u32 *keyid );
int get_pubkey_byfprint( PKT_public_key *pk, const byte *fprint,
						 size_t fprint_len );
int get_keyblock_byfprint( KBNODE *ret_keyblock, const byte *fprint,
						 size_t fprint_len );
int get_keyblock_bylid( KBNODE *ret_keyblock, ulong lid );
int seckey_available( u32 *keyid );
int get_seckey_byname( PKT_secret_key *sk, const char *name, int unlock );
int get_seckey_bynames( GETKEY_CTX *rx, PKT_secret_key *sk,
			STRLIST names, KBNODE *ret_keyblock );
int get_seckey_next( GETKEY_CTX ctx, PKT_secret_key *sk, KBNODE *ret_keyblock );
void get_seckey_end( GETKEY_CTX ctx );
int enum_secret_keys( void **context, PKT_secret_key *sk, int with_subkeys );
void merge_keys_and_selfsig( KBNODE keyblock );
char*get_user_id_string( u32 *keyid );
char*get_user_id_string_native( u32 *keyid );
char*get_long_user_id_string( u32 *keyid );
char*get_user_id( u32 *keyid, size_t *rn );

/*-- keyid.c --*/
int pubkey_letter( int algo );
int get_lsign_letter ( PKT_signature *sig );
u32 keyid_from_sk( PKT_secret_key *sk, u32 *keyid );
u32 keyid_from_pk( PKT_public_key *pk, u32 *keyid );
u32 keyid_from_sig( PKT_signature *sig, u32 *keyid );
u32 keyid_from_fingerprint( const byte *fprint, size_t fprint_len, u32 *keyid );
unsigned nbits_from_pk( PKT_public_key *pk );
unsigned nbits_from_sk( PKT_secret_key *sk );
const char *datestr_from_pk( PKT_public_key *pk );
const char *datestr_from_sk( PKT_secret_key *sk );
const char *datestr_from_sig( PKT_signature *sig );
const char *expirestr_from_pk( PKT_public_key *pk );
const char *expirestr_from_sk( PKT_secret_key *sk );
const char *expirestr_from_sig( PKT_signature *sig );

const char *colon_strtime (u32 t);
const char *colon_datestr_from_pk (PKT_public_key *pk);
const char *colon_datestr_from_sk (PKT_secret_key *sk);
const char *colon_datestr_from_sig (PKT_signature *sig);
const char *colon_expirestr_from_sig (PKT_signature *sig);

byte *fingerprint_from_sk( PKT_secret_key *sk, byte *buf, size_t *ret_len );
byte *fingerprint_from_pk( PKT_public_key *pk, byte *buf, size_t *ret_len );

/*-- kbnode.c --*/
KBNODE new_kbnode( PACKET *pkt );
KBNODE clone_kbnode( KBNODE node );
void release_kbnode( KBNODE n );
void delete_kbnode( KBNODE node );
void add_kbnode( KBNODE root, KBNODE node );
void insert_kbnode( KBNODE root, KBNODE node, int pkttype );
void move_kbnode( KBNODE *root, KBNODE node, KBNODE where );
void remove_kbnode( KBNODE *root, KBNODE node );
KBNODE find_prev_kbnode( KBNODE root, KBNODE node, int pkttype );
KBNODE find_next_kbnode( KBNODE node, int pkttype );
KBNODE find_kbnode( KBNODE node, int pkttype );
KBNODE walk_kbnode( KBNODE root, KBNODE *context, int all );
void clear_kbnode_flags( KBNODE n );
int  commit_kbnode( KBNODE *root );
void dump_kbnode( KBNODE node );

#endif /*G10_KEYDB_H*/
