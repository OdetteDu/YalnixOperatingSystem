By Odette Du, Bing Xue

TracePrintf Level Guildline

256: Kernel Call
512: Trap

-------1000 Anything related to Region 1 above 1000----------
1020: SetKernelBrk

-------1500 Load Idle Program--------------------------------
1536: LoadProgram
1792: Print UserPageTable after load idle

-------2000 Kernel Page Table Before Virtual Memory---------
2000: Number of page Kernel used before enable virtual memory
2044: Print Initial Kernel Page Table And User Page Table
2048: Allocate pages to Kernel before enalbe virtual memory

-------3000 Physical Page Implementation---------------------
3070: Number of Physical pages available after allocate to Kernel
3072: Print Physical Page Linked List


