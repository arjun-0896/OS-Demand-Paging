#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

    STATWORD ps;
    if (( bs_id < 0 || bs_id >= NBACKINGSTORE ) || ( npages < 0 || npages >= NPAGES)) {
        return SYSERR;
    }
    
    disable(ps);
    if ((bsm_tab[bs_id].bs_isprivheap == 1) && (bsm_tab[bs_id].bs_status == 1)) {
        restore(ps);
        return SYSERR;
    }
    
    /* npages is greater than the already allocated pages */
    if (bsm_tab[bs_id].bs_status == 1 && npages > bsm_tab[bs_id].bs_npages )
    {
        restore(ps);
        return SYSERR;
    }
    
    /* If unmapped, update the bsm_table to indicate allocation and return the requested pages */
    if (bsm_tab[bs_id].bs_status == 0) {
        int bsm_result = bsm_map(currpid, 4096, bs_id, npages);
        if(bsm_result == -1)
        {
            restore(ps);
            return SYSERR;
        }
        restore(ps);
        return npages;
    }
    /* npages is less than or equal to the already allocated pages */
    else
    {
        restore(ps);
        return npages;
    }
}


