#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

int write_bs(char *src, bsd_t bs_id, int page) {

  /* write one page of data from src
     to the backing store bs_id, page
     page.
  */
    if (( bs_id < 0 || bs_id >= NBACKINGSTORE ) || ( page < 0 || page >= NPAGES)) {
        return SYSERR;
    }
   char * phy_addr = BACKING_STORE_BASE + bs_id * BACKING_STORE_UNIT_SIZE + page * NBPG;
   bcopy((void*)src, phy_addr, NBPG);
    return OK;
}

