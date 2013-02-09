// Eric,
//
// the data structure works now. there's a list of thread groups, and each
// thread group contains a list of threads
//
// you can test it by cat /proc/lfrng. or make test (runs thru a fork, openmpi,
// and pthreads that all attempt to read from /proc/lfrng, but as we said, fork
// doesn't matter). then look at /var/log/messages where it prints tgids. tgids
// are outside the parentheses, while pid's are inside.
//
// what isn't working is:
//
// 	thread recognition. right now, threads only get added to the pool when
// 	they read from /proc/lfrng (this decision was completely arbitrary).
// 	seeing the threads available  is important bc threads need to get their
// 	first random number. I'm thinking about using *next_thread(const struct task_struct *p)
// 	http://lxr.linux.no/#linux+v2.6.24.6/include/linux/sched.h#L1743 use of that
// 	function requires that all threads are already initialized, so perhaps the
// 	first step for users is to create all threads, get one to seed, and then
// 	they can leapfrog.
//
// 	writing seeds/reading random numbers. there's no mechanism right now to
// 	look for seeds in write_proc, nor give random numbers to readers in
// 	read_proc.
//
// 	there's no removal of thread/thread_groups that might be dead from the pool.
// 	(not a huge problem, everything's still freed correctly on module exit, but
// 	there might be a lot of cruft given enough time. practically, this code only
// 	has to survive a demo).
//
// 	we need to start writing a man document the describe exactly what the code
// 	can/can't do
//
// 	i've written tests to read from /proc/lfrng as said, but these don't parse
// 	any results (i.e. the random numbers). we need to test that 1) threads are
// 	getting output from lfrng and 2) that the output follows leapfrog
// 	algorithm.
//
// 	haven't tested if the proc recognizes a thread it's already talked to (so it
// 	knows to just give the next number)
//
// do your best. and it'd be really nice if you were available to talk before
// turn-in.

//
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
#include <linux/list.h>      // simple DLL, see http://isis.poly.edu/kulesh/stuff/src/klist/
#include <linux/vmalloc.h>   // mem allocations

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
static char lfrng_buffer[1024];

// Size of the buffer
static unsigned long lfrng_buffer_size = 0;

struct lfrng_thread_group;

struct lfrng_thread {
	unsigned int id;               // the pid of the thread
	int next_rand;
	struct lfrng_thread_group *tg; // the thread group this thread belongs to
	struct list_head list;         // this is a list of other threads in a single thread group
};

struct lfrng_thread_group {
	unsigned int id;               // the tgid of this group
	u32 seed;
	int n_threads;                 // not defined yet, but it should hold the number of threads in this group?
	struct lfrng_thread *head;
	struct list_head list;
};

static struct lfrng_thread_group *seen_tg_list;

///////////////////////////////////////////////////////////////////////////////
// LEAPFROG RNG
///////////////////////////////////////////////////////////////////////////////
// LeapFrog RNG constants
static const u64 MULTIPLIER = 764261123;
static const u64 PMOD       = 2147483647;
static const u64 INCREMENT  = 0;

// Seeds the input lfrng_thread with f|n (the nth random number in the sequence).
static void lfrng_seed(struct lfrng_thread *thread, int seed, int n)
{
	u64 last = seed;
	u64 curr;

	int i;
	for(i=0; i < n; i++) {
		curr = (MULTIPLIER*last + INCREMENT) % PMOD;
		last = curr;
	}
	// handle the case n == 0
	thread->next_rand = last;
}

// Returns the next leapfrog random number for the input lfrng_thread.
// Also updates the thread's random number from f|i to f|(i + #num threads).
//
// Assumes we have a valid, seeded thread.
static int lfrng_rand(struct lfrng_thread *thread,
                      struct lfrng_thread_group *group)
{
	u64 last = thread->next_rand;
	u64 curr;

	int i;
	for(i=0; i < group->n_threads; i++) {
		curr = (MULTIPLIER*last + INCREMENT) % PMOD;
		last = curr;
	}

	last = thread->next_rand;
	thread->next_rand = curr;

	return last;
}

///////////////////////////////////////////////////////////////////////////////
// LEAPFROG THREADS
///////////////////////////////////////////////////////////////////////////////

// Returns the lfrng_thread_group associated with the task (matches tgid) or
// null.
static struct lfrng_thread_group *get_lfrng_group(struct task_struct *task)
{
	u32 tgid = task->tgid;
	
	struct list_head *i = 0;
	struct lfrng_thread_group *group = 0;

	list_for_each(i, &(seen_tg_list->list)) {
		group = list_entry(i, struct lfrng_thread_group, list);
		if(tgid == group->id) {
			break;
		}
		group = 0; // Reset to indicate not found
	}

	return group;
}

// Returns lfrng_thread associated with the task (matches pid & tgid) or null.
static struct lfrng_thread *get_lfrng_thread(struct task_struct *task)
{
	u32 pid = task->pid;

	struct lfrng_thread_group *group = get_lfrng_group(task);
	struct list_head *i = 0;
	struct lfrng_thread *thread = 0;

	if(!group) {
		return 0;
	}

	list_for_each(i, &(group->head->list)) {
		thread = list_entry(i, struct lfrng_thread, list);
		if(pid == thread->id) {
			break;
		}
		thread = 0; // Reset to indicate not found
	}

	return thread;
}

/*static void add_thread_group(struct task_struct *current_task)*/
/*{*/
	/*struct lfrng_thread_group *new_tg = vmalloc(sizeof(struct lfrng_thread_group));*/
	/*struct lfrng_thread *new_t = vmalloc(sizeof(struct lfrng_thread));*/

	/*//init the new thread group*/
	/*new_tg->id = current_task->tgid;*/
	/*INIT_LIST_HEAD(&(new_tg->list));*/

	/*list_add_tail(&(new_tg->list), &(seen_tg_list->list));*/

	/*//init the new thread*/
	/*new_t->id = current_task->tid;*/
	/*INIT_LIST_HEAD(&(new_t->list));*/

	/*list_add_tail(&(new_t->list), &(seen_tg_list->list));*/
/*}*/

static unsigned int add_thread(struct task_struct *current_task)
{
	char found_tg = 0, found_t = 0;
	unsigned int pid = current_task->pid;
	unsigned int tgid = current_task->tgid;
	struct list_head *i;
	struct lfrng_thread_group *tg_iter;
	struct lfrng_thread *t_iter;

	//first see if thread group exists
	list_for_each(i, &(seen_tg_list->list)){
		tg_iter = list_entry(i, struct lfrng_thread_group, list);
		if (tg_iter->id == tgid){
			found_tg = 1;
			break;
		}
	}

	//make one if it doesn't exist
	if (!found_tg){
		tg_iter = vmalloc(sizeof(struct lfrng_thread_group));

		tg_iter->id = tgid;
		INIT_LIST_HEAD(&(tg_iter->list));
		list_add_tail(&(tg_iter->list), &(seen_tg_list->list));

		tg_iter->head = vmalloc(sizeof(struct lfrng_thread));
		INIT_LIST_HEAD(&(tg_iter->head->list));

		//this necessarily means we don't have a thread yet either.
		//so make it now
		t_iter = vmalloc(sizeof(struct lfrng_thread));

		t_iter->id = pid;
		INIT_LIST_HEAD(&(t_iter->list));
		list_add_tail(&(t_iter->list), &(tg_iter->head->list));

		t_iter->tg = tg_iter;
	} else {
		//first see if thread exists
		list_for_each(i, &(tg_iter->list)){
			t_iter = list_entry(i, struct lfrng_thread, list);
			if (t_iter->id == pid){
				found_t = 1;
				break;
			}
		}

		if (!found_t){
			t_iter = vmalloc(sizeof(struct lfrng_thread));

			t_iter->id = pid;
			INIT_LIST_HEAD(&(t_iter->list));
			list_add_tail(&(t_iter->list), &(tg_iter->head->list));

			t_iter->tg = tg_iter;
		}
	}

	//might be a good idea to return something useful here like the tid,tgid or
	//whether something was added or not
	return 0;
}

static void print_thread_groups(void)
{
	struct list_head *i, *j;
	struct lfrng_thread_group *tg_iter;
	struct lfrng_thread *t_iter;

	printk(KERN_INFO LFRNG_LOG_ID "Printing tgids:\n");

	list_for_each(i, &(seen_tg_list->list)){
		tg_iter = list_entry(i, struct lfrng_thread_group, list);
		printk("\t%d ( ", tg_iter->id);
		list_for_each(j, &(tg_iter->head->list)){
			t_iter = list_entry(j, struct lfrng_thread, list);
			printk("%d ", t_iter->id);
		}
		printk(")\n");
	}
}

static int del_thread_group(int id)
{
	struct list_head *i, *tmp;
	struct lfrng_thread_group *ele;

	list_for_each_safe(i, tmp, &(seen_tg_list->list)){
		ele = list_entry(i, struct lfrng_thread_group, list);
		if (ele->id == id){
			list_del(&(ele->list));
			vfree(ele);
			return id;
		}
	}

	return 0;
}

static int del_all_thread_groups(void)
{
	struct list_head *i, *j, *tmp_i, *tmp_j;
	struct lfrng_thread_group *tg_iter;
	struct lfrng_thread *t_iter;

	int n = 0;

	list_for_each_safe(i, tmp_i, &(seen_tg_list->list)){
		n++;
		tg_iter = list_entry(i, struct lfrng_thread_group, list);

		list_for_each_safe(j, tmp_j, &(tg_iter->head->list)){
			t_iter = list_entry(j, struct lfrng_thread, list);
			list_del(&(t_iter->list));
			vfree(t_iter);
		}

		vfree(&(tg_iter->head->list));

		list_del(&(tg_iter->list));
		vfree(tg_iter);
	}

	return n;
}

// 
static int lfrng_read(char *buffer, char **start, off_t offset,
							 int count, int *peof, void *dat)
{
	struct task_struct *curr_task;
	int rand;
	int len;

	printk(KERN_INFO LFRNG_LOG_ID "lfrng_read called\n");

	// Get current task_struct
	curr_task = current;
	// print tgid and pid
	printk(KERN_INFO LFRNG_LOG_ID "called by tgid %u, pid %u\n", curr_task->tgid, curr_task->pid);

	add_thread(curr_task);
	print_thread_groups();

	// Adapted from method 2) in <fs/proc/generic.c>, "How to be a proc read
	// function".
	if(offset == 0) {
		// New call to lfrng_read
		// TODO: call lfrng_rand() into rand
		lfrng_buffer_size = sprintf(lfrng_buffer, "%d", rand);
	}

	*start = buffer;
	// Number of bytes to copy over
	len = min(lfrng_buffer_size - offset, count);
	if(len < 0) len = 0;

	memcpy(*start, lfrng_buffer+offset, len);

	if(offset + len == lfrng_buffer_size) {
		*peof = 1;
	}

	return len;
}

static int lfrng_write(struct file *file, const char *buffer,
							  unsigned long count, void *data)
{
	/* get buffer size */
	lfrng_buffer_size = count;
	if (lfrng_buffer_size > PROCFS_MAX_SIZE ) {
		lfrng_buffer_size = PROCFS_MAX_SIZE;
	}

	/* write data to the buffer */
	if ( copy_from_user(lfrng_buffer, buffer, lfrng_buffer_size) ) {
		return -EFAULT;
	}
	printk(KERN_INFO LFRNG_LOG_ID "lfrng_proc_f_write called\n");

	return lfrng_buffer_size;
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

	proc_f->read_proc  = lfrng_read;
	proc_f->write_proc = lfrng_write;
	proc_f->owner      = THIS_MODULE;
	proc_f->mode       = S_IFREG | S_IRUGO | S_IWUGO;
	proc_f->uid        = 0;
	proc_f->gid        = 0;
	proc_f->size       = 1024;

	seen_tg_list = vmalloc(sizeof(struct lfrng_thread_group));
	/*seen_tg_list->id=-1;*/
	INIT_LIST_HEAD(&(seen_tg_list->list));

	printk(KERN_INFO LFRNG_LOG_ID "/proc/%s created.\n", PROC_F_NAME);
	return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit lfrng_exit(void)
{
	del_all_thread_groups();
	vfree(seen_tg_list);
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
// vim:tw=80:ts=4:sw=4:noexpandtab
