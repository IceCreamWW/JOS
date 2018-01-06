// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
extern void _pgfault_upcall();

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	if (!(err & FEC_WR))
		panic("fork.c : pgfault recieves fault not cased by write\n");

	if (!((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW)))
		panic("fork.c : pgfault recieves fault not on copy-on-write page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	
	// LAB 4: Your code here.
	
	// addr is a fault address, it might not be page aligned.
	// Although this does not matter for PDX and PGNUM because they simply ignore the last 12 bit,
	// system calls about page will complain
	addr = ROUNDDOWN(addr, PGSIZE);
	if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_P|PTE_U) < 0)
		panic("fork.c : pgfault fail on page alloc");
	memcpy(PFTEMP, addr, PGSIZE);
	if (sys_page_map(0, PFTEMP, 0, addr, PTE_U|PTE_P|PTE_W) < 0)
		panic("fork.c : pgfault fail on page map");
	if (sys_page_unmap(0, PFTEMP) < 0)
		panic("fork.c : pgfault fail on page unmap");
	return;
	
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// int r;
	
	// LAB 4: Your code here.
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)){
		if (sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_COW|PTE_P|PTE_U) < 0)
			panic("fork.c : duppage fail on page_map, perm : PTE_P|PTE_U|PTE_COW");
		if (sys_page_map(0, (void *)(pn * PGSIZE), 0, (void *)(pn * PGSIZE), PTE_COW|PTE_P|PTE_U) < 0)
			panic("fork.c : duppage fail on page_map self, perm : PTE_P|PTE_U|PTE_COW");
		// the line below is not possible because uvpt must be read only
		// otherwise user would be able to change its own permission
		// uvpt[pn] = (uvpt[pn] & (~PTE_W)) | PTE_COW;
	} else {
		if (sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_P|PTE_U) < 0)
			panic("fork.c : duppage fail on page_map, perm : PTE_P|PTE_U");
	}
	return 0;
	// panic("duppage not implemented");
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();

	cprintf("envid = %d\n", envid);
	// debug fork
	if (envid < 0) {
		panic("fork.c : fork fail on set sys_exofork");
	}

	// if we are in child environment, fix our thisenv
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		// bellow line won't work, cannot wait for getting into child environment before set_pgfault_handler
		
		// Error takes place like this :
		// 1. parent call sys_fork;
		// 2. return in child
		// 3. write return value in variable envid
		// 4. a "write on copy-on-write" page fault is trapped
		// 5. no upcall has been set yet, as a result trap.c print trapframe and destroys environment 
		
		// thus upcal should be set in parent environment and do an extra work to allocate exception stack page
		// set_pgfault_handler(pgfault);
		return 0;
	} else {
		uintptr_t vaddr = 0;

		for (vaddr = 0; vaddr < USTACKTOP; vaddr += PGSIZE) {
			if ((uvpd[PDX(vaddr)] & PTE_P) && (uvpt[PGNUM(vaddr)] & PTE_P) && (uvpt[PGNUM(vaddr)] & PTE_P)) {
				
				// uncomment the following line and we got same result for each environment
				
				// vaddr = 00200000
				// vaddr = 00201000
				// vaddr = 00202000
				// vaddr = 00203000
				// vaddr = 00204000
				// vaddr = 00800000
				// vaddr = 00801000
				// vaddr = 00802000
				// vaddr = eebfd000
				
				// And that's  [User STAB Data][5 pages], [Program Data and Heap][3 pages], [Normal User Stack][1 page]

				// So Quetions is : 
				// 		Except for Normal User Stack, When did I map other virtual address ?
				
				// cprintf("vaddr = %08x\n", vaddr);
				duppage(envid, PGNUM(vaddr));
			}
		}
		if (sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U|PTE_P|PTE_W) < 0) {
			panic("fork.c : fork fail page alloc");
		};	
		sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
		if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
			panic("fork.c : fork fail on set status");
		return envid;
	}
}	




// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}