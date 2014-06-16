CPSC/ECE 3220 - Summer 2014 - Project 3 Assignment

[6/5/14 - added prototypes to the header file]


  Due date:         Monday, June 16, by midnight
  Grading standard: correctness of program - 100% of grade
  Submission:       use https://handin.cs.clemson.edu/; submit only the
                    memory_routines.c file (it should not be compressed
                    and should not contain header or driver code)
  Tools needed:     gcc
  Concepts needed:  memory allocation, boundary tag method

  This is to be an individual programming assignment.

You are to write two memory allocation routines to implement a variant
of Knuth's boundary-tag method. You should write the routines in C using
the default 64-bit pointers of gcc, and the allocations should be made
in 16-byte increments. You should use the first-fit policy to choose the
free block from which to allocate.

This is the header file defining the tag_block and free_block structures
used to implement this variant. Each tag block is 16 bytes, with the tag
and size stored separately. Eleven bytes are used as a character string
signature. (A more compact definition might place the tag and size in a
single word and use a second word as a magic number for validity
checking; see http://en.wikipedia.org/wiki/Magic_number_(programming).)


The following lines will be in "memory_routines.h":

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


The free list is a circular, doubly-linked list with tail and head
(back and forward) pointers allocated in a free_block structure just
below the last tag block. Released blocks of memory that cannot be
merged with existing free blocks should be added at the head of the
free list; there is no need to keep the free list in sorted order by
address since the boundary tags are used for merging ("coalescing")
contiguous blocks.

Here is the initial state of the memory area. Note that there are 80
bytes beyond the size of the area that can be allocated because of the
four tag blocks and free block header.

       =============  special ending tag block at start of region
       | tag=1     |    1 byte, this tag is always equal to one
       | signature |   11 bytes = "end_region"
       | empty     |    4 bytes = 0
       =============
       | tag       |    1 byte
       | signature |   11 bytes = "top_memblk"
       | size      |    4 bytes
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  ptr->| back_link |    8 bytes, used when part of free list       A
       | fwd_link  |    8 bytes, used when part of free list       |
       |           |                                               |
       |           |                                    size of free block
         ...                                             (multiple of 16)
       |           |                                               |
       |           |                                               V
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
       | tag       |    1 bytes
       | signature |   11 bytes = "end_memblk"
       | size      |    4 bytes
       =============  special starting tag block at end of region
       | tag=1     |    1 byte, this tag is always equal to one
       | signature |   11 bytes = "top_region"
       | empty     |    4 bytes = 0
       +-----------+  free list header node
  hdr->| back_link |    8 bytes, points to self if empty or to last node
       | fwd_link  |    8 bytes, points to self if empty or to first node
       =============

To give some example addresses and block sizes, assume that the data
structure starts at 0x100 and has 0x300 bytes to allocate.

       =============  special ending tag block at start of region
  0x100|       1   |    1 byte, this tag is always equal to one
  0x101|<signature>|   11 bytes = "end_region"
  0x10c|       0   |    4 bytes = 0, size
       =============
  0x110|       0   |    1 byte, free = 0
  0x111|<signature>|   11 bytes = "top_memblk"
  0x11c|   0x300   |    4 bytes, size
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x120|   0x440   |    8 bytes, used when part of free list       A
  0x128|   0x440   |    8 bytes, used when part of free list       |
       |           |                                               |
       |           |                                   size of free block
         ...                                         (multiple of 16 = 0x10)
       |           |                                    0x300 = 768 bytes
       |           |                                               V
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x420|       0   |    1 bytes, free = 0
  0x421|<signature>|   11 bytes = "end_memblk"
  0x42c|   0x300   |    4 bytes, size
       =============  special starting tag block at end of region
  0x430|       1   |    1 byte, this tag is always equal to one
  0x431|<signature>|   11 bytes = "top_region"
  0x43c|       0   |    4 bytes = 0
       +-----------+  free list header node
  0x440|   0x120   |    8 bytes, points to self if empty or to last node
  0x448|   0x120   |    8 bytes, points to self if empty or to first node
       =============

When a large enough free block is found, an allocation is made from the
higher-address end of the free block. Thus, if we allocate 0x60 bytes
from the 0x300-byte free block above, the data structures will now be:

       =============  special ending tag block at start of region
  0x100|       1   |    1 byte, this tag is always equal to one
  0x101|<signature>|   11 bytes = "end_region"
  0x10c|       0   |    4 bytes = 0, size
       =============
  0x110|       0   |    1 byte, free = 0
  0x111|<signature>|   11 bytes = "top_memblk"
  0x11c|   0x280   |    4 bytes, size
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x120|   0x440   |    8 bytes, used when part of free list       A
  0x128|   0x440   |    8 bytes, used when part of free list       |
       |           |                                               |
       ...                                              0x280 = 640 bytes
       |           |                                               V
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x3a0|       0   |    1 byte, free = 0
  0x3a1|<signature>|   11 bytes = "end_memblk"
  0x3ac|   0x280   |    4 bytes, size
       =============
  0x3b0|       1   |    1 byte, allocated = 1
  0x3b1|<signature>|   11 bytes = "top_memblk"
  0x3bc|    0x60   |    4 bytes, size
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x3c0|           |                                               A
         ...                                              0x60 = 96 bytes
       |           |                                               V
       +-----------+ - - - - - - - - - - - - - - - - - - - - - - - -
  0x420|       1   |    1 byte, allocated = 1
  0x421|<signature>|   11 bytes = "end_memblk"
  0x42c|    0x60   |    4 bytes, size
       =============  special starting tag block at end of region
  0x430|       1   |    1 byte, this tag is always equal to one
  0x431|<signature>|   11 bytes = "top_region"
  0x43c|       0   |    4 bytes = 0
       +-----------+  free list header node
  0x440|   0x120   |    8 bytes, points to self if empty or to last node
  0x448|   0x120   |    8 bytes, points to self if empty or to first node
       =============

In the normal case, each allocation uses 32 bytes beyond the requested
amount since additional tag blocks will be needed. For the example above,
0x300 - 0x60 - 0x20 = 0x280, or, in decimal,

   768   starting free space of 768 bytes (0x300 bytes)
  - 96   minus request of 96 bytes (0x60 bytes)
  - 32   minus extra tag block space of 32 bytes (0x20 bytes)
  ----
   640   equals resulting free space of 640 bytes (0x280 bytes)

When there is not at least 48 bytes left over in a free block after
an allocation, the whole free block is allocated. In this case, the
additional tags are not needed since the existing tags can be used.


You should only turn in a file with the two routines described below,
alloc_mem() and release_mem(). A header file and a driver program will
be compiled with your routines to test them.


  void *alloc_mem( unsigned amount )

    Input parameter
      "amount" is the number of bytes requested.

    Return value
      alloc_mem() returns a pointer to the start of the allocated
      block of free memory, beyond the allocation tag block. Note
      that the size of the beginning and ending tag blocks is over
      and above the number of bytes requested in "amount". If a
      suitable block cannot be found, alloc_mem() returns NULL.

    Description
      alloc_mem() rounds up the "amount" of memory requested to
      the nearest positive multiple of 16 bytes. It then searches
      in a first-fit manner for a block of free memory that can
      satisfy the requested amount of memory. There must be at
      least 48 bytes remaining in the free block after the
      allocation (i.e., enough leftover space for two tag blocks
      and a 16-byte remaining free area); otherwise, the whole
      free area must be allocated. If there is free memory
      leftover, it is left at the top of the free block. When a
      suitable block is found, alloc_mem() sets the tags, sizes,
      and signatures appropriately and returns a pointer to the
      beginning of the allocated memory (i.e., to the location
      immediately below the starting tag block).

      If a block cannot be found, alloc_mem() returns a NULL
      pointer.


  unsigned release_mem( void *ptr )

    Input parameter
      "ptr" is a pointer to the start of a block of memory that
      had been previously allocated by alloc_mem(); note that a
      tag block immediately precedes this address for a valid
      address.

    Return value
      release_mem() returns a code of 0 for valid pointers and
      a nonzero for invalid pointers. (Validity is determined
      by the presence of a tag block immediately preceding the
      "ptr" address with the tag set to allocated.)

    Description
      If the pointer yields a valid allocated block then the
      block is returned into the free list, possibly with
      merging taking place. The four possible valid cases are
      processed as described below, and each valid case results
      in a return value of zero.

      1) Both above and below blocks are allocated - add the
         returned block to the free list at the head of free
         list (thus the size of free list increases by one
         node); change the tags from allocated to free.

      2) Above block is free but below block is allocated -
         merge the returned block with the block above;
         change the tags and sizes appropriately, and change
         signatures in the ending tag block of the block
         below and the starting tag block of the returned
         block (thus the free list size and all the free list
         node pointers remain the same; you are only updating
         other fields in an existing free list node)

      3) Above block is allocated but below block is free -
         merge the returned block with the block below;
         change the tags and sizes appropriately, change
         signatures in the ending tag block of the returned
         block and the starting tag block of the block below,
         and change the free list pointers in the backward
         and forward free nodes to point to the top of the
         newly-merged free block (thus the size of the free
         list does not change)

      4) Both above and below block are free - merge the
         returned block with both the above and below blocks
         into a single free block and remove the node for the
         bottom block (thus reducing the size of the free list
         by one node); change the tags and sizes appropriately,
         and change signatures in all tag blocks except in the
         starting tag block of the block above and in the
         ending tag block of the block below.

      Invalid pointers result in a nonzero return code, with
      no other release actions performed.


You can change signatures in any unused tag blocks to hint strings,
such as "old_top_mb" and "old_end_mb". This will cause signature
checking to fail when you use a bad pointer to an old tag block; and,
since the signature check macro prints a failing character string,
these hints can sometimes help you in debugging.


Below is a test run of the routines. The lines starting with the three
periods are debugging messages in my version of alloc_mem() to verify
the input parameter and the order of searching the free list; you do
not need to include these print statements in your version of the
function. Other lines are printed by the test driver.

start memory allocation test, pointer size is 8 bytes
data structure starts at 0x1ef4010
header is located at 0x1ef4690
   ---------------free list---------------
   free block at 0x1ef4030 of size 0x640
   --------------end of list--------------
alloc 0x640
...alloc_mem() is passed amount of 0x640
...alloc_mem() examining free block at 0x1ef4030 of size 0x640
   ----------free list is empty-----------
release 0x640
   ---------------free list---------------
   free block at 0x1ef4030 of size 0x640
   --------------end of list--------------
alloc 6 blocks
...alloc_mem() is passed amount of 0x100
...alloc_mem() examining free block at 0x1ef4030 of size 0x640
...alloc_mem() is passed amount of 0x100
...alloc_mem() examining free block at 0x1ef4030 of size 0x520
...alloc_mem() is passed amount of 0x100
...alloc_mem() examining free block at 0x1ef4030 of size 0x400
...alloc_mem() is passed amount of 0x100
...alloc_mem() examining free block at 0x1ef4030 of size 0x2e0
...alloc_mem() is passed amount of 0x100
...alloc_mem() examining free block at 0x1ef4030 of size 0x1c0
...alloc_mem() is passed amount of 0xa0
...alloc_mem() examining free block at 0x1ef4030 of size 0xa0
   ----------free list is empty-----------
try to alloc 0xa0 more
*** alloc_mem() returns NULL
   ----------free list is empty-----------
release ptr[1] - tests case 1
   ---------------free list---------------
   free block at 0x1ef4570 of size 0x100
   --------------end of list--------------
release ptr[4] - tests case 1
   ---------------free list---------------
   free block at 0x1ef4210 of size 0x100
   free block at 0x1ef4570 of size 0x100
   --------------end of list--------------
release ptr[3] - tests case 2
   ---------------free list---------------
   free block at 0x1ef4210 of size 0x220
   free block at 0x1ef4570 of size 0x100
   --------------end of list--------------
release ptr[5] - tests case 3
   ---------------free list---------------
   free block at 0x1ef40f0 of size 0x340
   free block at 0x1ef4570 of size 0x100
   --------------end of list--------------
release ptr[2] - tests case 4
   ---------------free list---------------
   free block at 0x1ef40f0 of size 0x580
   --------------end of list--------------
release ptr[6] - tests case 3
   ---------------free list---------------
   free block at 0x1ef4030 of size 0x640
   --------------end of list--------------
re-release ptr[2] - logical error
*** release_mem() fails
alloc 12 blocks and release 5 to create 6 free blocks
...alloc_mem() is passed amount of 0x60
...alloc_mem() examining free block at 0x1ef4030 of size 0x640
...alloc_mem() is passed amount of 0x50
...alloc_mem() examining free block at 0x1ef4030 of size 0x5c0
...alloc_mem() is passed amount of 0x50
...alloc_mem() examining free block at 0x1ef4030 of size 0x550
...alloc_mem() is passed amount of 0x40
...alloc_mem() examining free block at 0x1ef4030 of size 0x4e0
...alloc_mem() is passed amount of 0x40
...alloc_mem() examining free block at 0x1ef4030 of size 0x480
...alloc_mem() is passed amount of 0x30
...alloc_mem() examining free block at 0x1ef4030 of size 0x420
...alloc_mem() is passed amount of 0x30
...alloc_mem() examining free block at 0x1ef4030 of size 0x3d0
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4030 of size 0x380
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4030 of size 0x340
...alloc_mem() is passed amount of 0x10
...alloc_mem() examining free block at 0x1ef4030 of size 0x300
...alloc_mem() is passed amount of 0x10
...alloc_mem() examining free block at 0x1ef4030 of size 0x2d0
...alloc_mem() changes amount from 0x293 to 0x2a0
...alloc_mem() examining free block at 0x1ef4030 of size 0x2a0
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef4390 of size 0x20
   free block at 0x1ef4420 of size 0x30
   free block at 0x1ef44d0 of size 0x40
   free block at 0x1ef45a0 of size 0x50
   --------------end of list--------------
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4320 of size 0x10
...alloc_mem() examining free block at 0x1ef4390 of size 0x20
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef4420 of size 0x30
   free block at 0x1ef44d0 of size 0x40
   free block at 0x1ef45a0 of size 0x50
   --------------end of list--------------
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4320 of size 0x10
...alloc_mem() examining free block at 0x1ef4420 of size 0x30
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef44d0 of size 0x40
   free block at 0x1ef45a0 of size 0x50
   --------------end of list--------------
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4320 of size 0x10
...alloc_mem() examining free block at 0x1ef44d0 of size 0x40
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef45a0 of size 0x50
   --------------end of list--------------
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4320 of size 0x10
...alloc_mem() examining free block at 0x1ef45a0 of size 0x50
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef45a0 of size 0x10
   --------------end of list--------------
...alloc_mem() is passed amount of 0x20
...alloc_mem() examining free block at 0x1ef4320 of size 0x10
...alloc_mem() examining free block at 0x1ef45a0 of size 0x10
*** alloc_mem() returns NULL
   ---------------free list---------------
   free block at 0x1ef4320 of size 0x10
   free block at 0x1ef45a0 of size 0x10
   --------------end of list--------------
