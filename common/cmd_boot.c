/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>

#if defined(CONFIG_I386)
DECLARE_GLOBAL_DATA_PTR;
#endif


void  call_linux(long a0, long a1, long a2)
{
__asm__(
	"mov	r0, %0\n"
	"mov	r1, %1\n"
	"mov	r2, %2\n"
	"mov	ip, #0\n"
	"mcr	p15, 0, ip, c13, c0, 0\n"	/* zero PID */
	"mcr	p15, 0, ip, c7, c7, 0\n"	/* invalidate I,D caches */
	"mcr	p15, 0, ip, c7, c10, 4\n"	/* drain write buffer */
	"mcr	p15, 0, ip, c8, c7, 0\n"	/* invalidate I,D TLBs */
	"mrc	p15, 0, ip, c1, c0, 0\n"	/* get control register */
	"bic	ip, ip, #0x0001\n"		/* disable MMU */
	"mcr	p15, 0, ip, c1, c0, 0\n"	/* write control register */
	"mov	pc, r2\n"
	"nop\n"
	"nop\n"
	: /* no outpus */
	: "r" (a0), "r" (a1), "r" (a2)
	: "r0","r1","r2","ip"
	);
}

#define LINUX_KERNEL_OFFSET			0x8000
#define LINUX_PARAM_OFFSET			0x100
#define LINUX_PAGE_SIZE				0x00001000
#define LINUX_PAGE_SHIFT			12
#define LINUX_ZIMAGE_MAGIC			0x016f2818
#define DRAM_SIZE				0x04000000


/*
 * pram_base: base address of linux paramter
 */
static void setup_linux_param(ulong param_base)
{
	struct param_struct *params = (struct param_struct *)param_base; 
	char *linux_cmd;

//	printf("Setup linux parameters at 0x%08lx\n", param_base);
	memset(params, 0, sizeof(struct param_struct));

	params->u1.s.page_size = LINUX_PAGE_SIZE;
	params->u1.s.nr_pages = (DRAM_SIZE >> LINUX_PAGE_SHIFT);

	/* set linux command line */
	linux_cmd = getenv ("bootargs");
	if (linux_cmd == NULL) {
		printf("Wrong magic: could not found linux command line\n");
	} else {
		memcpy(params->commandline, linux_cmd, strlen(linux_cmd) + 1);
//		printf("linux command line is: \"%s\"\n", linux_cmd);
	}
}



int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, rc;
	int     rcode = 0;
	ulong   to;

	to = 0x30000000 + LINUX_KERNEL_OFFSET;
	
	if (*(ulong *)(to + 9*4) != LINUX_ZIMAGE_MAGIC) {
		printf("Warning: this binary is not compressed linux kernel image\n");
		printf("zImage magic = 0x%08lx\n", *(ulong *)(to + 9*4));
	} else {
//		printf("zImage magic = 0x%08lx\n", *(ulong *)(to + 9*4));
		;
	}

	/* Go Go Go */
	printf("NOW, Booting Linux......\n");	
	
	/* Setup linux parameters and linux command line */
	setup_linux_param(0x30000000 + LINUX_PARAM_OFFSET);
	call_linux(0, MACH_TYPE_S3C2440, to);
	

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
#if defined(CONFIG_I386)
	/*
	 * x86 does not use a dedicated register to pass the pointer
	 * to the global_data
	 */
	argv[0] = (char *)gd;
#endif
#if !defined(CONFIG_NIOS)
	rc = ((ulong (*)(int, char *[]))addr) (--argc, &argv[1]);
#else
	/*
	 * Nios function pointers are address >> 1
	 */
	rc = ((ulong (*)(int, char *[]))(addr>>1)) (--argc, &argv[1]);
#endif
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);

	
	/* Setup linux parameters and linux command line */
	setup_linux_param(0x30000000 + LINUX_PARAM_OFFSET);

	/* Go Go Go */
	printf("NOW, Booting Linux......\n");	
	call_linux(0, MACH_TYPE_S3C2440, 0x30008000);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CFG_MAXARGS, 1,	do_go,
	"go      - start application at address 'addr'\n",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments\n"
);

extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

U_BOOT_CMD(
	reset, CFG_MAXARGS, 1,	do_reset,
	"reset   - Perform RESET of the CPU\n",
	NULL
);
