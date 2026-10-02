#define _POSIX_C_SOURCE 200809L
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "sha1.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void check(const char *input, const char *expected)
{
    struct sha1_context c; char hex[41]; sha1_init(&c);
    for(size_t i=0;i<strlen(input);i++) sha1_update(&c,input+i,1);
    sha1_final(&c,hex); assert(!strcmp(hex,expected));
}
int main(void)
{
    check("","da39a3ee5e6b4b0d3255bfef95601890afd80709");
    check("abc","a9993e364706816aba3e25717850c26c9cd0d89d");
    check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    struct sha1_context c; char hex[41], chunk[1000]; memset(chunk,'a',sizeof(chunk));
    sha1_init(&c); for(unsigned i=0;i<1000;i++) sha1_update(&c,chunk,sizeof(chunk));
    sha1_final(&c,hex); assert(!strcmp(hex,"34aa973cd4c4daa4f61eeb2bdbad27316534016f"));
    char path[]="/tmp/battlehud-sha1-XXXXXX"; int fd=mkstemp(path); assert(fd>=0);
    assert(write(fd,"abc",3)==3); assert(close(fd)==0);
    assert(sha1_file(path,hex)); assert(!strcmp(hex,"a9993e364706816aba3e25717850c26c9cd0d89d"));
    assert(unlink(path)==0); assert(!sha1_file(path,hex)); assert(!sha1_file(NULL,hex));
    puts("M3 SHA-1 known vectors, streaming and file reads passed");
    return 0;
}
