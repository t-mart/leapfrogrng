// Tim,
//
// grep <kernel-src>/fs/proc/generic.c for "How to be a proc read function"
//	 (It explains how to interface proc_read(<args>) works)
// Also see this:
//	 http://stackoverflow.com/questions/9286303/unable-to-understand-working-of-read-proc-in-linux-kernel-module
//
//

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
//   http://code.google.com/p/batterymine/wiki/Understanding_of_Procfs

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
#include <linux/sched.h>     // included for current task ptr
#include <asm/uaccess.h>     // get/put_use, copy_from/to_user

#define LFRNG_LOG_ID "[lfrng] "

#define PROC_F_NAME "lfrng"

//Documentation macros
#define LICENSE "GPL"
#define AUTHORS "Eric Huang <ehuang3@gatech.edu>, Tim Martin <tim.martin@gatech.edu>"
#define DESC "A Leapfrog RNG for Georgia Tech's CS3210, Spring 2013"

#define PROCFS_MAX_SIZE 1024

// Holds information about the /proc file
static struct proc_dir_entry *proc_f;

// Buffer used to store character for this module
static char proc_f_buffer[1024];

// Size of the buffer
static unsigned long proc_f_buffer_size = 0;

static int lfrng_proc_f_read(char *buffer, char **buffer_location, off_t offset, 
int buffer_length, int *eof, void *data)
{
	int ret;
	struct task_struct *curr_task;
	
	printk(KERN_INFO LFRNG_LOG_ID "lfrng_proc_f_read called\n");

	// Get current task_struct
	curr_task = current;
	// print gid and pid
	printk(KERN_INFO LFRNG_LOG_ID "called by gid %u, pid %u\n", curr_task->gid, curr_task->pid);

	// print input arguements
	printk(KERN_INFO LFRNG_LOG_ID "offset = %lu\n", offset);
	printk(KERN_INFO LFRNG_LOG_ID "buffer_length = %lu\n", buffer_length);
	
	if (offset > 0) {
		/* we have finished to read, return 0 */
		ret  = 0;
	} else {
		/* fill the buffer, return the buffer size */
		memcpy(buffer, proc_f_buffer, proc_f_buffer_size);
		ret = proc_f_buffer_size;
	}

	return ret;
}

static int lfrng_proc_f_write(struct file *file, const char *buffer,
unsigned long count, void *data)
{
	/* get buffer size */
	proc_f_buffer_size = count;
	if (proc_f_buffer_size > PROCFS_MAX_SIZE ) {
		proc_f_buffer_size = PROCFS_MAX_SIZE;
	}
	
	/* write data to the buffer */
	if ( copy_from_user(proc_f_buffer, buffer, proc_f_buffer_size) ) {
		return -EFAULT;
	}
	printk(KERN_INFO LFRNG_LOG_ID "lfrng_proc_f_write called\n");
	
	return proc_f_buffer_size;
}

static int __init lfrng_init(void)
{
	printk(KERN_INFO LFRNG_LOG_ID "Initializing...\n");
	/* create the /proc file */
	proc_f = create_proc_entry(PROC_F_NAME, 0666, NULL);

	if (proc_f == NULL) {
		printk(KERN_ALERT LFRNG_LOG_ID "Error: Could not initialize /proc/%s\n", PROC_F_NAME);
		return -ENOMEM;
	}

	proc_f->read_proc  = lfrng_proc_f_read;
	proc_f->write_proc = lfrng_proc_f_write;
	proc_f->owner      = THIS_MODULE;
	proc_f->mode       = S_IFREG | S_IRUGO;
	proc_f->uid        = 0;
	proc_f->gid        = 0;
	proc_f->size     = 37;

	printk(KERN_INFO LFRNG_LOG_ID "/proc/%s created.\n", PROC_F_NAME);
	return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit lfrng_exit(void)
{
	printk(KERN_INFO LFRNG_LOG_ID "Removing proc entry...\n");
	remove_proc_entry(PROC_F_NAME, &proc_root);
	printk(KERN_INFO LFRNG_LOG_ID "Exited.\n");
}

module_init(lfrng_init);
module_exit(lfrng_exit);

MODULE_LICENSE(LICENSE);
MODULE_AUTHOR(AUTHORS);
MODULE_DESCRIPTION(DESC);

// trying to follow the linux conventions
