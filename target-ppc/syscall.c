#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>


#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "sim.h"
#include "eio.h"
#include "syscall.h"
#include "assert.h"

#include <errno.h>

#include "target-ppc/syscall_types.h"

__thread md_addr_t errnoaddress;

void
sys_syscall(struct regs_t *regs, /* registers to access */
       mem_access_fn mem_fn,  /* generic memory accessor */
       struct mem_t *mem,     /* memory space to access */
       md_inst_t inst,     /* system call inst */
       int traceable)      /* traceable system call? */
{
	printf("syscall\n");
}

/*
  ssize_t write(FileDescriptor, Buffer, NBytes)
  int FileDescriptor;
  void *Buffer;
  size_t NBytes;
*/


void syscall_kwrite(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   char *buf;
   int count;
   int retval;
	int e;
   count = regs->regs_R[5];
   buf = (char *) malloc(count+20);
   assert (buf != NULL);
   mem_bcopy(my_mem_fn, mem, Read, regs->regs_R[4], buf, count);
   retval = write(regs->regs_R[3], buf, count);
   e = MD_SWAPW(errno); // jdonald
   regs->regs_R[3] = retval;  /* return number of chars written */
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
   free(buf);
}

/*
ssize_t read (FileDescriptor, Buffer, NBytes)
int FileDescriptor;
void *Buffer;
size_t NBytes;
*/

void syscall_kread(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   char *buf;
   int retval;
   int e;
	
   buf = (char *) malloc(regs->regs_R[5]+1);
   assert (buf != NULL);
   retval = read(regs->regs_R[3], buf, regs->regs_R[5]);
   e = MD_SWAPW(errno); // jdonald one line shrink
   mem_bcopy(my_mem_fn, mem, Write, regs->regs_R[4], buf, retval);
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
   regs->regs_R[3] = retval;
   free(buf);
}

void syscall_open(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   char buf[PPC_SYSCALL_BUFFER];
   int retval;
   int e;
   mem_strcpy(mem_access, mem, Read, regs->regs_R[3], buf);
   retval = open(buf, regs->regs_R[4], regs->regs_R[5]);
   e = MD_SWAPW(errno);
   regs->regs_R[3] = retval;
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
}

void syscall_creat(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   char buf[PPC_SYSCALL_BUFFER];
   int retval;
	int e;
   mem_strcpy(mem_access, mem, Read, regs->regs_R[3], buf);
   retval = creat(buf, regs->regs_R[4]);
   e = MD_SWAPW(errno);
   regs->regs_R[3] = retval;
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
}


/*lseek (FileDescriptor, Offset, Whence)
int FileDescriptor, Whence;
off_t Offset;*/

void syscall_klseek(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   int filedescriptor, whence;
	int e;
   off_t offset, retval;
	//fprintf(stderr, "klseek called %x\n", regs->regs_PC);
   filedescriptor = regs->regs_R[3];
   whence = regs->regs_R[5];
   offset = regs->regs_R[4];
   retval = lseek(filedescriptor, offset, whence);
   e = MD_SWAPW(errno);
   regs->regs_R[3] = retval;
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
}

void syscall_lseek(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
  //fprintf(stderr, "lseek called %x\n", regs->regs_PC);
  syscall_klseek(regs, mem, my_mem_fn);
  /* may need to correct this */
}

void syscall_close(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
  int retval;
  int e;
  retval = close(regs->regs_R[3]);
  e = MD_SWAPW(errno);
  regs->regs_R[3] = retval;
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
}

/* int ioctl (fd, request, ...arg)
int fd;
int request;
int arg */

void syscall_kioctl(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {

#if defined(linux)
  warn("Unimplemented linux ioctl call");
  regs->regs_R[3] = 0;
   /* basically ignoring call :-) */
#elif defined(SOLARIS)
  int fd, request, arg, retval;
  int e;
  
  arg = regs->regs_R[5];
  fd = regs->regs_R[3];
  request = regs->regs_R[4];

  retval = ioctl(fd, request, arg);
  e = MD_SWAPW(errno); // jdonald: in case x86 solaris
  fprintf(stderr, "kioctl called %x %x %x %x returning %x\n", regs->regs_PC,
	  regs->regs_R[3], regs->regs_R[4], regs->regs_R[5], retval);
  if (retval == -1) fprintf(stderr, "ERROR %x\n", errno);
  
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
  regs->regs_R[3] = retval;
#else
  warn("Unimplemented ioctl call");
  regs->regs_R[3] = 0;
#endif
}


void syscall_exit(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   /* basically ignoring call :-) */
}


void syscall_sigprocmask(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   regs->regs_R[3] = 0;
   /* basically ignoring call :-) */
}

void syscall_sigcleanup(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   regs->regs_R[3] = 0;
   /* basically ignoring call :-) */
}


void syscall_kfcntl(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
  int fd, command, retval;
#ifndef linux
  int arg;
#else
  long arg;
#endif
  int e;
  fd = regs->regs_R[3];
  command = regs->regs_R[4];
  arg = regs->regs_R[5];
  retval = fcntl(fd, command, arg);
  e = MD_SWAPW(errno); // jdonald
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
  regs->regs_R[3] = retval;
}

void syscall_brk(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   md_addr_t addr;


   /* round the new heap pointer to the its page boundary */
   addr = regs->regs_R[3];
   ld_brk_point = addr;
   regs->regs_R[3] = 0;
   /* check whether heap area has merged with stack area. to be done! */
}

void syscall_sbrk(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   md_addr_t addr;


   /* round the new heap pointer to the its page boundary */
   addr = ld_brk_point + regs->regs_R[3];
   regs->regs_R[3] = ld_brk_point;
   ld_brk_point = addr;
   /* error checkinffor heap stack merge to be done */
}

/*
int fstatx (FileDescriptor, Buffer, Length, Command)
int FileDescriptor;
struct stat *Buffer;
int Length;
int Command;
*/

void syscall_fstatx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
	int fd;
	int e;
   struct stat buffer;
#if (defined SOLARIS || defined linux)
   struct aix_stat aix_stat_buffer;
#endif
   int length, command, retval;
   enum md_fault_type _fault;

   _fault = md_fault_none;
   fd = regs->regs_R[3];
	length = regs->regs_R[5];
   command = regs->regs_R[6];
#ifdef SOLARIS
   retval = fstat (fd, &buffer );
   aix_stat_buffer.st_dev = buffer.st_dev;
   aix_stat_buffer.st_ino = buffer.st_ino;
   aix_stat_buffer.st_mode = buffer.st_mode;
   aix_stat_buffer.st_nlink = buffer.st_nlink;
   // aix_stat_buffer.st_flag = buffer.st_flag;
   aix_stat_buffer.st_uid = buffer.st_uid;
   aix_stat_buffer.st_gid = buffer.st_gid;
   aix_stat_buffer.st_rdev = buffer.st_rdev ;
   aix_stat_buffer.st_size = buffer.st_size ;
   aix_stat_buffer.statime = buffer.st_atim.tv_sec ;
   aix_stat_buffer.stmtime = buffer.st_mtim.tv_sec ;
   aix_stat_buffer.stctime  = buffer.st_ctim.tv_sec ;
   aix_stat_buffer.st_blksize  = buffer.st_blksize ;
   aix_stat_buffer.st_blocks  = buffer.st_blocks ;
   aix_stat_buffer.st_vfstype = *((int *) &buffer.st_fstype[0]) ;
   aix_stat_buffer.st_vfs = *((int *) &buffer.st_fstype[4]) ;
   aix_stat_buffer.st_type = *((int *) &buffer.st_fstype[8]) ;
   aix_stat_buffer.st_gen = *((int *) &buffer.st_fstype[12]) ;	
   _fault = mem_bcopy(mem_access, mem, Write, regs->regs_R[4], &aix_stat_buffer, length);
#elif linux
   retval = fstat (fd, &buffer );
   e = MD_SWAPW(errno);
   //fprintf(stderr, "errno %x\n", e);
   aix_stat_buffer.st_dev = buffer.st_dev;
   aix_stat_buffer.st_ino = buffer.st_ino;
   aix_stat_buffer.st_mode = buffer.st_mode;
   aix_stat_buffer.st_nlink = buffer.st_nlink;
   // aix_stat_buffer.st_flag = buffer.st_flag;
   aix_stat_buffer.st_uid = buffer.st_uid;
   aix_stat_buffer.st_gid = buffer.st_gid;
   aix_stat_buffer.st_rdev = buffer.st_rdev ;
   aix_stat_buffer.st_size = buffer.st_size ;
   aix_stat_buffer.statime = buffer.st_atime;
   aix_stat_buffer.stmtime = buffer.st_mtime;
   aix_stat_buffer.stctime  = buffer.st_ctime;
   aix_stat_buffer.st_blksize  = buffer.st_blksize ;
   aix_stat_buffer.st_blocks  = buffer.st_blocks ;
   /*aix_stat_buffer.st_vfstype = *((int *) &buffer.st_fstype[0]) ;
   aix_stat_buffer.st_vfs = *((int *) &buffer.st_fstype[4]) ;
   aix_stat_buffer.st_type = *((int *) &buffer.st_fstype[8]) ;
   aix_stat_buffer.st_gen = *((int *) &buffer.st_fstype[12]) ;	*/
   _fault = mem_bcopy(mem_access, mem, Write, regs->regs_R[4], &aix_stat_buffer, length);
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#else
   retval = fstatx (fd, &buffer, length, command);
   e = MD_SWAPW(errno); // jdonald: in case any little endian platform other than linux
   _fault = mem_bcopy(my_mem_fn, mem, Write, regs->regs_R[4], &buffer, length);
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#endif
   if (_fault != md_fault_none) fatal( "memory fault in fstatx");
   regs->regs_R[3] = retval;
}

/*
int statx (Path, Buffer, Length, Command)
char *Path;
struct stat *Buffer;
int Length;
int Command;
*/

void syscall_statx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   char path[PPC_SYSCALL_BUFFER];
   struct stat buffer;
	int e;
#if (defined (SOLARIS) || defined (linux)) 
	struct aix_stat aix_stat_buffer;
#endif
   int length, command, retval;
   enum md_fault_type _fault;
  
   _fault = md_fault_none;
   _fault = mem_strcpy(mem_access, mem, Read, regs->regs_R[3], path);
   if (_fault != md_fault_none) fatal( "memory fault in statx");

   length = regs->regs_R[5];
   command = regs->regs_R[6];
#ifdef SOLARIS
   retval = stat( path, &buffer);
   aix_stat_buffer.st_dev = buffer.st_dev;
   aix_stat_buffer.st_ino = buffer.st_ino;
   aix_stat_buffer.st_mode = buffer.st_mode;
   aix_stat_buffer.st_nlink = buffer.st_nlink;
   // aix_stat_buffer.st_flag = buffer.st_flag;
   aix_stat_buffer.st_uid = buffer.st_uid;
   aix_stat_buffer.st_gid = buffer.st_gid;
   aix_stat_buffer.st_rdev = buffer.st_rdev ;
   aix_stat_buffer.st_size = buffer.st_size ;
   aix_stat_buffer.statime = buffer.st_atim.tv_sec ;
   aix_stat_buffer.stmtime = buffer.st_mtim.tv_sec ;
   aix_stat_buffer.stctime  = buffer.st_ctim.tv_sec ;
   aix_stat_buffer.st_blksize  = buffer.st_blksize ;
   aix_stat_buffer.st_blocks  = buffer.st_blocks ;
   aix_stat_buffer.st_vfstype = *((int *) &buffer.st_fstype[0]) ;
   aix_stat_buffer.st_vfs = *((int *) &buffer.st_fstype[4]) ;
   aix_stat_buffer.st_type = *((int *) &buffer.st_fstype[8]) ;
   aix_stat_buffer.st_gen = *((int *) &buffer.st_fstype[12]) ;
   _fault = mem_bcopy(mem_access, mem, Write, regs->regs_R[4], &aix_stat_buffer, length);
#elif linux
   retval = stat(path, &buffer);
   e = MD_SWAPW(errno);
   aix_stat_buffer.st_dev = buffer.st_dev;
   aix_stat_buffer.st_ino = buffer.st_ino;
   aix_stat_buffer.st_mode = buffer.st_mode;
   aix_stat_buffer.st_nlink = buffer.st_nlink;
   aix_stat_buffer.st_uid = buffer.st_uid;
   aix_stat_buffer.st_gid = buffer.st_gid;
   aix_stat_buffer.st_rdev = buffer.st_rdev ;
   aix_stat_buffer.st_size = buffer.st_size ;
   aix_stat_buffer.statime = buffer.st_atime;
   aix_stat_buffer.stmtime = buffer.st_mtime;
   aix_stat_buffer.stctime = buffer.st_ctime;
   aix_stat_buffer.st_blksize  = buffer.st_blksize ;
   aix_stat_buffer.st_blocks  = buffer.st_blocks ;
   /* aix_stat_buffer.st_vfstype = *((int *) &buffer.st_fstype[0]) ;
      aix_stat_buffer.st_vfs = *((int *) &buffer.st_fstype[4]) ;
      aix_stat_buffer.st_type = *((int *) &buffer.st_fstype[8]) ;
      aix_stat_buffer.st_gen = *((int *) &buffer.st_fstype[12]) ;*/
   //fprintf(stderr, "errno %x\n", e);
   _fault = mem_bcopy(my_mem_fn, mem, Write, regs->regs_R[4], &aix_stat_buffer, length);
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#else
   retval = statx (path, &buffer, length, command);
   e = MD_SWAPW(errno); // jdonald: in case any little-endian platform other than linux
   _fault = mem_bcopy(my_mem_fn, mem, Write, regs->regs_R[4], &buffer, length);
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#endif
   if (_fault != md_fault_none) fatal( "memory fault in statx");
   regs->regs_R[3] = retval;
}

void syscall_getgidx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
	gid_t g;
	g = getgid();
	regs->regs_R[3] = g;
}

void syscall_getuidx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
	uid_t t;
	t = getuid();
	regs->regs_R[3] = t;
}

void syscall_getpid(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
   pid_t t;
   t = getpid();
	fprintf(stderr, "getpid returning %x\n", t);
   regs->regs_R[3] = t;
}

void syscall_kill(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
	fprintf(stderr, "Kill %x %x\n", regs->regs_R[3], regs->regs_R[4]);
	regs->regs_R[3] = 0;
}

/* 
int access (PathName, Mode)
char *PathName;
int Mode;
*/

void syscall_access(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
  char *buf;
  int retval;
  int e;
  
  buf = (char *) malloc(PPC_SYSCALL_BUFFER);
  assert (buf != NULL);
  mem_bcopy(mem_access, mem, Read, regs->regs_R[3], buf, PPC_SYSCALL_BUFFER);
  retval = access(buf, regs->regs_R[4]);
  e = MD_SWAPW(errno); // jdonald
  regs->regs_R[3] = retval;  /* return status */
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
  free(buf);
}

/*
int accessx (PathName, Mode, Who)
char *PathName;
int Mode, Who;

*/
void syscall_accessx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
  char *buf;
  int retval;
  int e;

#ifdef SOLARIS

#elif linux
  buf = (char *) malloc(PPC_SYSCALL_BUFFER);
  assert (buf != NULL);
  mem_bcopy(mem_access, mem, Read, regs->regs_R[3], buf, PPC_SYSCALL_BUFFER);
  retval = access(buf, regs->regs_R[4]);
  e = MD_SWAPW(errno);
  regs->regs_R[3] = retval;  /* return status */
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
  free(buf);
#else  
  buf = (char *) malloc(PPC_SYSCALL_BUFFER);
  assert (buf != NULL);
  mem_bcopy(mem_access, mem, Read, regs->regs_R[3], buf, PPC_SYSCALL_BUFFER);
  retval = accessx(buf, regs->regs_R[4], regs->regs_R[5]);
  e = MD_SWAPW(errno); // jdonald: in case x86 solaris
  regs->regs_R[3] = retval;  /* return status */
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
  free(buf);
#endif
}

/* 
int faccessx (FileDescriptor, Mode, Who)
int FileDescriptor;
int Mode, Who;
*/

void syscall_faccessx(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {

#if defined(linux) // dude he used #if instead of #ifdef /jdonald
   fatal("Unimplemented linux syscall faccessx");
#elif defined(SOLARIS)
   int retval;
   int e;
      retval = faccessx(regs->regs_R[3], regs->regs_R[4], regs->regs_R[5]);
   e = MD_SWAPW(errno); // jdonald: in case x86 solaris
   regs->regs_R[3] = retval;  /* return status */
   mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#else
   fatal("Unimplemented syscall faccessx");
#endif
}

/*
off_t fclear (FileDescriptor, NumberOfBytes)
int FileDescriptor;
off_t NumberOfBytes;
*/

void syscall_fclear(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {

#if defined(linux) // jdonald fixed to #ifdef instead of #if
  fatal("Unimplemented linux syscall fclear");
#elif defined(SOLARIS)
  int retval, e;

  off_t o;
  o = regs->regs_R[4];
  retval = fclear(regs->regs_R[3], regs->regs_R[4]);
  e = MD_SWAPW(errno); // jdonald: in case x86 solaris
  regs->regs_R[3] = retval;  /* return status */
  mem_bcopy(my_mem_fn, mem, Write, errnoaddress, &e, 4); /* set errno */
#else
  fatal("Unimplemented syscall fclear");
#endif
}


void syscall_unimplemented(struct regs_t *regs, struct mem_t *mem, mem_access_fn my_mem_fn) {
	regs->regs_R[3] = 0;
	printf("unimplemented system call ");
	exit(1);
	/* basically ignoring call :-) */
}

void dosyscall(int r, struct regs_t *regs, struct mem_t *mem, 
			   mem_access_fn mem_fn) {
  if (sim_eio_fd != NULL) {
	eio_read_trace(sim_eio_fd, sim_num_insn, regs, mem_access, mem, 0);
	return;
  }
  fprintf(stderr, "SYSCALL:: %x %x\n", regs->regs_PC, r);
  errno = 0;
  switch (r) {
      case SYSCALL_VALUE_SBRK: syscall_sbrk(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_BRK: syscall_brk(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KWRITE: syscall_kwrite(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KIOCTL: syscall_kioctl(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KFCNTL: syscall_kfcntl(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_EXIT: syscall_exit(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KREAD: syscall_kread(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_STATX: syscall_statx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_OPEN: syscall_open(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_CLOSE: syscall_close(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_SIGPROCMASK: syscall_sigprocmask(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KLSEEK: syscall_klseek(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_SIGCLEANUP: syscall_sigcleanup(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_LSEEK: syscall_lseek(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_FSTATX: syscall_fstatx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_GETGIDX: syscall_getgidx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_GETUIDX: syscall_getuidx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_GETPID: syscall_getpid(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_KILL: syscall_kill(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_CREAT: syscall_creat(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_ACCESS: syscall_access(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_ACCESSX: syscall_accessx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_FACCESSX: syscall_faccessx(regs, mem, mem_fn); break;
      case SYSCALL_VALUE_FCLEAR: syscall_fclear(regs, mem, mem_fn); break;
		default: 
			fprintf(stderr, "unimplemented syscall %x %d\n", regs->regs_PC, r); 
			regs->regs_R[3] = 0;
			break;
   }
}

