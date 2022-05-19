#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/reboot.h>
#include <linux/string.h>
MODULE_LICENSE("GPL");

static char *newname;
module_param(newname, charp, 0644);

static int __init rehostname_init(void)
{

        struct file *fp;
        mm_segment_t old_fs;
        loff_t oldpos;
        loff_t newpos;
        fp = filp_open("/etc/hostname", O_RDWR | O_CREAT, 0);

        if (IS_ERR(fp))
        {
                return -1;
        }
        if (strlen(newname) < 0 || strlen(newname) > 65)
        {
                return -2;
        }

        old_fs = get_fs();
        set_fs(KERNEL_DS);

        oldpos = 0;
        newpos = 0;
        char oldname[65];
        kernel_read(fp, oldname, 65, &oldpos);
        kernel_write(fp, "", oldpos, &newpos);
        newpos = 0;
        kernel_write(fp, newname, strlen(newname), &newpos);

        filp_close(fp, NULL);

        set_fs(old_fs);
        orderly_reboot();
        return 0;
}

static void __exit rehostname_exit(void)
{
        printk(KERN_ALERT "rehostname exit\n");
}

module_init(rehostname_init);
module_exit(rehostname_exit);
