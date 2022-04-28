#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/slab.h>

#include "sso.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 1000

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

static ktime_t kt;

static long long fib_sequence(long long k)
{
    /* FIXME: C99 variable-length array (VLA) is not allowed in Linux kernel. */
    long long f[k + 2];

    f[0] = 0;
    f[1] = 1;

    for (int i = 2; i <= k; i++) {
        f[i] = f[i - 1] + f[i - 2];
    }

    return f[k];
}


static __uint128_t fib_sequence_128_bit(long long k, __uint128_t *buf)
{
    __uint128_t f[3];

    f[0] = 0;
    f[1] = 1;

    for (int i = 2; i <= k; i++)
        f[i % 3] = f[(i - 1) % 3] + f[(i - 2) % 3];

    copy_to_user(buf, &f[k % 3], sizeof(__uint128_t));

    return k;
}

typedef struct BigN {
    unsigned long long lower, upper;
} BigN;

static inline void addBigN(struct BigN *output, struct BigN x, struct BigN y)
{
    output->upper = x.upper + y.upper;
    if (y.lower > ~x.lower)
        output->upper++;
    output->lower = x.lower + y.lower;
}

static long long fib_sequence_big_number(long long k, BigN *buf)
{
    BigN f[3];

    f[0].lower = f[0].upper = 0;

    f[1].lower = 1;
    f[1].upper = 0;

    for (int i = 2; i <= k; i++) {
        addBigN(&f[i % 3], f[(i - 1) % 3], f[(i - 2) % 3]);
    }
    copy_to_user(buf, &f[k % 3], sizeof(BigN));

    return k;
}

typedef struct _bn {
    unsigned int *number;
    unsigned int size;
} BigN_arr;

static long long fib_sequence_BigN_arr(long long k, BigN_arr *buf)
{
    return k;
}

typedef struct _str_num {
    char *str;
    int len;
    int digits;
} str_num;

static void add_str_num(str_num *output, str_num x, str_num y)
{
    if (output->len < x.digits + 1) {
        output->str = krealloc(output->str, output->len * 2, GFP_KERNEL);
        output->len *= 2;
    }

    if (x.digits > y.digits)
        y.str[y.digits] = 0;

    int carry = 0, j;
    for (j = 0; j < x.digits; j++) {
        output->str[j] = x.str[j] + y.str[j] + carry;
        carry = output->str[j] >= 10;
        output->str[j] %= 10;
    }
    if (carry)
        output->str[j++] = carry;

    output->digits = j;
}

static long long fib_sequence_string_number(long long k, char *buf)
{
    str_num f[3];

    for (int i = 0; i < 3; i++) {
        f[i].str = kmalloc(128, GFP_KERNEL);
        f[i].len = 128;
        f[i].digits = 1;
    }

    f[0].str[0] = 0;
    f[1].str[0] = 1;


    for (int i = 2; i <= k; i++) {
        add_str_num(&f[i % 3], f[(i - 1) % 3], f[(i - 2) % 3]);
    }
    str_num *ans = &f[k % 3];

    for (int i = 0; i < ans->digits; i++)
        ans->str[i] += '0';

    reverse_str(ans->str, ans->digits);

    copy_to_user(buf, ans->str, ans->digits + 1);

    for (int i = 0; i < 3; i++)
        kfree(f[i].str);

    return k;
}

static long long fib_sequence_sso(long long k, char *buf)
{
    SSO f[3];
    sso_init(&f[0], 0);
    sso_init(&f[1], 1);
    sso_init(&f[2], 0);

    for (int i = 2; i <= k; i++) {
        sso_add(&f[i % 3], &f[(i - 1) % 3], &f[(i - 2) % 3]);
    }


    SSO *ans = &f[k % 3];
    char *data;
    int size, cap;

    get_sso_content(ans, &data, &size, &cap);

    for (int i = 0; i < size; i++)
        data[i] += '0';

    reverse_str(data, size);

    copy_to_user(buf, data, size);

    for (int i = 0; i < 3; i++)
        sso_free(&f[i]);

    return k;
}
static long fib_time_proxy(long long k, char *buf)
{
    kt = ktime_get();
    // long result = fib_sequence_128_bit(k, (__uint128_t *) buf);
    // long result = fib_sequence_big_number(k, (BigN *)buf);
    // long result = fib_sequence_string_number(k, buf);
    long result = fib_sequence_sso(k, buf);
    kt = ktime_sub(ktime_get(), kt);

    return result;
}

static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    return (ssize_t) fib_time_proxy(*offset, buf);
}

static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return ktime_to_ns(kt);
}



static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
// static ssize_t fib_read(struct file *file,
//                         char *buf,
//                         size_t size,
//                         loff_t *offset)
// {
//     return (ssize_t) fib_sequence(*offset);
// }

/* write operation is skipped */
// static ssize_t fib_write(struct file *file,
//                          const char *buf,
//                          size_t size,
//                          loff_t *offset)
// {
//     return 1;
// }

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
