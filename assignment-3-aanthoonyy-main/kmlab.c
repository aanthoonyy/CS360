#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include "kmlab_given.h"
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */ 
#include <linux/uaccess.h> /* for copy_from_user */ 
#include <linux/version.h> 
// Include headers as needed ...
#include <linux/string.h> /* For memset */
#include <linux/list.h>
#include <linux/slab.h> // For kmalloc and kfree
#include <linux/spinlock.h>
#include <linux/workqueue.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devito"); // Change with your lastname
MODULE_DESCRIPTION("CPTS360 Lab 4");

#define DEBUG 1

// Global variables as needed ...
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) 
#define HAVE_PROC_OPS 
#endif 
 


#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "kmlab"
static spinlock_t lock;
struct list_head my_list;
static struct work_struct *workStruct;
static struct workqueue_struct *workQueue;
static struct timer_list my_timer;
// List Stuff ----------------------------------
struct ll_struct {
		struct list_head list;
		long cpuTime;
        int PID;
};

void cleanup_list(void)
{
    struct ll_struct *entry, *tmp;
    list_for_each_entry_safe(entry, tmp, &my_list, list) {
        list_del(&entry->list);
        kfree(entry);
    }
}



// Work stuff ---------------------------------
// queue
// update list_for_each_entry_safe
void work_function(struct work_struct *work){
    struct ll_struct *entry, *temp;
    unsigned long cpuUse;
    int status;
    
    
    list_for_each_entry_safe(entry, temp, &my_list, list){
        status = get_cpu_use(entry->PID, &cpuUse);
        if (status == 0){
            pr_info("updating cpu time");
            entry->cpuTime = cpuUse;
        }
        else if (status == -1){
            pr_info("removing from list :)");
            //spin_lock_irqsave(&lock, flags);
            list_del(&entry->list);
            //spin_unlock_irqrestore(&lock, flags);
            kfree(entry);
        }
    }

    return;
}

// Timer Stuff ---------------------------------
void my_timer_callback(struct timer_list *timer)
{
    printk(KERN_INFO "timer callback function called.\n");
    work_function(workStruct);
    mod_timer(timer, jiffies + msecs_to_jiffies(5000));
}

// PROC STUFF -------------------------------

/* This structure hold information about the /proc file */
static struct proc_dir_entry *our_proc_file;
static struct proc_dir_entry *kmlab_proc_file;

/* The buffer used to store character for this module */

/* The size of the buffer */

/* This function is called then the /proc file is read */
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset)
{
    char *buf;
    unsigned long flags;
    size_t buf_size = 0;
    struct ll_struct *entry;
    int error;

    if (*offset > 0) {
        return 0;
    }
    buf = kmalloc(min(buffer_length, (size_t)PROCFS_MAX_SIZE), GFP_KERNEL);
    // char buf[PROCFS_MAX_SIZE];
    if (buf == NULL) {
        return -ENOMEM;
    }

    spin_lock_irqsave(&lock, flags);
    list_for_each_entry(entry, &my_list, list) {
        if (buf_size >= buffer_length) {
            break;
        }
        buf_size += snprintf(buf + buf_size, PROCFS_MAX_SIZE - buf_size, "%u: CPU time: %lu\n", entry->PID, entry->cpuTime);
    }
    spin_unlock_irqrestore(&lock, flags);

    if (copy_to_user(buffer, buf, buf_size)) {
        error = -EFAULT;
    } else {
        // *offset += buf_size;
        error = buf_size;
    }

    kfree(buf);
    return error;
}

/* This function is called with the /proc file is written. */
static ssize_t procfile_write(struct file *file, const char __user *buff,
                              size_t len, loff_t *off)
{
    
    char *str;
    struct ll_struct *new;
    int result = 0;
    unsigned long flags;
    pr_info("inside proc write ");
    new = kmalloc(sizeof(struct ll_struct), GFP_KERNEL);
    str = kmalloc(len + 1, GFP_KERNEL);

    if (!new || !str) {
        kfree(new);
        kfree(str);
        return -ENOMEM;
    }

    if(copy_from_user(str, buff, len)){
        result = -EFAULT;
    }
    else{
        str[len] = '\0';
        pr_info("line 142");
        if (sscanf(str, "%u", &new->PID) != 1)
        {
            result = -EFAULT;
        }
        else{
            new->cpuTime = 0;
            spin_lock_irqsave(&lock, flags);
            list_add(&new->list, &my_list);
            pr_info("line 147\n");
            spin_unlock_irqrestore(&lock, flags);
            pr_info(" added to list ");
        }
    }

    kfree(str);

    if (result){
        kfree(new);
        return result;
    }

    // Return the number of bytes written
    return len;
}

static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};



// ---------------------------------------------
// kmlab_init - Called when module is loaded
int __init kmlab_init(void)
{

    our_proc_file = proc_mkdir(PROCFS_NAME, NULL);
    kmlab_proc_file = proc_create("status", 0666, our_proc_file, &proc_file_fops);

    if (NULL == our_proc_file) {
        pr_alert("Error:Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

   

   pr_info("/proc/%s created\n", PROCFS_NAME);
   printk(KERN_INFO "Hello world 1.\n");
   
   pr_info("KMLAB MODULE LOADED\n");
   timer_setup(&my_timer, my_timer_callback, 0);
   INIT_LIST_HEAD(&my_list);
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
   spin_lock_init(&lock);
   workQueue = create_workqueue("workqueue");
    if (!workQueue){
        pr_info("failed to create work queue");
    }
   workStruct = kmalloc(sizeof(*workStruct), GFP_KERNEL);
   INIT_WORK(workStruct, work_function);

   
   return 0;   
}

// kmlab_exit - Called when module is unloaded
void __exit kmlab_exit(void)
{
    cleanup_list();
    proc_remove(our_proc_file);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
    del_timer(&my_timer);
    flush_workqueue(workQueue);
    destroy_workqueue(workQueue);
}

// Register init and exit funtions
module_init(kmlab_init);
module_exit(kmlab_exit);


// https://litux.nl/mirror/kerneldevelopment/0672327201/app01lev1sec2.html