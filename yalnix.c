#include <yalnix.c>
#include <hardware.c>

extern int Fork(void)
{

}

extern int Exec(char *, char **)
{

}

extern void Exit(int) __attribute__ ((noreturn))
{

}

extern int Wait(int *)
{

}

extern int GetPid(void)
{

}

extern int Brk(void *)
{

}

extern int Delay(int)
{

}

extern int TtyRead(int, void *, int)
{

}

extern int TtyWrite(int, void *, int)
{

}

extern int Register(unsigned int)
{

}

extern int Send(void *, int)
{

}

extern int Receive(void *)
{

}

extern int ReceiveSpecific(void *, int)
{

}

extern int Reply(void *, int)
{

}

extern int Forward(void *, int, int)
{

}

extern int CopyFrom(int, void *, void *, int)
{

}

extern int CopyTo(int, void *, void *, int)
{

}

extern int ReadSector(int, void *)
{

}

extern int WriteSector(int, void *)
{

}

extern int DiskStats(struct diskstats *)
{

}


