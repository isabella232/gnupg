/* tdbio.h - Trust database I/O functions
 *	Copyright (C) 1998 Free Software Foundation, Inc.
 *
 * This file is part of GNUPG.
 *
 * GNUPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef G10_TDBIO_H
#define G10_TDBIO_H


#define TRUST_RECORD_LEN 40
#define SIGS_PER_RECORD 	((TRUST_RECORD_LEN-10)/5)
#define ITEMS_PER_HTBL_RECORD	((TRUST_RECORD_LEN-2)/4)
#define ITEMS_PER_HLST_RECORD	((TRUST_RECORD_LEN-6)/5)
#define MAX_LIST_SIGS_DEPTH  20


#define RECTYPE_VER  1
#define RECTYPE_DIR  2
#define RECTYPE_KEY  3
#define RECTYPE_UID  4
#define RECTYPE_PREF 5
#define RECTYPE_SIG  6
#define RECTYPE_CACH 9
#define RECTYPE_HTBL 10
#define RECTYPE_HLST 11


struct trust_record {
    int  rectype;
    struct trust_record *next;
    union {
	struct {	    /* version record: */
	    byte version;   /* should be 1 */
	    ulong locked;    /* pid of process which holds a lock */
	    ulong created;   /* timestamp of trustdb creation  */
	    ulong modified;  /* timestamp of last modification */
	    ulong validated; /* timestamp of last validation   */
	    byte marginals_needed;
	    byte completes_needed;
	    byte max_cert_depth;
	} ver;
	struct {	    /* directory record */
	    ulong lid;
	    ulong keylist;  /* List of keys (the first is the primary key)*/
	    ulong uidlist;  /* list of uid records */
	    ulong cacherec; /* the cache record */
	    byte ownertrust;
	    byte sigflag;
	} dir;
	struct {	    /* primary public key record */
	    ulong lid;
	    ulong next;    /* next key */
	    byte pubkey_algo;
	    byte fingerprint_len;
	    byte fingerprint[20];
	} key;
	struct {	    /* user id reord */
	    ulong lid;	    /* point back to the directory record */
	    ulong next;    /* points to next user id record */
	    ulong prefrec;   /* recno of reference record */
	    ulong siglist;   /* list of valid signatures (w/o self-sig)*/
	    byte namehash[20]; /* ripemd hash of the username */
	} uid;
	struct {	    /* preference reord */
	    ulong lid;	    /* point back to the directory record */
			    /* or 0 for a glocal pref record */
	    ulong next;    /* points to next pref record */
	} pref;
	struct {	    /* signature record */
	    ulong lid;
	    ulong next;   /* recnno of next record or NULL for last one */
	    struct {
		ulong lid;	 /* of pubkey record of signator (0=unused) */
		byte flag;	 /* reserved */
	    } sig[SIGS_PER_RECORD];
	} sig;
	struct {	    /* cache record */
	    ulong lid;
	    byte blockhash[20];
	    byte trustlevel;   /* calculated trustlevel */
	} cache;
	struct {
	    ulong item[ITEMS_PER_HTBL_RECORD];
	} htbl;
	struct {
	    ulong next;
	    struct {
		byte hash;
		ulong rnum;
	    } item[ITEMS_PER_HLST_RECORD];
	} hlst;
    } r;
};
typedef struct trust_record TRUSTREC;

typedef struct {
    ulong     local_id;    /* localid of the pubkey */
    ulong     sigrec;
    ulong     sig_id;	   /* returned signature id */
    unsigned  sig_flag;    /* returned signature record flag */
    struct {		   /* internal data */
	int init_done;
	int eof;
	TRUSTREC rec;
	int index;
    } ctl;
} SIGREC_CONTEXT;


/*-- tdbio.c --*/
int tdbio_set_dbname( const char *new_dbname, int create );
const char *tdbio_get_dbname(void);
void tdbio_dump_record( ulong rnum, TRUSTREC *rec, FILE *fp );
int tdbio_read_record( ulong recnum, TRUSTREC *rec, int expected );
int tdbio_write_record( ulong recnum, TRUSTREC *rec );
ulong tdbio_new_recnum(void);
int tdbio_search_dir_record( PKT_public_key *pk, TRUSTREC *rec );
int tdbio_update_sigflag( ulong lid, int sigflag );


#define buftoulong( p )  ((*(byte*)(p) << 24) | (*((byte*)(p)+1)<< 16) | \
		       (*((byte*)(p)+2) << 8) | (*((byte*)(p)+3)))
#define buftoushort( p )  ((*((byte*)(p)) << 8) | (*((byte*)(p)+1)))
#define ulongtobuf( p, a ) do { 			  \
			    ((byte*)p)[0] = a >> 24;	\
			    ((byte*)p)[1] = a >> 16;	\
			    ((byte*)p)[2] = a >>  8;	\
			    ((byte*)p)[3] = a	   ;	\
			} while(0)
#define ushorttobuf( p, a ) do {			   \
			    ((byte*)p)[0] = a >>  8;	\
			    ((byte*)p)[1] = a	   ;	\
			} while(0)
#define buftou32( p)	buftoulong( (p) )
#define u32tobuf( p, a) ulongtobuf( (p), (a) )



#endif /*G10_TDBIO_H*/
