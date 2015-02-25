#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
 
#define PROCF_MAX_SIZE 1024
#define PROCF_NAME "minifirewall"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux minifirewall");
MODULE_AUTHOR("Liu Feipeng/roman10");

//the structure used for procfs
static struct proc_dir_entry *mf_proc_file;
unsigned long procf_buffer_pos;
char *procf_buffer;

static ssize_t procRead(struct file *fp, char *buffer, size_t len, 
                        loff_t *offset)
{
  	static int finished = 0;
    int ret;
    struct mf_rule *a_rule;
    char token[20];

    printk(KERN_INFO "procf_read (/proc/%s) called \n", PROCF_NAME);

    if (finished) {
        printk(KERN_INFO "eof is 1, nothing to read\n");
        finished = 0;
        return 0;
    } else {
        procf_buffer_pos = 0;
        ret = 0;
        finished = 1;
    }
    return ret;
}
 
static ssize_t procWrite(struct file *file, const char *buffer, 
                         size_t count, loff_t *off)
{
   printk(KERN_INFO "procf_write is called.\n");
   /*read the write content into the storage buffer*/
   return count;
}

int procOpen(struct inode *inode, struct file *fp) {
  printk(KERN_INFO "procOpen called\n");
  return 0;
}

int procClose(struct inode *inode, struct file *fp) {
  printk(KERN_INFO "procClose called\n");
        return 0;
}


/* Initialization routine */
int init_module()
{
  printk(KERN_INFO "initialize kernel module\n");
  procf_buffer = (char *) vmalloc(PROCF_MAX_SIZE);

  static struct file_operations procFops;
  procFops.open = procOpen;
  procFops.release = procClose;
  procFops.read = procRead;
  procFops.write = procWrite;

  mf_proc_file = proc_create(PROCF_NAME, 0644, NULL, &procFops);
  if (mf_proc_file==NULL) {
      printk(KERN_INFO "Error: could not initialize /proc/%s\n", PROCF_NAME);
      return -ENOMEM; 
  } 

  printk(KERN_INFO "/proc/%s is created\n", PROCF_NAME);

    return 0;
}

/* Cleanup routine */
void cleanup_module() {
    remove_proc_entry(PROCF_NAME, NULL);
    printk(KERN_INFO "kernel module unloaded.\n");
}
