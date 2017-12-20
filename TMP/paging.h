/* paging.h */
#ifndef _PAGING_H_
#define _PAGING_H_
typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct{
  int bs_refcnt;            /* To check how many processes are mapped to the store */
  int bs_isprivheap;        /* Private heap of a process or not */
  int bs_status;            /* MAPPED or UNMAPPED        */
  int bs_pid;				/* process id using this slot   */
  int bs_vpno;				/* starting virtual page number */
  int bs_npages;			/* number of pages in the store */
  int bs_sem;				/* semaphore mechanism ?	*/
} bs_map_t;

typedef struct{
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;
  int fr_counter;
}fr_map_t;

extern bs_map_t bsm_tab[]; // To hold all backing stores alloated
extern fr_map_t frm_tab[]; // To hold all the frames allocated
extern int page_replace_policy; // To hold the page replacement policy
extern int global_page_table[]; // To hold the 4 pages to store first 16 MB of memory

int get_SC_frame();
int get_AGING_frame();
void insert_frame_into_fifoqueue(int);
int AGING_update_counter();

/* for keeping track of the allocated frames */
typedef struct _FQ{
    int fid;
    struct _FQ *next_frame;
}FQ;

extern FQ F_head;
extern int n_allocated_pages;


/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);

/* given calls for dealing with backing store mapping */

SYSCALL init_bsm();
SYSCALL get_bsm(int* avail);
SYSCALL free_bsm(int i);
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth);
SYSCALL bsm_map(int pid, int vpno, int source, int npages);
SYSCALL bsm_unmap(int pid, int vpno, int flag);

/* given calls for dealing with frames */

SYSCALL init_frm();
SYSCALL get_frm(int* avail);
SYSCALL free_frm(int i);

/* given calls for dealing with page tables and directories */

int create_page_table(int pid);
int create_page_directory(int pid);
int initialize_globalPageTables();

int update_cr3(int pid); /* function to write the page directory base address of process to control register 3 */

#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/
#define NFRAMES 	1024	/* number of frames		*/
#define NBACKINGSTORE 8     /* number of backing stores */
#define NPAGES  256         /* number of peges per backing stores */

#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2
#define no_of_entries 1024

#define SC 3
#define AGING 4

#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000

#endif
