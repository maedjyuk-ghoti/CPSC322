#include "memory_routines.h"
#include <stdio.h>
#include <string.h>

int main () {
  char* mem_block = malloc(sizeof(struct free_block)*20);

  header = ((struct free_block*)(mem_block)) + 19;

  // Bottom anchor tag
  ((struct tag_block*)(mem_block))->tag = 1;
  strcpy(((struct tag_block*)(mem_block))->sig, "end_region");
  ((struct tag_block*)(mem_block))->size = 0;

  // Top anchor tag
  (((struct tag_block*)(mem_block)) + 18)->tag = 1;
  strcpy((((struct tag_block*)(mem_block)) + 18)->sig, "top_region");
  (((struct tag_block*)(mem_block)) + 18)->size = 0;

  // Top free tag
  (((struct tag_block*)(mem_block)) + 1)->tag = 0;
  strcpy((((struct tag_block*)(mem_block)) + 1)->sig, "top_memblk");
  (((struct tag_block*)(mem_block)) + 1)->size = (15 * 16);

  // End free tag
  (((struct tag_block*)(mem_block)) + 17)->tag = 0;
  strcpy((((struct tag_block*)(mem_block)) + 17)->sig, "end_memblk");
  (((struct tag_block*)(mem_block)) + 17)->size = (15 * 16);

  // Set header pointers
  header->fwd_link = ((struct free_block*)(mem_block)) + 2;
  header->back_link = ((struct free_block*)(mem_block)) + 2;

  // Set free space pointers
  (((struct free_block*)(mem_block)) + 2)->fwd_link = header;
  (((struct free_block*)(mem_block)) + 2)->back_link = header;

  // check header and mem_block size
  // header should be located 0x130 higher than mem_block
  printf("\nRequested 20, %lu size chunks\n", sizeof(struct free_block));
  printf("mem_block is located at: %p\n", mem_block);
  printf("Header is located at   : %p\n", header);

  // check pointers
  printf("\nHeader->fwd_link points to: %p\n", header->fwd_link);
  printf("Header->back_link points to: %p\n", header->back_link);
  printf("free_block->fwd_link points to: %p\n", header->fwd_link->fwd_link);
  printf("free_block->back_link points to: %p\n", header->back_link->back_link);

  // check sizes
  printf("\nFree size: %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);
  unsigned ret;
/*  // allocate a 16 byte block
  printf("\nAsking for 16 bytes of memory\n");
  struct free_block* A = alloc_mem(16);
  printf("A is located at: %p\n", A);
  printf("Size of A is: %u\n", ((struct tag_block*)(A - 1))->size);
  printf("Remaining free space: %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);
  printf("Remaining free space: %u\n", ((struct tag_block*)(header->fwd_link->fwd_link - 1))->size);

  // allocate another 16 byte block
  printf("\nAsking for 16 bytes of memory\n");
  struct free_block* B = alloc_mem(16);
  printf("B is located at: %p\n", B);
  printf("Size of B is: %u\n", ((struct tag_block*)(B - 1))->size);
  printf("Remaining free space: %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);

  // releasing first 16 byte block
  printf("\nReleasing A (16 bytes)\n");
  ret = release_mem(A);
  printf("A is located at: %p\n", A);
  printf("Header->fwd_link points to: %p\n", header->fwd_link);
  printf("Header->back_link points to: %p\n", header->back_link);

  printf("Remaining free space (1): %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);
  printf("Remaining free space (2): %u\n", ((struct tag_block*)(header->fwd_link->fwd_link - 1))->size);

  // releasing other 16 byte block
  printf("\nReleasing B (16 bytes)\n");
  ret = release_mem(B);
  printf("B is located at: %p\n", B);
  printf("Header->fwd_link points to: %p\n", header->fwd_link);
  printf("Header->back_link points to: %p\n", header->back_link);
  printf("Remaining free space (1): %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);
  printf("Remaining free space (2): %u\n", ((struct tag_block*)(header->fwd_link->fwd_link - 1))->size);
*/
  // allocate all free space
  printf("\nAsking for 13*16 bytes of memory\n");
  struct free_block* E = alloc_mem(13*16);

  // releasing E
  printf("\nReleasing E (13*16 bytes)\n");
  ret = release_mem(E);
  printf("Remaining free space: %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);

  // allocate more than free space
  printf("\nAsking for 14*16 bytes of memory\n");
  struct free_block* F = alloc_mem(14*16);

  // releasing F
  printf("\nReleasing F (14*16 bytes)\n");
  ret = release_mem(E);
  printf("Remaining free space: %u\n", ((struct tag_block*)(header->fwd_link - 1))->size);

  return 0;
}
