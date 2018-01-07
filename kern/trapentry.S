/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

	TRAPHANDLER(handler_8, 8);
	TRAPHANDLER(handler_10, 10);
	TRAPHANDLER(handler_11, 11);
	TRAPHANDLER(handler_12, 12);
	TRAPHANDLER(handler_13, 13);
	TRAPHANDLER(handler_14, 14);

	TRAPHANDLER_NOEC(handler_0, T_DIVIDE);
	TRAPHANDLER_NOEC(handler_1, T_DEBUG);
	TRAPHANDLER_NOEC(handler_2, 2);
	TRAPHANDLER_NOEC(handler_3, 3);
	TRAPHANDLER_NOEC(handler_4, 4);
	TRAPHANDLER_NOEC(handler_5, 5);
	TRAPHANDLER_NOEC(handler_6, 6);
	TRAPHANDLER_NOEC(handler_7, 7);
	TRAPHANDLER_NOEC(handler_9, 9);
	TRAPHANDLER_NOEC(handler_15, 15);
	TRAPHANDLER_NOEC(handler_16, 16);
	TRAPHANDLER_NOEC(handler_17, 17);
	TRAPHANDLER_NOEC(handler_18, 18);
	TRAPHANDLER_NOEC(handler_19, 19);
	TRAPHANDLER_NOEC(handler_20, 20);
	TRAPHANDLER_NOEC(handler_21, 21);
	TRAPHANDLER_NOEC(handler_22, 22);
	TRAPHANDLER_NOEC(handler_23, 23);
	TRAPHANDLER_NOEC(handler_24, 24);
	TRAPHANDLER_NOEC(handler_25, 25);
	TRAPHANDLER_NOEC(handler_26, 26);
	TRAPHANDLER_NOEC(handler_27, 27);
	TRAPHANDLER_NOEC(handler_28, 28);
	TRAPHANDLER_NOEC(handler_29, 29);
	TRAPHANDLER_NOEC(handler_30, 30);
	TRAPHANDLER_NOEC(handler_31, 31);
	
	TRAPHANDLER_NOEC(handler_32, 32);
	TRAPHANDLER_NOEC(handler_33, 33);
	TRAPHANDLER_NOEC(handler_34, 34);
	TRAPHANDLER_NOEC(handler_35, 35);
	TRAPHANDLER_NOEC(handler_36, 36);
	TRAPHANDLER_NOEC(handler_37, 37);
	TRAPHANDLER_NOEC(handler_38, 38);
	TRAPHANDLER_NOEC(handler_39, 39);
	TRAPHANDLER_NOEC(handler_40, 40);
	TRAPHANDLER_NOEC(handler_41, 41);
	TRAPHANDLER_NOEC(handler_42, 42);
	TRAPHANDLER_NOEC(handler_43, 43);
	TRAPHANDLER_NOEC(handler_44, 44);
	TRAPHANDLER_NOEC(handler_45, 45);
	TRAPHANDLER_NOEC(handler_46, 46);
	TRAPHANDLER_NOEC(handler_47, 47);
	

	TRAPHANDLER_NOEC(handler_48, 48);


/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	
	pushl %ds;
	pushl %es;
	pushal;
	pushl $GD_KD;
	popl %ds;
	pushl $GD_KD;
	popl %es;
	pushl %esp;
	call trap;
