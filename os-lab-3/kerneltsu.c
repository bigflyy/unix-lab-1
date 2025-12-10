#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>

static struct proc_dir_entry *proc_entry;

static ssize_t tsulab_read(struct file *file_pointer, char __user *buffer,
                          size_t buffer_length, loff_t *offset)
{
    struct timespec64 current_time;
    time64_t current_sec, target_sec, diff_sec, minutes_left;
    char s[16]; 
    int len;

    if (*offset > 0)
        return 0; 

    ktime_get_real_ts64(&current_time);
    current_sec = current_time.tv_sec;
    current_sec += 7 * 3600; // Tomsk - UTC+7

    struct tm current_tm;
    time64_to_tm(current_sec, 0, &current_tm);

    struct tm target_tm = {
        .tm_sec = 0,
        .tm_min = 59,
        .tm_hour = 23,
        .tm_mday = 31,
        .tm_mon = 11,
        .tm_year = current_tm.tm_year,
    };
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

    len = snprintf(s, sizeof(s), "%lld\n", minutes_left);
    if (copy_to_user(buffer, s, len))
        return -EFAULT;
    
    *offset = len;
    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return len;
}

static const struct proc_ops tsulab_ops = {
    .proc_read    = tsulab_read,
};

static int __init tsu_module_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");
    proc_entry = proc_create("tsulab", 0644, NULL, &tsulab_ops);  
    return 0;
}

static void __exit tsu_module_exit(void)
{
    proc_remove(proc_entry); 
    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_module_init);
module_exit(tsu_module_exit);

MODULE_LICENSE("GPL");