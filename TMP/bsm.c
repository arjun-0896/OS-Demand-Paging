/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
bs_map_t bsm_tab[NBACKINGSTORE];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    int i;
    for (i = 0; i < NBACKINGSTORE; i++) {
        bsm_tab[i].bs_refcnt = 0;
        bsm_tab[i].bs_isprivheap = 0;
        bsm_tab[i].bs_status = 0;
        bsm_tab[i].bs_pid = -1;
        bsm_tab[i].bs_vpno = -1;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = -1;
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    int i;
    for (i = 0; i < NBACKINGSTORE; i++) {
        if (bsm_tab[i].bs_status == 0)
            *avail = i; // store the corresponding backing store
        return OK;
    }
    *avail = -1;
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    if (bsm_tab[i].bs_refcnt != 0 || i < 0 || i >= NBACKINGSTORE)
    {
        return SYSERR;
    }
    bsm_tab[i].bs_refcnt = 0;
    bsm_tab[i].bs_isprivheap = 0;
    bsm_tab[i].bs_status = 0;
    bsm_tab[i].bs_pid = -1;
    bsm_tab[i].bs_vpno = -1;
    bsm_tab[i].bs_npages = 0;
    bsm_tab[i].bs_sem = -1;
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    struct pentry *p = &proctab[pid];
    int i;
    int virt_page_no = (vaddr / NBPG) & 0x000fffff; /* This right shifts the vaddr by 12bits to remove the offset and get the virtual page number */
    
    for (i = 0; i < NBACKINGSTORE; i++) {
        bs_map_t *bptr = &p -> backing_store_map[i];
        if ((bptr->bs_status == 1) && (bptr->bs_vpno <= virt_page_no) && (bptr->bs_vpno + bptr->bs_npages > virt_page_no)) {
            *store = i;
            *pageth = virt_page_no - bptr->bs_vpno; /* the difference b/w starting page_no and the required page_no gives the pageth */
            return OK;
        }
    }
    *store = -1;
    *pageth = -1;
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    struct pentry *p = &proctab[pid];
    if (vpno < NBPG || ( source < 0 || source >= NBACKINGSTORE ) || ( npages < 0 || npages >= NPAGES))
    {
        return SYSERR;
    }
    
    if (bsm_tab[source].bs_status == 0)
    {
        bsm_tab[source].bs_status = 1;
        bsm_tab[source].bs_npages = npages;
    }
        
    if (p -> backing_store_map[source].bs_status == 0)
    {
        p -> backing_store_map[source].bs_status = 1;
        bsm_tab[source].bs_refcnt++; /* if the process is mapping for the first time increase backingstore refcnt */
    }
    
    p -> backing_store_map[source].bs_refcnt = 0;
    p -> backing_store_map[source].bs_isprivheap = 0;
    p -> backing_store_map[source].bs_pid = pid;
    p -> backing_store_map[source].bs_vpno = vpno;
    p -> backing_store_map[source].bs_sem = -1;
    p -> backing_store_map[source].bs_npages = npages;
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    struct pentry *p = &proctab[pid];
    if (vpno < NBPG) {
        return SYSERR; // invalid page number
    }
    
    int store, pageth;
    int result = bsm_lookup(pid, (vpno * NBPG), &store, &pageth);
    if (result == -1 || (store == -1) || (pageth == -1)) {
        return SYSERR;
    }
    else
    {
        /*pageth and store will be storing the backing store page and number respectively */
    }
    if (flag == 1) {
        p -> vmemlist = NULL; // if it is a private heap clear the vmemlist
    }
    /* if it is a backing store map */
    else {
        int given_vpno = vpno;
        int final_page_in_mapping = p -> backing_store_map[store].bs_npages + p -> backing_store_map[store].bs_vpno;
        
        while (given_vpno < final_page_in_mapping) {
        
            unsigned int virt_Addr = given_vpno * NBPG;
            pd_t *page_directory_entry = p -> pdbr + (virt_Addr >> 22) * sizeof(pd_t);
            pt_t *page_table_entry = (page_directory_entry->pd_base) * NBPG + sizeof(pt_t) * ((virt_Addr >> 12) & 0x000003ff);
            
            if (frm_tab[page_table_entry -> pt_base - FRAME0].fr_status == 1)
                free_frm(page_table_entry -> pt_base - FRAME0);
            
            if (page_directory_entry -> pd_pres == 0)
                break;
            given_vpno++;
        }
    }
    
    p -> backing_store_map[store].bs_refcnt = 0;
    p -> backing_store_map[store].bs_isprivheap = 0;
    p -> backing_store_map[store].bs_status = 0;
    p -> backing_store_map[store].bs_pid = -1;
    p -> backing_store_map[store].bs_vpno = -1;
    p -> backing_store_map[store].bs_sem = -1;
    p -> backing_store_map[store].bs_npages = 0;
    
    bsm_tab[store].bs_refcnt--;
    return OK;
}


