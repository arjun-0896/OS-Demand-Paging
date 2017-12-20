/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
    STATWORD ps;
    disable(ps);
    
    int store_id = 0;
    int gbr = get_bsm(&store_id);
    
    if ((store_id == -1) || hsize < 0 || hsize > NPAGES) {
        restore(ps);
        return SYSERR;
    }
    
    int proc_id = create(procaddr, ssize, priority, name, nargs, args);
    struct pentry *p = &proctab[proc_id];
    int bsm_result = bsm_map(proc_id, 4096, store_id, hsize);
    if(bsm_result == SYSERR)
    {
        restore(ps);
        return SYSERR;
    }
    /*To indicate that the backing store is a priv heap and cannot be shared */
    bsm_tab[store_id].bs_isprivheap = 1;
    bsm_tab[store_id].bs_refcnt = 1;
    /*.........................................*/
    p -> store = store_id;
    p -> vhpno = 4096;
    p -> vhpnpages = hsize;
    p -> backing_store_map[store_id].bs_isprivheap = 1;
    
    p -> vmemlist = getmem(sizeof(struct mblock));
    p -> vmemlist->mlen = hsize * NBPG;
    p -> vmemlist->mnext = NULL;
    
    restore(ps);
    return proc_id;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
