// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/dwarf.h>
#include <kern/kdebug.h>
#include <kern/dwarf_api.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
extern short int cga_mode_gv;

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
        { "change_color", "Change text attributes ", changecolor }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
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

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
        uint64_t rbp_base;
        rbp_base =  read_rbp();
        uint64_t * rbp_ptr;
        uint64_t rip_value ;
        read_rip(rip_value);
        int i ;
        struct Ripdebuginfo rip_info;
        while (rbp_base != 0x0)
        {

          rbp_ptr = (uint64_t *) rbp_base;
          debuginfo_rip((uintptr_t)rip_value,&rip_info);

          cprintf("\n  rbp %016x rip %016x ",(rbp_ptr),rip_value);
          cprintf("\n    %s:%d: ",rip_info.rip_file,rip_info.rip_line);
          cprintf("%.*s",rip_info.rip_fn_namelen,rip_info.rip_fn_name);
          cprintf("+%016x ",(rip_value - rip_info.rip_fn_addr));
          cprintf("args:%d ",rip_info.rip_fn_narg);
          for(i =1;i <= rip_info.rip_fn_narg;i++)
          cprintf("%016x",*((int *)rbp_ptr-i));
          rip_value = *(rbp_ptr+1);
          rbp_base = *rbp_ptr;
        }

        return 0;
}

int
changecolor(int argc, char **argv, struct Trapframe *tf)
{
 cprintf("Fore Color  %c\n",*argv[1]);

  cprintf("Back Color  %c\n",*argv[2]);

  cprintf("Cursor Mode %c\n",*argv[3]);

  cga_mode_gv=0;

  

  switch(*argv[1])

	{

		case 'r' :		

		       cga_mode_gv|=0x400;	// red

		       break;	

		case 'g' :			//green

		       cga_mode_gv|=0x200;	

		       break;	

		case 'b' :			//blue

		       cga_mode_gv|=0x100;	

		       break;	

		case 'w' :			

		       cga_mode_gv|=0x700;	//white

		       break;	

		case 'y' :			

		       cga_mode_gv|=0x600;	//yellow

		       break;	

		default:

		       cga_mode_gv|=0x000;	//black

		       break;

	}



  switch(*argv[2])

	{

		case 'r' :			

		       cga_mode_gv|=0x4000;	//red	

		       break;	

		case 'g' :			

		       cga_mode_gv|=0x2000;	//green

		       break;	

		case 'b' :			

		       cga_mode_gv|=0x1000;	//blue

		       break;	

		case 'y' :			

		       cga_mode_gv|=0x6000;	//yellow

		       break;	

		case 'w' :			//white

		       cga_mode_gv|=0x7000;	

		       break;	

		default :

		       cga_mode_gv|=0x0000;   //black

		       break;	

	}

  switch(*argv[3])

	{

		case '0' :			

		       cga_mode_gv|=0x0000;		

		       cprintf("Alive1\n");

		       break;	

		case '1':			

		       cga_mode_gv|=0x8000;		

		       cprintf("Alive2\n");

		       break;	

		default :

		       cga_mode_gv|=0x0000;  

		       break;	

	}	



  return 0;	

};	


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
	for (i = 0; i < NCOMMANDS; i++) {
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
