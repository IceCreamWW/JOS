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
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line



struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

int mon_printf(int argc, char **argv, struct Trapframe* tf){
	int x = 1, y = 3, z = 4;
	cprintf("x %d, y %x",3);
	return 0;
}

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "print", "Test cprintf function", mon_printf},
	{ "backtrace", "Get backtrace display for exercise 11", mon_backtrace}
};

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



static inline uint32_t
read_argc(void)
{
	uint32_t argc;
	asm volatile("movl 8(%%ebp),%0" : "=r" (argc));
	return argc;
}


static inline uint32_t
read_args(int which){
	uint32_t argsp;
	uint32_t arg;
	
	asm volatile("movl 12(%%ebp),%0" : "=r" (argsp));		
	asm volatile("movl %1,%0" : "=r" (arg) : "r" (argsp + which * 4));
	return arg;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp();

	while (*(uint32_t *)ebp) {
		struct Eipdebuginfo info;
		uint32_t eip = *((uint32_t *)ebp + 1);
		uint32_t argv[5];
		int i;
		for (i=1; i<=5; ++i) {
			argv[i - 1] = *((uint32_t *)ebp + 1 + i);
		}
		debuginfo_eip(eip, &info);

		/* Format funciton name */
		char *index = strchr(info.eip_fn_name, ':');
		uint32_t size = index - info.eip_fn_name + 1;
		char eip_fn_name[size];
		memcpy(eip_fn_name, info.eip_fn_name, size);
		eip_fn_name[size - 1] = 0;

		cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x  %s:%d: %s+%d\n",read_ebp(), eip, argv[0], argv[1], argv[2], argv[3], argv[4], info.eip_file, info.eip_line, eip_fn_name, eip - info.eip_fn_addr);
       		ebp = *((uint32_t *)ebp);
		
	}
	
//	cprintf("ebp %x  eip %x  args %x %s %s %s %s\n",read_ebp(), read_eip(), argc, argv[0], argv[1], argv[2], argv[3]);
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

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
