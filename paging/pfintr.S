/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
            popl pferrcode // step 1 - store error code in global variable
            pushfl // step 2 - store flag register
            cli  // step 3 - clear all interrupts
            pushal // step 4 - save all general registers
            call pfint // step 5 - page fault handling
            popal // step 6 - restore general registers
            popfl // step 7 - restore flag register
            iret // step 8



