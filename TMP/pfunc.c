/* pfunc.c -  pagetable functions */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
fr_map_t frm_tab[NFRAMES];
int global_page_table[4];

int create_page_table(int pid) {
    
    int i,frm;
    get_frm(&frm);
    if (frm == -1) {
        return SYSERR;
    }
    
    frm_tab[frm].fr_status = 1;
    frm_tab[frm].fr_pid = pid;
    frm_tab[frm].fr_vpno = -1;
    frm_tab[frm].fr_refcnt = 0;
    frm_tab[frm].fr_dirty = 0;
    frm_tab[frm].fr_type = FR_TBL;
    frm_tab[frm].fr_counter = 0;
    
    
    for (i = 0; i < 1024; i++) {
        pt_t *pt = (FRAME0 + frm) * NBPG + i * sizeof(pt_t);
        pt->pt_pres = 0;
        pt->pt_write = 1;
        pt->pt_user = 0;
        pt->pt_pwt = 0;
        pt->pt_pcd = 0;
        pt->pt_acc = 0;
        pt->pt_dirty = 0;
        pt->pt_mbz = 0;
        pt->pt_global = 0;
        pt->pt_avail = 0;
        pt->pt_base = 0;
    }
    return frm;
}

int create_page_directory(int pid) {
    int i,frm;
    get_frm(&frm);
    if (frm == -1) {
        return -1;
    }
    struct pentry *p = &proctab[pid];
    p -> pdbr = (FRAME0 + frm) * NBPG; // Setting base register od the directory of the current process
    
    frm_tab[frm].fr_status = 1;
    frm_tab[frm].fr_pid = pid;
    frm_tab[frm].fr_vpno = -1;
    frm_tab[frm].fr_refcnt = 4;
    frm_tab[frm].fr_type = FR_DIR;
    frm_tab[frm].fr_dirty = 0;
    frm_tab[frm].fr_counter = 0;
    
    
    for (i = 0; i < 1024; i++) {
        pd_t *pd = p -> pdbr + (i * sizeof(pd_t));
        pd->pd_pres = 0;
        pd->pd_write = 1;
        pd->pd_user = 0;
        pd->pd_pwt = 0;
        pd->pd_pcd = 0;
        pd->pd_acc = 0;
        pd->pd_mbz = 0;
        pd->pd_fmb = 0;
        pd->pd_global = 0;
        pd->pd_avail = 0;
        pd->pd_base = 0;
        
        if (i < 4) {
            pd->pd_pres = 1;
            pd->pd_write = 1;
            pd->pd_base = global_page_table[i];
        }
    }
    return frm;
}

int initialize_globalPageTables() {
    int i, j, k;
    for (i = 0; i < 4; i++) {
        k = create_page_table(NULLPROC);
        if (k == -1) {
            return SYSERR;
        }
        global_page_table[i] = FRAME0 + k;
        
        for (j = 0; j < 1024; j++) {
            pt_t *pt = global_page_table[i] * NBPG + j * sizeof(pt_t);
            
            pt->pt_pres = 1;
            pt->pt_write = 1;
            pt->pt_base = j + i * 1024;
            
            frm_tab[k].fr_refcnt++;
        }
    }
    return OK;
}

/* used to update the conrol register 3 to hold the pdbr of the new process in case of context switch */
int update_cr3(int pid) {
    unsigned int pdbr = (proctab[pid].pdbr) / NBPG;
    
    /* check if the process has a valid page directory */
    if ((frm_tab[pdbr - FRAME0].fr_status != 1) || (frm_tab[pdbr - FRAME0].fr_type != FR_DIR) || (frm_tab[pdbr - FRAME0].fr_pid != pid)) {
        return SYSERR;
    }
    write_cr3(proctab[pid].pdbr);
    return OK;
}
