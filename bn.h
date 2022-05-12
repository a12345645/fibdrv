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

#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

#ifndef _unsign_
#define _unsign_
#define ulong unsigned long
#define uint unsigned int
#define uint8_t unsigned char
#endif

typedef struct _bign {
    uint *num;
    size_t size;
    uint len;
} bign;

int bn_init(bign *bn, size_t size, uint n)
{
    if (bn == NULL || size == 0)
        return 0;
    bn->num = (uint *) kcalloc(size, sizeof(uint), GFP_KERNEL);
    if (bn->num == NULL)
        return 0;
    bn->num[0] = n;
    bn->len = 1;
    bn->size = size;
    return 1;
}

int bn_resize(bign *bn, size_t size)
{
    if (bn == NULL || bn->size > size)
        return 0;

    uint *tmp = bn->num;
    bn->num = (uint *) kcalloc(bn->num, size, sizeof(uint), GFP_KERNEL);

    if (tmp != NULL) {
        for (int i = 0; i < bn->len; i++)
            bn->num[i] = tmp[i];
        kfree(tmp);
    }

    if (bn->num == NULL)
        return 0;
    bn->size = size;
    return 1;
}

void bn_free(bign *bn)
{
    if (bn->num != NULL)
        kfree(bn->num);
    bn->num = NULL;
}

int bn_add(bign *out, const bign *bn1, const bign *bn2)
{
    const bign *x, *y;
    if (bn1->len > bn2->len) {
        x = bn1;
        y = bn2;
    } else {
        x = bn2;
        y = bn1;
    }
    if (out->size < max(x->len, y->len) + 1)
        bn_resize(out, max(x->len, y->len) + 2);

    uint i = 0;
    ulong carry = 0;

    for (i = 0; i < y->len; i++) {
        carry = x->num[i] + carry + y->num[i];
        out->num[i] = carry;
        carry >>= 32;
    }
    for (i; i < x->len; i++) {
        carry = x->num[i] + carry;
        out->num[i] = carry;
        carry >>= 32;
    }
    if (carry) {
        out->num[i] = carry;
        i++;
    }
    out->len = i;

    return 1;
}

void bn_set(bign *bn, uint n)
{
    if (bn->size == 0)
        return;
    for (int i = 1; i < bn->size; i++)
        bn->num[i] = 0;
    bn->num[0] = n;
    bn->len = 1;
}

int bn_mult(bign *out, bign *x, bign *y)
{
    bn_set(out, 0);

    for (int i = 0; i < x->len; i++) {
        for (int j = 0; j < y->len; j++) {
            ulong tmp = (ulong) x->num[i] * y->num[y];
            uint t = tmp c = tmp >> 32;

            out->num[]
        }
    }
    return 1;
}

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

int bn_to_string(const bign *bn, char *buf)
{
    int digits = 1;
    for (int i = bn->len - 1; i >= 0; i--) {
        uint mask = 1L << ((sizeof(uint) * 8) - 1);
        for (mask; mask != 0; mask >>= 1) {
            char carry = (mask & bn->num[i]) > 0;
            for (int j = 0; j < digits; j++) {
                buf[j] = (buf[j] << 1) + carry;
                carry = buf[j] >= 10;
                buf[j] %= 10;
            }
            if (carry) {
                buf[digits++] = carry;
                carry = 0;
            }
        }
    }
    for (int i = 0; i < digits; i++)
        buf[i] += '0';
    reverse_str(buf, digits);

    return digits;
}

static long long fib_sequence_bn(long long k, char *buf, size_t buf_len)
{
    bign f[3];
    memset(f, 0, sizeof(bign) * 3);

    int ret, d = k / 38 + 1;
    char tmp[(k / 4) + 2];
    memset(tmp, 0, (k / 4) + 2);

    ret = bn_init(&f[0], d, 0);
    if (ret == 0)
        goto bn_err;
    ret = bn_init(&f[1], d, 1);
    if (ret == 0)
        goto bn_err;
    ret = bn_init(&f[2], d, 0);
    if (ret == 0)
        goto bn_err;

    for (int i = 2; i <= k; i++) {
        ret = bn_add(&f[i % 3], &f[(i - 1) % 3], &f[(i - 2) % 3]);
        if (ret == 0)
            goto bn_err;
    }

    ret = bn_to_string(&f[k % 3], tmp);
    if (ret == 0 || ret > buf_len) {
        ret = 0;
        goto bn_err;
    }
    copy_to_user(buf, tmp, ret);

bn_err:
    if (!tmp)
        kfree(tmp);

    for (int i = 0; i < 3; i++)
        bn_free(&f[i]);

    return k;
}
