#include "memory_routines.h"
#include <stdio.h>
#include <string.h>

// Don't want to include another library, but still want readability
enum BOOL {FALSE, TRUE};

void* alloc_mem( unsigned amount ) {

  // Round amount up to nearest multiple of 16
  unsigned alloc_amnt;
  if ( amount % 16 == 0 ) {
    alloc_amnt = amount;
  }
  else {
    alloc_amnt = ( amount + (16 - (amount % 16)) );
  }

  // Find first fit for alloc_amount in free_list
  // 1) List is empty, returns on 1st trip through
  // 2) There is enough room, done = TRUE, exits loop
  // 3) There isn't enough room, iter will circle around to header and exit
  struct free_block* iter = header;
  enum BOOL done = FALSE;
  do {
    if ( iter->fwd_link == header ) {
      return NULL;
    }
    iter = iter->fwd_link;
    // free space needed >= alloc_amount + 2 tags (16 each) + 16 (extra space)
    if ( ((struct tag_block*)(iter - 1))->size >= (alloc_amnt + 48) ) {
      // leave header alone
      // move onto changing tags
      done = TRUE;
    }
    // else increase alloc amount to eat all free space
    else if ( ((struct tag_block*)(iter - 1))->size == (alloc_amnt + 32) ) {
      // increase alloc_amnt to cover free space
      alloc_amnt = ((struct tag_block*)(iter - 1))->size;
      // redirect pointers
      iter->back_link->fwd_link = iter->fwd_link;
      iter->fwd_link->back_link = iter->back_link;
      // move onto changing tags
      done = TRUE;
    }
  } while ( done != TRUE );

  // Set up new tag blocks and info
  // Ease of use pointers
  struct tag_block* free_top  = ((struct tag_block*)(iter - 1));
  struct tag_block* alloc_end = free_top + 1 + (free_top->size / 16);
  struct tag_block* alloc_top = alloc_end - (1 + (alloc_amnt / 16));
  struct tag_block* free_end  = alloc_top - 1;

  // Top of free block
  // 32 = 2 tags
  free_top->size = free_top->size - (alloc_amnt + 32);

  // End of free block
  if ( free_top != alloc_top ) {
    free_end->tag = 0;
    strcpy( alloc_end->sig, "end_memblk" );
    free_end->size = free_top->size;
  }

  // Top of new block
  alloc_top->tag = 1;
  strcpy( alloc_top->sig, "top_memblk" );
  alloc_top->size = alloc_amnt;

  // End of new block
  alloc_end->tag = 1;
  strcpy( alloc_end->sig, "end_memblk" );
  alloc_end->size = alloc_amnt;

  return (alloc_top + 1);
}

unsigned release_mem( void* ptr ) {
  struct tag_block* alloc_top_tag = ((struct tag_block*)(ptr - 16));

  // If parameter is not allocated
  if ( alloc_top_tag->tag != 1 ) {
    return 1;
  }

  // If above tag is allocated
  if ( (alloc_top_tag - 1)->tag == 1 ) {
    // if below is allocated
    if ( (alloc_top_tag + 2 + (alloc_top_tag->size / 16))->tag == 1 ) {
      // add to head of list
      ((struct free_block*)(ptr))->back_link = header;
      ((struct free_block*)(ptr))->fwd_link = header->fwd_link;
      header->fwd_link->back_link = ptr;
      header->fwd_link = ptr;

      // change top and bot tags to 0
      alloc_top_tag->tag = 0;
      (alloc_top_tag + 1 + (alloc_top_tag->size / 16))->tag = 0;

    }
    // below tag is free
    else {
      // merge returned block with block below
      // change the tags and sizes appropriately

      struct tag_block* alloc_end_tag = alloc_top_tag + 1 +
                                        (alloc_top_tag->size / 16);
      struct tag_block* free_end_tag  = alloc_end_tag + 2 +
                                        ((alloc_end_tag + 1)->size / 16);
      alloc_top_tag->tag  = 0;
      alloc_end_tag->tag  = 0;
      alloc_top_tag->size = alloc_top_tag->size + 32 +
                            (alloc_end_tag + 1)->size;
      free_end_tag->size  = alloc_top_tag->size;

      // change signatures in the ending tag block of the returned block
      //  and the starting tag block of the block below
      strcpy( alloc_end_tag->sig, "free" );
      strcpy( (alloc_end_tag + 1)->sig, "free" );

      // change the free list pointers in the backward and forward free nodes
      //  to point to the top of the newly-merged free block
      //  (aka the returned block)
      ((struct free_block*)(ptr))->fwd_link =
        ((struct free_block*)(alloc_end_tag + 2))->fwd_link;
      ((struct free_block*)(ptr))->back_link =
        ((struct free_block*)(alloc_end_tag + 2))->back_link;
      ((struct free_block*)(ptr))->fwd_link->back_link = ptr;
      ((struct free_block*)(ptr))->back_link->fwd_link = ptr;
    }
  }
  // Above tag is free
  else {
    // Below tag is allocated
    if ( (alloc_top_tag + 2 + (alloc_top_tag->size / 16))->tag == 1 ) {
      // merge the returned block with the block above
      // change the tags and sizes appropriately
      alloc_top_tag->tag = 0;
      (alloc_top_tag + 1 + (alloc_top_tag->size/16))->tag = 0;
      (alloc_top_tag + 1 + (alloc_top_tag->size/16))->size =
                           alloc_top_tag->size + (alloc_top_tag - 1)->size + 32;
      (alloc_top_tag - (2 + ((alloc_top_tag - 1)->size / 16)))->size =
                                 alloc_top_tag->size +(alloc_top_tag - 1)->size;

      // change signatures in the ending tag block of the block above and the
      //  starting tag block of the returned block
      strcpy( alloc_top_tag->sig, "free" );
      strcpy( (alloc_top_tag - 1)->sig, "free" );
    }

    // Below tag is free
    else {
      // merge the returned block with both the above and below blocks into a
      //  single free block and remove the node for the bottom block
      struct tag_block* below_top_tag = (alloc_top_tag + 2 +
                                        (alloc_top_tag->size / 16));
      struct tag_block* above_top_tag = (alloc_top_tag -
                                        ((2 + (alloc_top_tag - 1)->size / 16)));

      // change the tags and sizes appropriately
      alloc_top_tag->tag = 0;
      (below_top_tag - 1)->tag = 0;
      above_top_tag->size = above_top_tag->size + alloc_top_tag->size + 64 +
                            below_top_tag->size;
      (below_top_tag + (below_top_tag->size / 16) + 1)->size =
                                                            above_top_tag->size;

      // change the signatures in all tag blocks except in the starting tag
      //  block of the block above and in the ending tag block of the block
      //  below.
      strcpy( alloc_top_tag->sig, "free" );
      strcpy( below_top_tag->sig, "free" );
      strcpy( (alloc_top_tag - 1)->sig, "free" );
      strcpy( (below_top_tag - 1)->sig, "free" );

      // Change the free list pointers in the below->forward free node and the
      //  above free node.

      // keep pointer to lower address (above block)
      // disconnect higher address (below block) from list
      ((struct free_block*)(below_top_tag + 1))->back_link->fwd_link =
                            ((struct free_block*)(below_top_tag + 1))->fwd_link;
      ((struct free_block*)(below_top_tag + 1))->fwd_link->back_link =
                           ((struct free_block*)(below_top_tag + 1))->back_link;
    }
  }

  return 0;
}
