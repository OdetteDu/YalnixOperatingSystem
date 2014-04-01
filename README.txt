By Odette Du, Bing Xue

Before Context Switch
1. Virtual Memory is working
2. Load.c is working
3. SetKernelBrk is working
4. Trap Memory is working
5. Brk is working

After Context Switch
1. GetPid is working
2. Delay is working
3. Exec is working
4. Fork works except but the due to problems in exit parent doesn't return properly
5. Wait and Exit is working except the previous bug

In addition, we successfull use the top virtual pages in region 1 to create
page tables for forked process. And we successfully translate the vpn to pfn
in the context switch so that we use physical address to write the page table
address into the register

After Fork
1. We tried tty write, but we have not finished it because of the time limit
