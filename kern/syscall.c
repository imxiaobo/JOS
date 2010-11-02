/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	
	// LAB 3: Your code here.
	user_mem_assert(curenv, (void *)s, len, PTE_U) ;
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}



static void * system_call_table[] = {
	(void *)sys_cputs ,
	(void *)sys_cgetc ,
	(void *)sys_getenvid ,
	(void *)sys_env_destroy ,
} ;

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, 
		uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	int ret = 0 ;
	void * func = NULL ;
	cprintf("func: %p\n", func) ;
	// check if syscallno is valid
	if (syscallno < sizeof(system_call_table) / sizeof(void *) ) {
		// hook up the appropriate syscall
		func = system_call_table[syscallno] ;
		cprintf("func: %p\n", func) ;
		// prepare a system call, push arguments into stack
		asm volatile("push %%esi\n"
					 "push %%edi\n"
					 "push %%ebx\n"
					 "push %%ecx\n"
					 "push %%edx\n"
					 "call *%1\n"
					 : "=a" (ret)
					 : "m" (func),
					 "d" (a1),
					 "c" (a2),
					 "b" (a3),
					 "D" (a4),
					 "S" (a5)
					 : "cc", "memory") ;
		return ret;
	}
	
	// invalid syscallno
	return -E_INVAL ;
	
}

