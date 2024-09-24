// Adapted from Linux Kernel Workbook
// https://lkw.readthedocs.io/en/latest/index.html
/*
 * Module to show how to use the kernel linked list for managing data
 * 1. Add node to linked list
 * 2. Print the list
 * 3. Delete nodes from the list
 * The modules exposes a /proc interface 
 * 1. add 5 -- adds node "5"
 * 2. print -- prints the linked list
 * 3. delete 5 -- deletes the node "5"
 * 4. destroy -- destroys the whole linked list
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/types.h>


#include "ll.h"

struct list_head my_list;
struct proc_dir_entry *proc;

int len,temp;
char *msg;

#define PROC_NAME "linked_list"

int add_node(int number)
{
   struct ll_struct *new_node = kmalloc((size_t) (sizeof(struct ll_struct)), GFP_KERNEL);
   if (!new_node) {
      printk(KERN_INFO
             "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
      return 1;
   }
   new_node->value = number;
   list_add_tail(&(new_node->list), &my_list);
   return 0;
}

void show_list(void)
{
   struct ll_struct *entry = NULL;

   list_for_each_entry(entry, &my_list, list) {
      printk(KERN_INFO "Node is %d\n", entry->value);
   }
}

int delete_node(int number)
{
   struct ll_struct *entry = NULL, *n;

   // list_for_each_entry(entry, &my_list, list) {
   list_for_each_entry_safe(entry, n, &my_list, list) {
      if (entry->value == number) {
         printk(KERN_INFO "Found the element %d\n",
                entry->value);
         list_del(&(entry->list));
         kfree(entry);
         return 0;
      }
   }
   printk(KERN_INFO "Could not find the element %d\n", number);
   return 1;
}


static int __init my_init(void)
{

   printk(KERN_INFO "Hello from Linked List Module");

   if (ll_proc_init()) {
      printk(KERN_INFO "Falied to create the proc interface");
      return -EFAULT;
   }

   INIT_LIST_HEAD(&my_list);
   return 0;
}

static void __exit my_exit(void)
{
   if (proc) {
      proc_cleanup();
      pr_info("Removed the entry");
   }
   printk(KERN_INFO "Bye from Hello Module");
}


ssize_t my_proc_write(struct file *filp, const char __user * buffer, size_t count, loff_t *pos)
{
   char *str;
   char command[20];
   int val = 0;

   // printk("Calling Proc Write");
   str = kmalloc((size_t) count, GFP_KERNEL);
   if (copy_from_user(str, buffer, count)) {
      kfree(str);
      return -EFAULT;
   }

   sscanf(str, "%s %d", command, &val);
   // printk("First Arguement %s\n", command);
   // printk("Second Argument %d\n", val);

   if (!strcmp(command, "add")) {
      /* Add node */
      printk(KERN_INFO "Adding data to linked list %d\n", val);
      add_node(val);
   }

   if (!strcmp(command, "delete")) {
      /* Delete Node */
      printk(KERN_INFO "Deleting node from linked list %d\n", val);
      if (delete_node(val)) {
         printk(KERN_INFO "Delete failed \n");
      }
   }

   if (!strcmp(command, "print")) {
      /* Print the linked list */
      printk(KERN_INFO "Printing the linked list\n");
      show_list();
   }

   kfree(str);
   return count;
}

ssize_t my_proc_read(struct file *filp,char *buf,size_t count,loff_t *offp ) 
{
        char *data;
        int err;
        data=PDE_DATA(file_inode(filp));
        if(!(data)){
                printk(KERN_INFO "Null data");
                return 0;
        }

        if(count>temp) {
                count=temp;
        }

        temp=temp-count;

        err = copy_to_user(buf,data, count);
        if (err) {      
                printk(KERN_INFO "Error in copying data.");
        }

        if(count==0) {
                temp=len;
        }

        return count;
}

// struct file_operations proc_fops = {
//     .read = my_proc_read,
//     .write = my_proc_write,
// };

struct proc_ops proc_fops = {
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
};

int create_new_proc_entry(void) {
        msg="Hello LL Module";
        proc=proc_create_data(PROC_NAME, 0666, NULL, &proc_fops, msg);
        len=strlen(msg);
        temp=len;
        if (proc) {
            return 0;
        } else {
            return -1;
        }
}

int ll_proc_init (void) {
        create_new_proc_entry();
        return 0;
}

void proc_cleanup(void) {
        remove_proc_entry("hello", NULL);
}


module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("abr");