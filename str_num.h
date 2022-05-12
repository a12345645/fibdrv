#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/slab.h>
#include <linux/string.h>

typedef struct _str_num {
    char *str;
    int len;
    int digits;
} str_num;
#ifndef _reverse_str_
#define _reverse_str_
void reverse_str(char *str, size_t n)
{
    for (int i = 0; i < (n >> 1); i++) {
        char tmp = str[n - i - 1];
        str[n - i - 1] = str[i];
        str[i] = tmp;
    }
}
#endif
void add_str_num(str_num *output, str_num x, str_num y)
{
    if (output->len < x.digits + 2) {
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
