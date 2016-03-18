#include <linux/proc_fs.h>
#include "sandboxer-proc.h"
#include "sandboxer-core.h"

/* You want to write this to enable sandbox on your thread and it's children */
/* After enabling sandbox you can't turn it off */
static const char SANDBOX_RESTRICTED[2] = "1";


/*int sandboxer_proc_entry_open(struct inode *sp_inode, struct file *sp_file)
{
    (I believe null handler is enough here)
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_open called\n");
    return 0;
}*/ 

ssize_t sandboxer_proc_entry_read(struct file* _file, char *buffer, size_t length,
    loff_t *offset)
{
    /* I have doubts why this should work, what if length is very short or some other weird things? */
    ssize_t ret = 0;
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_read(%p, %p, %lu, %p) called\n", _file, buffer, length, offset);
    printk(KERN_INFO "[sandboxer] offset value is %lld\n", *offset);
    if (*offset > 0)
        return 0;
    ret = sprintf(buffer, "%u", slot_of[current->pid]);
    *offset = ret;
    return ret;
}

ssize_t sandboxer_proc_entry_write(struct file* _file, const char *buffer, 
    size_t length, loff_t * offset)
{
    /* same as for read */
    
    printk(KERN_INFO "[sandboxer] write %s\n", buffer);
    if (*offset > 0)
        return 0;
    if (strcmp(buffer, SANDBOX_RESTRICTED) == 0) {
        u8 slot = create_new_slot();
        if (slot == NOT_SANDBOXED)
            return -EFAULT;
        else
            attach_pid_to_slot(current->pid, slot);
    } else
        return -EFAULT;
    *offset = 2;
    return 2;
}

/*int sandboxer_proc_entry_release(struct inode *sp_inode, struct file *sp_file)
{
    (I belive null handler is enough here)
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_release called\n");
    return 0;
}*/

struct proc_dir_entry *sandboxer_proc_entry;

static const struct file_operations sandboxer_proc_entry_file_ops = {
    .owner = THIS_MODULE,
//    .open = sandboxer_proc_entry_open,
    .read = sandboxer_proc_entry_read,
    .write = sandboxer_proc_entry_write,
//    .release = sandboxer_proc_entry_release,
};

/* returns 0 on success, errno otherways */
int sandboxer_init_proc(void) {
    sandboxer_proc_entry = proc_create("sandboxer", 0666, NULL, &sandboxer_proc_entry_file_ops);
    if (sandboxer_proc_entry == NULL)
        return -EFAULT;
    return 0;
}

void sandboxer_shutdown_proc(void) {
    if (sandboxer_proc_entry != NULL)
        remove_proc_entry("sandboxer", NULL);
}
