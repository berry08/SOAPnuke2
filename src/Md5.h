#ifndef SEQARC_MD5_H
#define SEQARC_MD5_H
#include <stdint.h>
#include <cstdio>
#include <cstring>

/* POINTER defines a generic pointer type */  
typedef unsigned char * POINTER;    
    
/* MD5 context. */  
typedef struct {  
    uint32_t state[4];                                   /* state (ABCD) */  
    uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */  
    unsigned char buffer[64];                         /* input buffer */  
} MD5_CTX;  
    
void MD5Init (MD5_CTX *context);  
void MD5Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen);  
void MD5UpdaterString(MD5_CTX *context,const char *string);  
int MD5FileUpdateFile (MD5_CTX *context,char *filename);  
void MD5Final (unsigned char digest[16], MD5_CTX *context);  
void MDString (const char *string,unsigned char digest[16]);
void MDString (char *string, int len, unsigned char digest[16]);
int MD5File (char *filename,unsigned char digest[16]);  

#endif //SEQARC_MD5_H