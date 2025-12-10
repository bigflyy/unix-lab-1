#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/time64.h>  // ← ADD THIS (required for time64_to_tm / mktime64)

static struct proc_dir_entry *proc_entry;

static int tsulab_show(struct seq_file *m, void *v)
{
    struct timespec64 current_time;
    time64_t current_sec, target_sec, diff_sec, minutes_left;

    ktime_get_real_ts64(&current_time);
    current_sec = current_time.tv_sec;

    struct tm current_tm;
    time64_to_tm(current_sec, 0, &current_tm);
    
    struct tm target_tm = {
        .tm_sec = 0,
        .tm_min = 59,
        .tm_hour = 23,
        .tm_mday = 31,
        .tm_mon = 11,
        .tm_year = current_tm.tm_year,
        // ← REMOVED .tm_isdst (doesn't exist in kernel's struct tm)
    };
    
    // ← FIXED mktime64 args: year + 1900, month + 1
    target_sec = mktime64(
        target_tm.tm_year + 1900,
        target_tm.tm_mon + 1,
        target_tm.tm_mday,
        target_tm.tm_hour,
        target_tm.tm_min,
        target_tm.tm_sec
    );

    diff_sec = target_sec - current_sec;
    minutes_left = diff_sec / 60;

    seq_printf(m, "%lld\n", minutes_left);
    return 0;
}

static int tsulab_open(struct inode *inode, struct file *file)
{
    return single_open(file, tsulab_show, NULL);
}

// ← FIXED: added missing .proc_read, .proc_lseek, .proc_release
static const struct proc_ops tsulab_ops = {
    .proc_open    = tsulab_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init tsu_module_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");
    proc_entry = proc_create("tsulab", 0444, NULL, &tsulab_ops);  // ← now actually creates the file
    return 0;
}

static void __exit tsu_module_exit(void)
{
    proc_remove(proc_entry);  // ← safe even if NULL in modern kernels
    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_module_init);
module_exit(tsu_module_exit);

MODULE_LICENSE("GPL");