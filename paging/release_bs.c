#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
   if (bs_id < 0 || bs_id >= NBACKINGSTORE)
   {
        return SYSERR;
   }
   free_bsm(bs_id);
   update_cr3(currpid); /* pdbr updated in control register 3 */
   return OK;
}

