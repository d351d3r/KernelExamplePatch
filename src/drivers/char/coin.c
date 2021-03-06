#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/random.h>
#include <linux/debugfs.h>

#define DEVNAME "coin"
#define LEN  20
enum values {HEAD, TAIL};

struct dentry *dir, *file;
int file_value;
int stats[2] = {0, 0};
char *msg[2] = {"head\n", "tail\n"};

static int major;
static struct class *class_coin;
static struct device *dev_coin;

static ssize_t r_coin(struct file *f, char __user *b,
                      size_t cnt, loff_t *lf)
{
        char *ret;
        u32 value = random32() % 2;
        ret = msg[value];
        stats[value]++;
        return simple_read_from_buffer(b, cnt,
                                       lf, ret,
                                       strlen(ret));
}

static struct file_operations fops = { .read = r_coin };

#ifdef CONFIG_COIN_STAT
static ssize_t r_stat(struct file *f, char __user *b,
                         size_t cnt, loff_t *lf)
{
        char buf[LEN];
        snprintf(buf, LEN, "head=%d tail=%d\n",
                 stats[HEAD], stats[TAIL]);
        return simple_read_from_buffer(b, cnt,
                                       lf, buf,
                                       strlen(buf));
}

static struct file_operations fstat = { .read = r_stat };
#endif

int init_module(void)
{
        void *ptr_err;
        major = register_chrdev(0, DEVNAME, &fops);
        if (major < 0)
                return major;

        class_coin = class_create(THIS_MODULE,
                                  DEVNAME);
        if (IS_ERR(class_coin)) {
                ptr_err = class_coin;
                goto err_class;
        }

        dev_coin = device_create(class_coin, NULL,
                                 MKDEV(major, 0),
                                 NULL, DEVNAME);
        if (IS_ERR(dev_coin))
                goto err_dev;

#ifdef CONFIG_COIN_STAT
        dir = debugfs_create_dir("coin", NULL);
        file = debugfs_create_file("stats", 0644,
                                   dir, &file_value,
                                   &fstat);
#endif

        return 0;
err_dev:
        ptr_err = class_coin;
        class_destroy(class_coin);
err_class:
        unregister_chrdev(major, DEVNAME);
        return PTR_ERR(ptr_err);
}

void cleanup_module(void)
{
#ifdef CONFIG_COIN_STAT
        debugfs_remove(file);
        debugfs_remove(dir);
#endif

        device_destroy(class_coin, MKDEV(major, 0));
        class_destroy(class_coin);
        return unregister_chrdev(major, DEVNAME);
}