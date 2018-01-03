// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line



struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};



static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "print", "Test cprintf function", mon_printf},
	{ "backtrace", "Get backtrace display for exercise 11", mon_backtrace},
	{ "showmappings", "get key mappings info in an addr range", mon_showmappings},
	{ "setperm", "set or clear a mapping permission", mon_setperm},
	{ "dump", "dump content in a physical or virtual addr range", mon_dump}
};

/***** Manipulate Input and Ouput Format and Type *****/

/* Convert *clean* (Ex. hex without '0x' prefix) char* number to uint_32 */
uint32_t 
atoi(char* number, int radix, int *error) {
	const char *digits = "0123456789abcdef";
	uint32_t result = 0;
	char* index_ch = NULL;

	while(*number) {
		if ( (index_ch = strchr(digits, *number)) == NULL) {
			if (error) *error = *error | 1;
			return 0;
		} else {
			result  = (result * radix) + (index_ch - digits);
			++number;
		}
	}
	if (error)  *error = *error | 0;
	return result;
}
void 
showmapping(uintptr_t addr) {
		pte_t *pte_pt = pgdir_walk(kern_pgdir, (void *)addr, false);
		if (!pte_pt)
			cprintf("%08x\tnot mapped yet\n", addr);
		else {
			cprintf("%08x\t%08x\t\tPTE_P: %d, PTE_W: %d, PTE_U: %d\n",
					addr, PTE_ADDR(*pte_pt),
					*pte_pt & PTE_P, (*pte_pt & PTE_W) >> 1, (*pte_pt & PTE_U) >> 2);
		}
}

void
dump(uint32_t addr, int is_va) {
	if (is_va) {
		pte_t *pte_pt = pgdir_walk(kern_pgdir, (void *)addr, false);
		if (!pte_pt) {
			cprintf("va %08x : not mapped yet\n", addr);
		} else {
			cprintf("va %08x : %08x\n", addr, *((uint32_t *)addr));
		}
	} else {
		if (PGNUM(addr) >= npages) {
			cprintf("pa %08x : not accessable \n", addr);
		} else {
			cprintf("pa %08x : %08x\n", addr,*((uint32_t *)(KADDR(addr))));
		}
	}
}
/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int mon_printf(int argc, char **argv, struct Trapframe* tf){
	int x = 1, y = 3, z = 4;
	cprintf("x %d, y %x\n",3, 4);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp();

	while (*(uint32_t*)ebp) {
		struct Eipdebuginfo info;
		uint32_t eip = *((uint32_t*)ebp + 1);
		uint32_t argv[5];
		int i;
		for (i=1; i<=5; ++i) {
			argv[i - 1] = *((uint32_t*)ebp + 1 + i);
		}
		debuginfo_eip(eip, &info);

		/* Format funciton name */
		char *index = strchr(info.eip_fn_name, ':');
		uint32_t size = index - info.eip_fn_name + 1;
		char eip_fn_name[size];
		memcpy(eip_fn_name, info.eip_fn_name, size);
		eip_fn_name[size - 1] = 0;

		cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x  %s:%d: %s+%d\n",read_ebp(), eip, argv[0], argv[1], argv[2], argv[3], argv[4], info.eip_file, info.eip_line, eip_fn_name, eip - info.eip_fn_addr);
       		ebp = *((uint32_t*)ebp);
		
	}
	
	return 0;
}


int
mon_showmappings(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 3) {
		cprintf("Usage : showmappings addr_start addr_end\ttakes %d arguments, %d given\n", 2, argc - 1);
		return 1;
	}
	
	if ( argv[1][0] == '0' && (argv[1][1] == 'x' || argv[1][1] == 'X')) argv[1] += 2;
	if ( argv[2][0] == '0' && (argv[2][1] == 'x' || argv[2][1] == 'X')) argv[2] += 2;

	int error = 0;
	uintptr_t addr_start = atoi(argv[1], 16, &error);
	uintptr_t addr_end = atoi(argv[2], 16, &error);
	
	addr_start = addr_start > addr_end ? ROUNDUP(addr_start, PGSIZE) : ROUNDDOWN(addr_start, PGSIZE);
	addr_end = addr_start > addr_end ? ROUNDDOWN(addr_end, PGSIZE) : ROUNDUP(addr_end, PGSIZE);

	if (error) {
		cprintf("Convert address to integer failed, check your parameter\n");
		return 1;
	}
	
	cprintf("\n  == show mappings from %08x to %08x ==\n\n", addr_start, addr_end);
	cprintf("virtual addr\tpysical addr\tpremissions\n");

	if (addr_start < addr_end)
		for (; addr_start <= addr_end; addr_start += PGSIZE) 
			showmapping(addr_start);
	else
		for (; addr_start >= addr_end; addr_start -= PGSIZE) 
			showmapping(addr_start);
	cprintf("\n");
	return 0;
}

int
mon_setperm(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 4) {
		cprintf("Usage : setperm vaddr [0|1] [P|W|U]\ttakes %d arguments, %d given\n", 3, argc - 1);
		return 0;
	}

	int error = 0;
	if ( strchr(argv[1], 'x') || strchr(argv[1], 'X')) argv[1] += 2;
	uintptr_t addr = atoi(argv[1], 16, &error);	
	if (error) {
		cprintf("Convert address to integer failed, check your parameter\n");
		return 0;
	}
	
	pte_t *pte = pgdir_walk(kern_pgdir, (void *)ROUNDDOWN(atoi(argv[1], 16, 0), PGSIZE), false);
	if (!pte) {
		cprintf("Mapping is not available\n");
	} else {
		cprintf("\n  ==  old page mapping info:  ==\n\nvirtual addr\tphysical addr\tpremissions\n");
		showmapping(addr);

		uint32_t perm;
		switch(argv[3][0]) {
			case 'P':	perm = PTE_P;	break;
			case 'W':	perm = PTE_W;	break;
			case 'U':	perm = PTE_U;	break;
			default:
				cprintf("parameter [P|U|W] get %c", argv[3][0]);
				return 0;
		}
		switch(argv[2][0]) {
			case '0':	*pte = *pte & ~perm; 	break;
			case '1':	*pte = *pte | perm;		break;
			default:
				cprintf("parameter [0|1] get %c", argv[2][0]);
				return 1;		
		}
		cprintf("\n  ==  new page mapping info:  ==\n\nvirtual addr\tphysical addr\tpremissions\n");
		showmapping(addr);
		cprintf("\n");
	}

	return 0;
}

int
mon_dump(int argc, char **argv, struct Trapframe *tf) {
	if (argc < 4) {
		cprintf("Usage : dump [-p|v] addr_start addr_end\ttakes %d arguments, %d given\n", 3, argc - 1);
		return 1;
	}
	
	if ( argv[2][0] == '0' && (argv[2][1] == 'x' || argv[2][1] == 'X')) argv[2] += 2;
	if ( argv[3][0] == '0' && (argv[3][1] == 'x' || argv[3][1] == 'X')) argv[3] += 2;

	int error = 0;
	uintptr_t addr_start = atoi(argv[2], 16, &error);
	uintptr_t addr_end = atoi(argv[3], 16, &error);
	
	if (error) {
		cprintf("Convert address to integer failed, check your parameter\n");
		return 1;
	}
	if (argv[1][1] != 'v' && argv[1][1] != 'p') {
		cprintf("parameter -[p|v] get %s\n", argv[1]);
		return 1;
	}
	int is_va = (argv[1][1] == 'v');

	if (addr_start < addr_end)
		for (; addr_start <= addr_end; ++addr_start) 
			dump(addr_start, is_va);
	else
		for (; addr_start >= addr_end; --addr_start) 
			dump(addr_start, is_va);

	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}


void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
