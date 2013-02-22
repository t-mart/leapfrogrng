//if rsyslogd stops working run:
//sudo cp rsyslog.conf /etc/rsyslog.conf && sudo /etc/init.d/rsyslog restart
//
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

struct lfrng_thread_group;

struct lfrng_thread {
	unsigned int id;               // the pid of the thread
	long long unsigned int next_rand;
	struct lfrng_thread_group *tg; // the thread group this thread belongs to
	struct list_head list;         // this is a list of other threads in a single thread group
};

struct lfrng_thread_group {
	unsigned int id;               // the tgid of this group
	u64 seed;
	int n_threads;                 // _the user declares_ this number at seed. there might not be that many at head
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

static u64 power_mod(u64 base, int exp, u64 mod, u64 inc, u64 mult)
{
	int i;
	for(i=0; i < exp; i++) {
		base = (mult*base + inc) % mod;
	}

	return base;
}

static int count_group_threads(struct lfrng_thread_group *group)
{
	struct list_head *i;
	int c = 0;

	list_for_each(i, &(group->head->list)){
		c++;
	}

	return c;
}

#define LFRNG_POWER_MOD(base,exp) power_mod((base),(exp),PMOD,INCREMENT,MULTIPLIER)

#define FIRST_RAND(thread_ptr) LFRNG_POWER_MOD(((thread_ptr)->tg->seed),(count_group_threads((thread_ptr)->tg)+1))
#define SUBSEQ_RAND(thread_ptr) LFRNG_POWER_MOD(((thread_ptr)->next_rand),(u64)((thread_ptr)->tg->n_threads))

// Seeds the input lfrng_thread with f|n (the nth random number in the sequence).
static u64 lfrng_seed_thread(struct lfrng_thread *thread)
{
	return (thread->next_rand = FIRST_RAND(thread));
}

// Returns the next leapfrog random number for the input lfrng_thread.
// Also updates the thread's random number from f|i to f|(i + #num threads).
//
// Assumes we have a valid, seeded thread.
static u64 lfrng_leapfrog_thread(struct lfrng_thread *thread)
{
	return (thread->next_rand = SUBSEQ_RAND(thread));
}


///////////////////////////////////////////////////////////////////////////////
// LEAPFROG THREADS
///////////////////////////////////////////////////////////////////////////////

// Returns the lfrng_thread_group associated with the task (matches tgid) or
// null.
static struct lfrng_thread_group *get_lfrng_group(int tgid)
{
	struct list_head *i = 0;
	struct lfrng_thread_group *group = 0;

	list_for_each(i, &(seen_tg_list->list)) {
		group = list_entry(i, struct lfrng_thread_group, list);
		if(tgid == group->id) {
			return group;
		}
	}
	return NULL;
}

// Returns lfrng_thread associated with the task (matches pid & tgid) or null.
static struct lfrng_thread *get_lfrng_thread(int pid, int tgid)
{
	struct lfrng_thread_group *group = get_lfrng_group(tgid);
	struct list_head *i = 0;
	struct lfrng_thread *thread = 0;

	if(group == NULL) {
		return NULL;
	}

	list_for_each(i, &(group->head->list)) {
		thread = list_entry(i, struct lfrng_thread, list);
		if(pid == thread->id) {
			return thread;
		}
	}

	return NULL;
}

//assumes that you've already searched for it with get_lfrng_thread!!!!
static struct lfrng_thread *
attach_new_thread_to_group(struct lfrng_thread_group *group, int thread_id)
{
	struct lfrng_thread *thread = vmalloc(sizeof(struct lfrng_thread));

	//(group->n_threads - #elements(group->head)
	//the difference between the users delcaration and the actual number of
	//threads the group has
	/*int thread_diff;*/

	thread->id = thread_id;
	INIT_LIST_HEAD(&(thread->list));
	list_add_tail(&(thread->list), &(group->head->list));
	
	lfrng_seed_thread(thread);

	thread->tg = group;;

	return thread;
}

//returns a pointer to the lfrng_thread added, or if it already exists, a
//pointer to that
static struct lfrng_thread * add_thread(unsigned int pid, unsigned int tgid)
{
	struct lfrng_thread_group *group = get_lfrng_group(tgid);
	struct lfrng_thread *thread;

	if (group == NULL){
		//your thread group never seeded
		//bye bye!
		return NULL;
	} else {
		//first see if thread exists
		thread = get_lfrng_thread(pid, tgid);

		if (thread == NULL){
			return attach_new_thread_to_group(group, pid);
		}

		return thread;
	}
}

static struct lfrng_thread_group * create_thread_group(int tgid)
{
	struct lfrng_thread_group *group = get_lfrng_group(tgid);

	if (group == NULL){
		//make one
		group = vmalloc(sizeof(struct lfrng_thread_group));

		group->id = tgid;
		INIT_LIST_HEAD(&(group->list));
		list_add_tail(&(group->list), &(seen_tg_list->list));

		group->head = vmalloc(sizeof(struct lfrng_thread));
		INIT_LIST_HEAD(&(group->head->list));
	}

	return group;
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

static long long unsigned int get_next_rand(int pid, int tgid)
{
	int rand, threads_in_group;
	struct lfrng_thread *thread;
	struct lfrng_thread_group *group;

	group = get_lfrng_group(tgid);
	if (group==NULL){
		/*printk(KERN_INFO LFRNG_LOG_ID "pid %u called without a seed from his tgid (%u)!\n", pid, tgid);*/
		rand = -1;
	} else {
		thread = get_lfrng_thread(pid, tgid);
		threads_in_group = count_group_threads(group);
		if (thread==NULL){
			//thread doesn't exist...make it
			if (threads_in_group < group->n_threads){
				//group has room
				thread = attach_new_thread_to_group(group, pid);
				/*lfrng_seed_thread(thread);*/
				rand = thread->next_rand;
			} else {
				//no room
				rand = -1;
			}
		} else {
			lfrng_leapfrog_thread(thread);
			rand = thread->next_rand;
		}
	}
	printk(KERN_INFO LFRNG_LOG_ID "next rand for pid=%u,tgid=%u is %d\n", pid, tgid, rand);
	return rand;

}

static int lfrng_read(char *buffer, char **start, off_t offset,
							 int count, int *peof, void *dat)
{
	int len = 0;

	//if we're entering this function again
	if (offset > 0) {
		*peof = 1;
		return 0;
	}

	/*printk( KERN_INFO LFRNG_LOG_ID "next rand would be %llu", get_next_rand(current->pid, current->tgid));*/
	len = sprintf(buffer,"%llu\n", get_next_rand(current->pid, current->tgid));
	/*len = sprintf(buffer,"%i %i\n", current->pid, current->tgid);*/

	return len;
}

static int lfrng_write(struct file *file, const char *buffer,
							  unsigned long count, void *data)
{
	int n_threads;
	unsigned long long seed;
	unsigned long b_size = 1024;
	char our_buffer[b_size];

	unsigned long len = min(count,b_size);

	struct lfrng_thread_group * group;

	printk(KERN_INFO LFRNG_LOG_ID "lfrng_proc_f_write called\n");
	/* write data to the buffer */
	if ( copy_from_user(our_buffer, buffer, len) ) {
		printk(KERN_INFO LFRNG_LOG_ID "some error took place while reading the write buffer\n");
		return -EFAULT;
	}

	our_buffer[len - 1] = '\0';

	sscanf(our_buffer, "%llu %i", &seed, &n_threads);
	printk(KERN_INFO LFRNG_LOG_ID "tgid=%u,pid=%u parsed: seed:%llu, n_threads:%d\n", current->tgid, current->pid, seed, n_threads);

	group = create_thread_group(current->tgid);
	group->seed = seed;
	group->n_threads = n_threads;

	print_thread_groups();

	return len;
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
