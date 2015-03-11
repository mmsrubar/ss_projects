#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>      // for kmalloc()

static int limit = 10;
module_param(limit, int, S_IRUGO);

static int *even_ptr;

/* @pos on the first call the value of pos will be zero, otherwise it represents
 * a position of the item of your structure you should continue with */
static void *ct_seq_start(struct seq_file *s, loff_t *pos)
{
  printk(KERN_INFO "Entering start(), pos = %Ld.\n", *pos);

  item_ptr;

  if (pos == head) {
    /* pointing at head again, we're done with printing */
    printk(KERN_INFO "Apparently, we're done.\n");
    return NULL;
  } else {
    /* starting */
    pos = head->next;
  }

  return pos;
}

/* @v   is a pointer returned from start() or next()
 */
static int ct_seq_show(struct seq_file *s, void *v)
{
  printk(KERN_INFO "In show(), even = %d.\n", *((int *) v));
  seq_printf(s, "The current value of the even  number is %d\n", *((int *) v));
  return 0;
}

/* A mejor job of this routine is to detect when you have no data left to print
 * and return NULL when that happens. If you're done, you have to bump up both
 * the "offset" and the corresponding object pointer value.
 */
static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
  printk(KERN_INFO "In next(), v = %pX, pos = %Ld.\n", v, *pos);

  pos = pos->next
  if (pos = head) {
    return NULL;
  }

  return pos;
}

/* Clean up and release all system resources that might have been allocated.
 * This might NOT BE THE END of all printing. Instead your stop routine might
 * have benn invoked simply because you were about to exceed that page limit, at
 * which point your sequence file is "stopped", then restarted with the current
 * offset so that you can pick up where you left off (so if will call the
 * start() again).
 */
static void ct_seq_stop(struct seq_file *s, void *v)
{
  printk(KERN_INFO "Entering stop().\n");

  if (v) {
    printk(KERN_INFO "v is %pX.\n", v);
  } else {
    printk(KERN_INFO "v is null.\n");
  }

  printk(KERN_INFO "In stop(), even_ptr = %pX.\n", even_ptr);

  if (even_ptr) {
    //printk(KERN_INFO "Freeing and clearing even_ptr.\n");
    //kfree(even_ptr);
    even_ptr = NULL;
  } else {
    printk(KERN_INFO "even_ptr is already null.\n");
  }
}

static struct seq_operations ct_seq_ops = {
  .start = ct_seq_start,
  .next = ct_seq_next,
  .stop = ct_seq_stop,
  .show = ct_seq_show
};

static int ct_open(struct inode *inode, struct file *file)
{
  return seq_open(file, &ct_seq_ops);
}

static struct file_operations ct_file_ops = {
  .owner = THIS_MODULE,
  .open = ct_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = seq_release
};

static int ct_init(void)
{
  struct proc_dir_entry *entry = proc_create("events", 0644, NULL, &ct_file_ops);
  return 0;
}

static void ct_exit(void)
{
  remove_proc_entry("events", NULL);
}

module_init(ct_init);
module_exit(ct_exit);
