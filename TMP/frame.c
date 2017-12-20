/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

fr_map_t frm_tab[NFRAMES];
static inline void invlpg(void* m)
{
    /* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
    asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}


/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
    int i;
    for (i = 0; i < NFRAMES; i++) {
        frm_tab[i].fr_status = 0;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_vpno = -1;
        frm_tab[i].fr_refcnt = 0;
        frm_tab[i].fr_dirty = 0;
        frm_tab[i].fr_type = -1;
        frm_tab[i].fr_counter = 0;
        
    }  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
    int i;
    for (i = 0; i < NFRAMES; i++) {
        if (frm_tab[i].fr_status == 0) {
            *avail = i;
            return OK;
        }
    }
    if (grpolicy() == SC) {
        i = get_frm_SC();
        *avail = i;
        return OK;
    }
    if (grpolicy() == AGING) {
        i = get_frm_AGING();
        *avail = i;
        return OK;
    }
    return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

    if (i < 0 || i > NFRAMES) {
        return SYSERR; //invalid frame
    }
    
    /* frame is a page and so we have to clear the page frame, pagetable entry holding the page and also if that pagetable has no more references, the page directory entry is removed */
   
    if(frm_tab[i].fr_type == FR_PAGE)
    {
        int store, pageth;
        int bsm_result = bsm_lookup(frm_tab[i].fr_pid, frm_tab[i].fr_vpno * NBPG, &store, &pageth);
        if (bsm_result == SYSERR || store == -1 || pageth == -1)
        {
            kill(frm_tab[i].fr_pid);
            return SYSERR; // Step 12(1)
        }
        if(frm_tab[i].fr_dirty == 1)
        {
            write_bs((i + FRAME0) * NBPG, store, pageth); // Step 12(2)
        }
        pd_t *pd_entry = proctab[frm_tab[i].fr_pid].pdbr + (frm_tab[i].fr_vpno / 1024) * sizeof(pd_t); /* page directory entry of the frame */
        pt_t *pt_entry = (pd_entry->pd_base) * NBPG + (frm_tab[i].fr_vpno % 1024) * sizeof(pt_t); /* page table entry holding the frame.. pd_base gives the base address of the pagetable */
        
        if (pt_entry->pt_pres == 0) {
            return SYSERR; /* the page is not found in the page table entry of the process */
        }
        
        /*Clearing page table entry */
        pt_entry->pt_pres = 0;
        pt_entry->pt_write = 1;
        pt_entry->pt_user = 0;
        pt_entry->pt_pwt = 0;
        pt_entry->pt_pcd = 0;
        pt_entry->pt_acc = 0;
        pt_entry->pt_dirty = 0;
        pt_entry->pt_mbz = 0;
        pt_entry->pt_global = 0;
        pt_entry->pt_avail = 0;
        pt_entry->pt_base = 0;
        
        /* Clearing frame table entry */
        frm_tab[i].fr_status = 0;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_vpno = -1;
        frm_tab[i].fr_refcnt = 0;
        frm_tab[i].fr_type = -1;
        frm_tab[i].fr_dirty = 0;
        frm_tab[i].fr_counter = 0;
        
        frm_tab[pd_entry->pd_base - FRAME0].fr_refcnt--;
        if (frm_tab[pd_entry->pd_base - FRAME0].fr_refcnt <= 0) {
            free_frm(pd_entry->pd_base - FRAME0);
        }
        return OK;
    }
    
    /*the frame is a page table and so we have to free all the corresponding pages' frames and the clear the directory entry and then clear the frame*/
    
    if(frm_tab[i].fr_type == FR_TBL)
    {
        int c;
        for (c = 0; c < no_of_entries; c++)
        {
            pt_t *pt_entry = (i + FRAME0) * NBPG + c * sizeof(pt_t);
            if (pt_entry->pt_pres == 1)
            {
                free_frm(pt_entry->pt_base - FRAME0);
            }
        }
            /* Clearing page directory entry */
        for (c = 0; c < no_of_entries ; c++)
        {
            pd_t *pd_entry = proctab[frm_tab[i].fr_pid].pdbr + c * sizeof(pd_t);
            if (pd_entry->pd_base - FRAME0 == i)
            {
                pd_entry->pd_pres = 0;
                pd_entry->pd_write = 1;
                pd_entry->pd_user = 0;
                pd_entry->pd_pwt = 0;
                pd_entry->pd_pcd = 0;
                pd_entry->pd_acc = 0;
                pd_entry->pd_mbz = 0;
                pd_entry->pd_fmb = 0;
                pd_entry->pd_global = 0;
                pd_entry->pd_avail = 0;
                pd_entry->pd_base = 0;
            }
        }
            
           /* Clearing frame table entry */
            frm_tab[i].fr_status = 0;
            frm_tab[i].fr_pid = -1;
            frm_tab[i].fr_vpno = -1;
            frm_tab[i].fr_refcnt = 0;
            frm_tab[i].fr_type = -1;
            frm_tab[i].fr_dirty = 0;
            frm_tab[i].fr_counter = 0;
            return OK;
        }
    
    /* frame is a directory and so clear all the pagetable frames referenced and clear the frame table entry */
    
        if(frm_tab[i].fr_type == FR_DIR)
        {
            int j;
            /* first four frames is for global page table and hence start from 4 */
            for (j = 4; j < no_of_entries; j++) {
                pd_t *pd_entry = proctab[frm_tab[i].fr_pid].pdbr + j * sizeof(pd_t);
                if (pd_entry->pd_pres == 1) {
                    free_frm(pd_entry->pd_base - FRAME0);
                }
            }
            frm_tab[i].fr_status = 0;
            frm_tab[i].fr_pid = -1;
            frm_tab[i].fr_vpno = -1;
            frm_tab[i].fr_refcnt = 0;
            frm_tab[i].fr_type = -1;
            frm_tab[i].fr_dirty = 0;
            frm_tab[i].fr_counter = 0;
            return OK;
        }
    
    return SYSERR;
}

void insert_frame_into_fifoqueue(int f)
{
    FQ *tmp = (FQ *) getmem(sizeof(FQ));
    tmp->fid = f;
    tmp->next_frame = F_head.next_frame;
    F_head.next_frame = tmp;
    n_allocated_pages++;
}

int get_frm_SC()
{
    unsigned long a;
    int i, curr_fid, curr_pid, curr_vpno;
    FQ *tmp = &F_head;
    for (i = 0; i < n_allocated_pages; i++)
    {
        tmp = tmp->next_frame;
        curr_fid = tmp -> fid;
        curr_vpno = frm_tab[curr_fid].fr_vpno;
        curr_pid = frm_tab[curr_fid].fr_pid;
        struct pentry *p = &proctab[curr_pid];
        a = curr_vpno * NBPG;
        pd_t *pd = p -> pdbr + (a >> 22) * sizeof(pd_t);
        pt_t *pt = (pd->pd_base) * NBPG + sizeof(pt_t) * ((a >> 12) & 0x000003ff);
        if(pt -> pt_acc == 0)
        {
            break;
        }
        /* implemented when the entire list is done with all pt_acc = 1 */
        if(pt -> pt_acc == 1 && i == (n_allocated_pages - 1))
        {
            pt -> pt_acc = 0;
            i = 0;
            tmp = &F_head;
            continue;
        }
        if(pt -> pt_acc == 1)
        {
            pt -> pt_acc == 0;
        }
    }
    n_allocated_pages--;
    if(curr_pid == currpid)
    {
        invlpg(a); // to invalidate the TLB entry (Step 10)
    }
    free_frm(tmp -> fid);
    freemem(tmp, sizeof(FQ));
    return tmp -> fid;
}

int get_frm_AGING()
{
    STATWORD ps;
    unsigned long a;
    int i, curr_fid, curr_pid, curr_vpno, rframe;
    FQ *tmp = &F_head;
    FQ *tmp2;
    unsigned long int min_counter = 300;
    fr_map_t *t_frame;
    disable(ps);
    for (i = 0; i < n_allocated_pages; i++)
    {
        tmp = tmp -> next_frame;
        curr_fid = tmp -> fid;
        t_frame = &frm_tab[curr_fid];
        
        if (t_frame -> fr_counter < min_counter)
        {
            min_counter = t_frame -> fr_counter;
            tmp2 = tmp;
        }
            
        else if (t_frame->fr_counter == min_counter)
        {
            /* the frame close to header of fifo queue is taken */
            continue;
        }
    }
    int curr_fid2;
    fr_map_t *t_frame2;
    curr_fid2 = tmp2 -> fid;
    t_frame2 = &frm_tab[curr_fid2];
    curr_vpno = t_frame2 -> fr_vpno;
    curr_pid = t_frame2 -> fr_pid;
    struct pentry *p = &proctab[curr_pid];
    a = curr_vpno * NBPG;
    n_allocated_pages--;
    if(curr_pid == currpid)
    {
        invlpg(a); // to invalidate the TLB entry (Step 10)
    }
    free_frm(tmp2 -> fid);
    freemem(tmp2, sizeof(FQ));
    restore(ps);
    return tmp2 -> fid;
}

int AGING_update_counter()
{
    unsigned long a;
    int i, curr_fid, curr_pid, curr_vpno;
    FQ *tmp = &F_head;
    for (i = 0; i < n_allocated_pages; i++)
    {
        tmp = tmp->next_frame;
        curr_fid = tmp -> fid;
        curr_vpno = frm_tab[curr_fid].fr_vpno;
        curr_pid = frm_tab[curr_fid].fr_pid;
        struct pentry *p = &proctab[curr_pid];
        a = curr_vpno * NBPG;
        pd_t *pd = p -> pdbr + (a >> 22) * sizeof(pd_t);
        pt_t *pt = (pd->pd_base) * NBPG + sizeof(pt_t) * ((a >> 12) & 0x000003ff);
        frm_tab[curr_fid].fr_counter = frm_tab[curr_fid].fr_counter >> 1;
        if(pt -> pt_acc == 1)
        {
            frm_tab[curr_fid].fr_counter = (frm_tab[curr_fid].fr_counter) + 128;
            pt -> pt_acc = 0;
        }
        
    }
    
}




    
    

