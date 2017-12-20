/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
    STATWORD ps;
    
    disable(ps);
    
    if (size == 0 || block < 4096 * NBPG)
        return SYSERR;
    
    size = roundmb(size);
    struct pentry *p = &proctab[currpid];
    struct mblock *vml = &p->vmemlist;
    struct mblock *prev = vml;
    struct mblock *next = vml->mnext;
    
    while (next != NULL && next < block) {
        prev = next;
        next = next->mnext;
    }
    
    block->mnext = next;
    block->mlen = size;
    prev->mnext = block;
    
    return (OK);
}
