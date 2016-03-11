#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/pid.h>

#include <stdbool.h>

MODULE_LICENSE("GPL");

/* Currently we believe it's true. Unfortunately, I haven't found a method to
 * determine it without reading from /proc/sys/kernel/pid_max */
#define PID_MAX 32768
bool should_detect[PID_MAX];

struct kretprobe sys_mmap_kretprobe;

// Structure for /proc/sandboxer file
struct proc_dir_entry *sandboxer_proc_entry;

// See fs/proc/array.c and fs/proc/task_mmu.c for the details
// Temporarily we need to have CONFIG_MMU enabled in kernel
unsigned long get_task_vm_size(struct task_struct* task)
{
    unsigned long ret = 0;
    struct mm_struct *mm = get_task_mm(task);

    if (mm)
        ret = mm->total_vm;

    return ret;
}

int sandboxer_sys_mmap_return_handler(struct kretprobe_instance *ri, struct pt_regs* regs)
{
    if (should_detect[current->pid])
    {
        printk(KERN_INFO "[sandboxer] sys_mmap return handled. Return value is %lu\n", (unsigned long)(regs_return_value(regs)));
        printk(KERN_INFO "[sandboxer] now task memory is %lu\n", get_task_vm_size(current) * PAGE_SIZE);
    }
    return 0; // Return value is currently ignored
}

// Handler for opening file /proc/sandboxer
int sandboxer_proc_entry_open(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_open called\n");
    return 0;
}

static const char SANDBOX_RESTRICTED[2] = "1";
static const char SANDBOX_NOT_RESTRICTED[2] = "0";

// Handler for reading from /proc/sandboxer
ssize_t sandboxer_proc_entry_read(struct file* _file, char *buffer, size_t length,
    loff_t *offset)
{
    ssize_t ret = 0;
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_read(%p, %p, %lu, %p) called\n", _file, buffer, length, offset);
    printk(KERN_INFO "[sandboxer] offset value is %lld\n", *offset);
    if (*offset > 0)
        return 0;
    if (should_detect[current->pid])
        ret = sprintf(buffer, SANDBOX_RESTRICTED);
    else
        ret = sprintf(buffer, SANDBOX_NOT_RESTRICTED);
    *offset = ret;
    return ret;
}

// Handler for writing to /proc/sandboxer
ssize_t sandboxer_proc_entry_write(struct file* _file, const char *buffer, 
    size_t length, loff_t * offset)
{
    printk(KERN_INFO "[sandboxer] write %s\n", buffer);
    if (*offset > 0)
        return 0;
    if (strcmp(buffer, SANDBOX_RESTRICTED) == 0)
        should_detect[current->pid] = true;
    else if (strcmp(buffer, SANDBOX_NOT_RESTRICTED) == 0)
        should_detect[current->pid] = false;
    else
        return -EFAULT;
    *offset = 2;
    return 2;
}

int sandboxer_proc_entry_release(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "[sandboxer] sandboxer_proc_entry_release called\n");
    return 0;
}

// For registering prevoius callbacks
static const struct file_operations sandboxer_proc_entry_file_ops = {
    .owner = THIS_MODULE,
    .open = sandboxer_proc_entry_open,
    .read = sandboxer_proc_entry_read,
    .write = sandboxer_proc_entry_write,
    .release = sandboxer_proc_entry_release,
};

static int __init sandboxer_module_init(void)
{
    int i;
    int errno;
    
    printk(KERN_INFO "[sandboxer] init\n");
   
    // Clean should_detect array
    for (i = 0; i < PID_MAX; i++)
        should_detect[i] = false;

    // Create /proc/sandboxer file
    sandboxer_proc_entry = proc_create("sandboxer", 0666, NULL, &sandboxer_proc_entry_file_ops);
    if (sandboxer_proc_entry == NULL)
    {
        printk(KERN_INFO "[sandboxer] ERROR: error while creating /proc/sandboxer file");
        return -ENOMEM;
    }

    // Register kretprobe
    sys_mmap_kretprobe.kp.symbol_name = "sys_mmap";
    sys_mmap_kretprobe.handler = sandboxer_sys_mmap_return_handler;
    sys_mmap_kretprobe.maxactive = PID_MAX;
    errno = register_kretprobe(&sys_mmap_kretprobe);
    if (errno != 0)
    {
        printk(KERN_INFO "[sandboxer] ERROR: register_kretprobe returned %d.\n", errno);
        return errno;
    }

    return 0;
}

static void __exit sandboxer_module_exit(void)
{
    printk(KERN_INFO "[sandboxer] missed %d probe events\n", sys_mmap_kretprobe.nmissed);
    printk(KERN_INFO "[sandboxer] exit\n");
    unregister_kretprobe(&sys_mmap_kretprobe);
    remove_proc_entry("sandboxer", NULL);
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
