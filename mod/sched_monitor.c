#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>

#include <asm/uaccess.h>


/* DO NOT MODIFY THIS CHECK. If you see the message in the error line below
 * when compliling your module, that means your kernel has not been configured
 * correctly. Complete exercises 1-3 of the lab before continuing
 */
#ifdef CONFIG_PREEMPT_NOTIFIERS
#include <linux/preempt.h>
#else
#error "Your kernel must be built with CONFIG_PREEMPT_NOTIFIERS enabled"
#endif

#include <sched_monitor.h>

#define MAX_PREEMPTS 32768

struct preemption_tracker
{
    /* notifier to register us with the kernel's callback mechanisms */
    struct preempt_notifier notifier;
    bool enabled;
    struct list_head list;
    spinlock_t lock;
};


/* Information tracked on each preemption. */
struct preemption_entry
{
    /* fields */
    int core;
    unsigned long long start;
    unsigned long long end;
    struct list_head list;
    char* preempted_by;    
};


/* Get the current time, in nanoseconds. Should be consistent across cores.
 * You are encouraged to look at the options in:
 *      include/linux/timekeeping.h
 */
static inline unsigned long long
get_current_time(void)
{
    struct timespec* ts;
    unsigned long long ns;
    getnstimeofday(ts);
    ns = (unsigned long long) timespec_to_ns(ts);	
    return ns;
}

/* 
 * Utilities to save/retrieve a tracking structure for a process
 * based on the kernel's file pointer.
 *
 * DO NOT MODIFY THE FOLLOWING 2 FUNCTIONS
 */
static inline void
save_tracker_of_process(struct file               * file,
                        struct preemption_tracker * tracker)
{
    file->private_data = tracker;
}

static inline struct preemption_tracker *
retrieve_tracker_of_process(struct file * file)
{
    return (struct preemption_tracker *)file->private_data;
}

/*
 * Utility to retrieve a tracking structure based on the 
 * preemption_notifier structure.
 *
 * DO NOT MODIFY THIS FUNCTION
 */
static inline struct preemption_tracker *
retrieve_tracker_of_notifier(struct preempt_notifier * notifier)
{
    return container_of(notifier, struct preemption_tracker, notifier);
}

/* 
 * Callbacks for preemption notifications.
 *
 * monitor_sched_in and monitor_sched_out are called when the process that
 * registered a preemption notifier is either scheduled onto or off of a
 * processor.
 *
 * You will use these functions to create a linked_list of 'preemption_entry's
 * With this list, you must be able to represent information such as:
 *      (i)   the amount of time a process executed before being preempted
 *      (ii)  the amount of time a process was scheduled out before being
 *            scheduled back on 
 *      (iii) the cpus the process executes on each time it is scheduled
 *      (iv)  the number of migrations experienced by the process
 *      (v)   the names of the processes that preempt this process
 *
 * The data structure 'preemption_tracker' will be necessary to track some of
 * this information.  The variable 'pn' provides unique state that persists
 * across invocations of monitor_sched_in and monitor_sched_out. Add new fields
 * to it as needed to provide the information above.
 */

static void
monitor_sched_in(struct preempt_notifier * pn,
                 int                       cpu)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_notifier(pn);
    struct preemption_entry* entry;
    unsigned long flags;
	printk(KERN_INFO "sched_in for process %s\n", current->comm);
    /*record information as needed */
    spin_lock_irqsave(&tracker->lock, flags);

	entry = list_last_entry(&tracker->list,struct preemption_entry, list); 
    entry->core = cpu;
    entry->end = get_current_time();
	
	spin_unlock_irqrestore(&tracker->lock, flags);    
	
 }

static void
monitor_sched_out(struct preempt_notifier * pn,
                  struct task_struct      * next)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_notifier(pn);
    struct preemption_entry* entry;
    unsigned long flags;
	printk(KERN_INFO "sched_out for process %s\n", current->comm);
   
	/* record information as needed */
    entry  = kmalloc(sizeof(struct preemption_entry), GFP_ATOMIC);
    if(!entry) {
		printk(KERN_ERR "Failed to kmalloc preempt_entry\n");
		return;
	}
	
	spin_lock_irqsave(&tracker->lock, flags);

	list_add_tail(&entry->list, &tracker->list);    
    entry->start = get_current_time();
    entry->preempted_by = next->comm;
    spin_unlock_irqrestore(&tracker->lock, flags);   
}

static struct preempt_ops
notifier_ops = 
{
    .sched_in  = monitor_sched_in,
    .sched_out = monitor_sched_out
};

/*** END preemption notifier ***/


/*
 * Device I/O callbacks for user<->kernel communication 
 */

/*
 * This function is invoked when a user opens the file /dev/sched_monitor.
 * You must update it to allocate the 'tracker' variable and initialize 
 * any fields that you add to the struct
 */
static int
sched_monitor_open(struct inode * inode,
                   struct file  * file)
{
    struct preemption_tracker * tracker;

    printk(KERN_DEBUG "Process %d (%s) opened " DEV_NAME "\n",
        current->pid, current->comm);



    /* allocate a preemption_tracker for this process, other initialization
     * as needed. The rest of this function will trigger a kernel oops until
     * you properly allocate the variable 'tracker'
     */
	
    /* setup tracker */
    /* initialize preempt notifier object */
	tracker = kmalloc(sizeof(struct preemption_tracker), GFP_KERNEL);
	if(!tracker) {
		printk(KERN_ERR "Failed to malloc tracker\n");
		return -1;
	}    

	/* Save tracker so that we can access it on other file operations from this process */
 	INIT_LIST_HEAD(&tracker->list);
	tracker->enabled = false;
	spin_lock_init(&tracker->lock);

	preempt_notifier_init(&tracker->notifier, &notifier_ops);  
	save_tracker_of_process(file, tracker);


    return 0;

}

/* This function is invoked when a user closes /dev/sched_monitor.
 * You must update is to free the 'tracker' variable allocated in the 
 * sched_monitor_open callback, as well as free any other dynamically 
 * allocated data structures you may have allocated (such as linked lists)
 */
static int
sched_monitor_flush(struct file * file,
                    fl_owner_t    owner)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);

    printk(KERN_DEBUG "Process %d (%s) closed " DEV_NAME "\n",
        current->pid, current->comm);

	unsigned long long time_sched_on;
	unsigned long long time_sched_off;
	char * comm;
	int cpu;
	/*Print the list of preempt_entries and delete them */
	struct preemption_entry *pos, *next;
	list_for_each_entry_safe(pos, next, &tracker->list, list) {
		cpu = pos->core;
		comm = pos->preempted_by;
		time_sched_off = pos->start;
		time_sched_on = pos->end;
		
		printk("Scheduled off from %llu to %llu\n", time_sched_off, time_sched_on);
		if(pos != list_last_entry(&tracker->list, struct preemption_entry, list)) {
			printk("Scheduled on from %llu to %llu on core %d\n", time_sched_on, next->start, cpu);
		}
		printk("Preempted by %s\n", comm);
		kfree(pos);
	}


    /* Unregister notifier */
    if (tracker->enabled) {
       	preempt_notifier_unregister(&tracker->notifier);
        
    }
    kfree(tracker);
    

    return 0;
}

/* 
 * Enable/disable preemption tracking for the process that opened this file.
 * Do so by registering/unregistering preemption notifiers.
 */
static long
sched_monitor_ioctl(struct file * file,
                    unsigned int  cmd,
                    unsigned long arg)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);

    switch (cmd) {
        case ENABLE_TRACKING:
            if (tracker->enabled) {
                printk(KERN_ERR "Tracking already enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            /* register notifier, set enabled to true, and remove the error return */
            preempt_notifier_register(&tracker->notifier);
            tracker->enabled = true;

            printk(KERN_DEBUG "Process %d (%s) enabled preemption tracking via " DEV_NAME ".\n",
                current->pid, current->comm);

    

            break;


        case DISABLE_TRACKING:
            if (!tracker->enabled) {
                printk(KERN_ERR "Tracking not enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            /*unregister notifier, set enabled to true, and remove the error return */
            tracker->enabled = false;
	    preempt_notifier_unregister(&tracker->notifier);
            printk(KERN_DEBUG "Process %d (%s) disabled preemption tracking via " DEV_NAME ".\n",
                current->pid, current->comm);

           

            break;

        default:
            printk(KERN_ERR "No such ioctl (%d) for " DEV_NAME "\n", cmd);
            return -ENOIOCTLCMD;
    }

    return 0;
}

/* User read /dev/sched_monitor
 *
 * In this function, you will copy an entry from the list of preemptions
 * experienced by the process to user-space.
 * NOTE: Only copies one entry at a time
 */
static ssize_t
sched_monitor_read(struct file * file,
                   char __user * buffer,
                   size_t        length,
                   loff_t      * offset)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);
    unsigned long flags;
	struct preemption_entry * entry;
	preemption_info_t info;
	unsigned long long start, end;
	if(length != sizeof(preemption_info_t)) {
		return -EINVAL;
	}

	spin_lock_irqsave(&tracker->lock, flags);
	
	if(list_empty(&tracker->list)) {
		printk(KERN_DEBUG "preempt_entry list is empty!\n");
		spin_unlock_irqrestore(&tracker->lock, flags);
		return 0;
	}
	
	/*Copy information from preempt_entry to buffer */
	entry = list_first_entry(&tracker->list, struct preemption_entry, list);
	start = entry->start;
	end = entry->end;
		
	info.cpu = entry->core;
	memcpy(&info.preempted_by, entry->preempted_by, 16); //16 is max len of comm name
	info.time_off = end - start;

	/* If this is the last entry, it marks the final sched_off  */
	if( unlikely( list_is_last(&entry->list, &tracker->list) )) {
		info.time_on = 0;
	} else {
		info.time_on = list_next_entry(entry, list)->start - end;
	}

	if(copy_to_user(buffer, &info, length)) {
		printk(KERN_ERR "Failed to copy preempt_info to user!\n");
		spin_unlock_irqrestore(&tracker->lock, flags);
		return -EFAULT;
	}

	list_del(&entry->list);
	kfree(entry);

	spin_unlock_irqrestore(&tracker->lock, flags);

    /*  
     * (1) make sure length is valid. It must be an even multiuple of the size of preemption_info_t.
     *     i.e., if the value is the size of the structure times 2, the user is requesting the first
     *     2 entries in the list
     * (2) lock your linked list
     * (3) retrieve the head of the linked list, if it is not empty
     * (4) copy the associated contents to user space
     * (5) remove the head of the linked list and free its dynamically allocated
     *     storage.
     * (6) repeat for each entry
     * (7) unlock the list
     */

    printk(KERN_DEBUG "Process %d (%s) read " DEV_NAME "\n",
        current->pid, current->comm);

    /* Use these functions to lock/unlock our list to prevent concurrent writes
     * by the preempt notifier callouts
     *
     *  spin_lock_irqsave(<pointer to spinlock_t variable>, flags);
     *  spin_unlock_irqrestore(<pointer to spinlock_t variable>, flags);
     *
     *  Before the spinlock can be used, it will have to be initialized via:
     *  spin_lock_init(<pointer to spinlock_t variable>);
     */

    return length;
}

static struct file_operations
dev_ops = 
{
    .owner = THIS_MODULE,
    .open = sched_monitor_open,
    .flush = sched_monitor_flush,
    .unlocked_ioctl = sched_monitor_ioctl,
    .compat_ioctl = sched_monitor_ioctl,
    .read = sched_monitor_read,
};

static struct miscdevice
dev_handle = 
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = SCHED_MONITOR_MODULE_NAME,
    .fops = &dev_ops,
};
/*** END device I/O **/



/*** Kernel module initialization and teardown ***/
static int
sched_monitor_init(void)
{
    int status;

    /* Create a character device to communicate with user-space via file I/O operations */
    status = misc_register(&dev_handle);
    if (status != 0) {
        printk(KERN_ERR "Failed to register misc. device for module\n");
        return status;
    }

    /* Enable preempt notifiers globally */
    preempt_notifier_inc();

    printk(KERN_INFO "Loaded sched_monitor module. HZ=%d\n", HZ);

    return 0;
}

static void 
sched_monitor_exit(void)
{
    /* Disable preempt notifier globally */
    preempt_notifier_dec();

    /* Deregister our device file */
    misc_deregister(&dev_handle);

    printk(KERN_INFO "Unloaded sched_monitor module\n");
}

module_init(sched_monitor_init);
module_exit(sched_monitor_exit);
/*** End kernel module initialization and teardown ***/


/* Misc module info */
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Nathan Jarvis, Patrick Gardner, Noah Saffer");
MODULE_DESCRIPTION ("CSE 422S Lab 1");
