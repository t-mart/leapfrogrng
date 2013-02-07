// Eric,
//
// To test, you might find yourself needing to actually get on the factor. So
// I put together some steps and documented around the project where I could:
//   follow instructions in ssh_config_entry
//   make tunnel (in the root Makefile, not src's)
//   <your_vnc_viewer> localhost:4 (i use tightvnc from my distro's repos)
//
// If you want to get a shell without ssh (easier imo),
//   (make tunnel if not already done)
//   ssh <username>@localhost:60022
//
// To get this code to the system, I made a git repo here which pulls from
// github.com, but that might by a little tricky (SSH keys). Use SCP for a quick
// and dirty solution.
//
// Use make load or make unload respectively. I put targets in both Makefile and
// src/Makefile. Gotta be root.
//
// To see the kernel messages,
//   sudo tail -f /var/log/messages
// (or run without the -f flag to just print the last few lines)
//
// This pages seem like it might come in handy:
//   http://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html
//   newdata.box.sk/raven/lkm.html (search for proc_fs)
//
// See you in class.
//
// -Tim

// Some snippets taken from:
//   http://blog.markloiseau.com/2012/04/hello-world-loadable-kernel-module-tutorial/
//   http://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html

// Defining __KERNEL__ and MODULE allows us to access kernel-level code not
// usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__

#undef MODULE
#define MODULE

// Linux Kernel/LKM headers: module.h is needed by all modules and kernel.h is
// needed for KERN_INFO.
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/proc_fs.h>   // give us access to proc_fs

#define PROC_F_NAME "lfrng"

//Documentation macros
#define LICENSE "GPL"
#define AUTHORS "Eric Huang <ehuang3@gatech.edu>, Tim Martin <tim.martin@gatech.edu>"
#define DESC "A Leapfrog RNG for Georgia Tech's CS3210, Spring 2013"

//proc file struct
static struct proc_dir_entry *proc_f;

static int __init lfrng_init(void)
{
	printk(KERN_INFO "Initializing lfrng...");
	/* create the /proc file */
	proc_f = create_proc_entry(PROC_F_NAME, 0644, NULL);

	if (proc_f == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROC_F_NAME);
		return -ENOMEM;
	}

	proc_f->owner = THIS_MODULE;
	proc_f->mode  = S_IFREG | S_IRUGO;
	proc_f->uid   = 0;
	proc_f->gid   = 0;
	//proc_f->size  = 37;

	printk(KERN_INFO "/proc/%s created\n", PROC_F_NAME);
	return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit lfrng_exit(void)
{
	printk(KERN_INFO "Exiting lfrng...");
	remove_proc_entry(PROC_F_NAME, &proc_root);
	printk(KERN_INFO "done.\n");
}

module_init(lfrng_init);
module_exit(lfrng_exit);

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHORS);
MODULE_DESCRIPTION(DESC);

// trying to follow the linux conventions
// vim:tw=80:ts=8:sw=8:noexpandtab:fdm=syntax
