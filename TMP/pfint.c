/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
    STATWORD ps;
    disable(ps);
    int get_store, get_page;
    /* faulted address read from cr2 */
    unsigned long faulted_address = read_cr2();
    
    /* Let vp be the virtual page number of the page containing the faulted address. */
    int vp = (faulted_address / NBPG) & 0x000fffff;
    
    /*  Let pd be the corresponding page directory etry */
    pd_t *pd = proctab[currpid].pdbr + sizeof(pd_t) * (faulted_address >> 22);
    
    /* a is an illegal address if not mapped already */
    int bsm_result = bsm_lookup(currpid, faulted_address, &get_store, &get_page);
    if(bsm_result == -1 || get_store == -1 || get_page == -1)
    {
        kill(currpid); /*address not mapped -> invalid address */
        restore(ps);
        return SYSERR ;
    }
    
    /* Let p1 be the upper ten bits of a (page_dir_offset) */
    int p1 = faulted_address >> 22;
    
    /* Let q1 be the [21:12] bits of a (page_tab_offset) */
    int q1 = (faulted_address >> 12) & 0x000003ff;
    
    /* If the p-th page table does not exist, obtain a frame for it and initialize it */
    if (pd->pd_pres == 0) {
        int nframe = create_page_table(currpid);
        if (nframe == -1) {
            return SYSERR; /* Page table not created */
        }
        /* if created update the page directory entries */
        pd->pd_pres = 1;
        pd->pd_write = 1;
        pd->pd_base = nframe + FRAME0;
        frm_tab[(unsigned int) pd / NBPG - FRAME0].fr_refcnt++;
    }
    /* Step 8 */
    int f;
    get_frm(&f);
    if (f == -1) {
        kill(currpid);
        restore(ps);
        return SYSERR;
    }
    frm_tab[f].fr_dirty = 0;
    frm_tab[f].fr_counter = 0;
    frm_tab[f].fr_pid = currpid;
    frm_tab[f].fr_refcnt = 1;
    frm_tab[f].fr_status = 1;
    frm_tab[f].fr_type = FR_PAGE;
    frm_tab[f].fr_vpno = (faulted_address / NBPG) & 0x000fffff;
    
    read_bs((f + FRAME0) * NBPG, get_store, get_page);
    pt_t *pt = (pd->pd_base) * NBPG + sizeof(pt_t) * ((faulted_address >> 12) & 0x000003ff);
    pt->pt_pres = 1;
    pt->pt_write = 1;
    pt->pt_base = f + FRAME0;
    frm_tab[(unsigned int) pt / NBPG - FRAME0].fr_refcnt++;
   
    /* Whenever a page is allocated insert into fifo queue */
    insert_frame_into_fifoqueue(f);
   
    //to be implemented to update counter

    if (page_replace_policy == AGING)
    {
        int rr = AGING_update_counter();
    }
    update_cr3(currpid); // to update the page direcotory as the page tables are altered
    
  restore(ps);
  return OK;
}


