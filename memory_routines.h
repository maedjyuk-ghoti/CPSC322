#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* global data structures */

struct tag_block { char tag; char sig[11]; unsigned size; };
struct free_block { struct free_block *back_link, *fwd_link; };
struct free_block *header;

/* signature check macros */

#define SIGCHK(w,x,y,z) {struct tag_block *scptr = (struct tag_block *)(w);\
if(strncmp((char *)(scptr)+1,(x),(y))!=0){printf("*** sigchk fail\n");\
printf("*** at %s, ptr is %p, sig is %s\n",(z),(w),(char *)(w)+1);}}

#define TOPSIGCHK(a,b) {SIGCHK((a),"top_",4,(b))}
#define ENDSIGCHK(a,b) {SIGCHK((a),"end_",4,(b))}

void *alloc_mem( unsigned );
unsigned release_mem( void * );
