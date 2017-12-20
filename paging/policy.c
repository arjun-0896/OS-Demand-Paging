/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  if(policy == SC )
  {
      page_replace_policy = policy;
      return OK;
  }
  else if(policy == AGING )
  {
      page_replace_policy = policy;
      return OK;
  }
  else
  {
      return SYSERR;
  }
  
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
