/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
    STATWORD ps;
    if ((virtpage < NBPG) || (source < 0) || (source >= NBACKINGSTORE) || (npages < 1) || (npages >= NPAGES))
    {
        return SYSERR;
    }
    /* If not allocated or private heap, the pages cannot be mapped */
    disable(ps);
    bs_map_t *bptr = &bsm_tab[source];
    if ((bptr->bs_status == 0) || (bptr->bs_isprivheap == 1)) {
        restore(ps);
        return OK;
    }
    /* cannot map pages greater than the allocated pages */
    if (npages > bsm_tab[source].bs_npages) {
        restore(ps);
        return SYSERR;
    }
    
    int bsm_result = bsm_map(currpid, virtpage, source, npages);
    if(bsm_result == SYSERR)
    {
        restore(ps);
        return SYSERR;
    }
    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    if ((virtpage < NBPG))
    {
        return SYSERR;
    }
    
    disable(ps);
    if(bsm_unmap(currpid, virtpage, 0) == -1)
    {
        restore(ps);
        return SYSERR;
    }
    update_cr3(currpid);
    restore(ps);
    return OK;
}
