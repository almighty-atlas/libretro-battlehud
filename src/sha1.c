#include "sha1.h"
#include <stdio.h>
#include <string.h>
static uint32_t rol(uint32_t x, unsigned n) { return (x << n) | (x >> (32 - n)); }
static void transform(struct sha1_context *c)
{
    uint32_t w[80], a=c->h[0], b=c->h[1], d=c->h[3], e=c->h[4], v=c->h[2];
    for (unsigned i=0;i<16;i++) {
        const unsigned char *p=c->block+4*i;
        w[i]=(uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
    }
    for (unsigned i=16;i<80;i++) w[i]=rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    for (unsigned i=0;i<80;i++) {
        uint32_t f,k;
        if (i<20) { f=(b&v)|(~b&d); k=0x5a827999; }
        else if (i<40) { f=b^v^d; k=0x6ed9eba1; }
        else if (i<60) { f=(b&v)|(b&d)|(v&d); k=0x8f1bbcdc; }
        else { f=b^v^d; k=0xca62c1d6; }
        uint32_t t=rol(a,5)+f+e+k+w[i];
        e=d; d=v; v=rol(b,30); b=a; a=t;
    }
    c->h[0]+=a; c->h[1]+=b; c->h[2]+=v; c->h[3]+=d; c->h[4]+=e;
}
void sha1_init(struct sha1_context *c)
{
    memset(c,0,sizeof(*c));
    c->h[0]=0x67452301; c->h[1]=0xefcdab89; c->h[2]=0x98badcfe;
    c->h[3]=0x10325476; c->h[4]=0xc3d2e1f0;
}
void sha1_update(struct sha1_context *c, const void *data, size_t size)
{
    const unsigned char *p=data;
    c->bytes+=size;
    while (size) {
        size_t n=64-c->used; if(n>size) n=size;
        memcpy(c->block+c->used,p,n); c->used+=n; p+=n; size-=n;
        if(c->used==64) { transform(c); c->used=0; }
    }
}
void sha1_final(struct sha1_context *c, char hex[41])
{
    uint64_t bits=c->bytes*8;
    unsigned char pad[128]={0x80};
    size_t n=c->used<56 ? 56-c->used : 120-c->used;
    for(unsigned i=0;i<8;i++) pad[n+i]=(unsigned char)(bits >> (56-8*i));
    sha1_update(c,pad,n+8);
    static const char digits[]="0123456789abcdef";
    for(unsigned i=0;i<20;i++) {
        unsigned char byte=(unsigned char)(c->h[i/4]>>(24-8*(i%4)));
        hex[2*i]=digits[byte>>4]; hex[2*i+1]=digits[byte&15];
    }
    hex[40]='\0';
}
bool sha1_file(const char *path, char hex[41])
{
    if(!path) return false;
    FILE *f=fopen(path,"rb"); if(!f) return false;
    struct sha1_context c; sha1_init(&c);
    unsigned char bytes[8192]; size_t n, total=0;
    while((n=fread(bytes,1,sizeof(bytes),f))) {
        if(total>32*1024*1024-n) { fclose(f); return false; }
        total+=n; sha1_update(&c,bytes,n);
    }
    bool ok=!ferror(f) && total>0;
    fclose(f);
    if(ok) sha1_final(&c,hex);
    return ok;
}
