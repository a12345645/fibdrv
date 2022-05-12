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

int bn_clz(const bign *bn)
{
    return __builtin_clz(bn->num[bn->len - 1]);
}

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
    if (bn == NULL)
        return 0;
    bn->num = (uint *) krealloc(bn->num, size * 4, GFP_KERNEL);
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

int bn_cpy(bign *dst, bign *src)
{
    if (dst->size > src->size) {
        if (!bn_resize(dst, src->size)) {
            return 0;
        }
    }
    int i;
    for (i = 0; i < src->size; i++) {
        dst->num[i] = src->num[i];
    }
    for (i; i < dst->size; i++) {
        dst->num[i] = 0;
    }
    dst->len = src->len;
    return 1;
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

void bn_sub(bign *out, const bign *x, const bign *y)
{
    int carry = 0, i;
    bn_set(out, 0);
    for (i = 0; i < y->len + 1; i++) {
        if (x->num[i] < y->num[i] + carry) {
            out->num[i] = (((ulong)(1) << 32) - y->num[i]) + x->num[i] - carry;
            carry = 1;
        } else {
            out->num[i] = x->num[i] - y->num[i] - carry;
            carry = 0;
        }
    }

    for (i; carry || i < x->len; i++) {
        if (carry > x->num[i]) {
            out->num[i] = -1;
            carry = 1;
        } else {
            out->num[i] = x->num[i] - carry;
            carry = 0;
        }
    }

    for (i = x->size - 1; i >= 0; i--) {
        if (out->num[i])
            break;
    }

    out->len = i + 1;
}



void bn_mult_add(bign *bn, ulong n, size_t i)
{
    uint t = n, c = n >> 32;
    n = (ulong) bn->num[i] + t;
    bn->num[i] = n;
    i++;
    n = (ulong) bn->num[i] + c + (n >> 32);
    bn->num[i] = n;
    i++;
    n >>= 32;
    while (n) {
        n = (ulong) bn->num[i] + n;
        bn->num[i] = n;
        i++;
        n >>= 32;
    }
}

int bn_mult(bign *out, bign *x, bign *y)
{
    bn_set(out, 0);
    if (out->size < x->len + y->len + 1) {
        bn_resize(out, x->len + y->len + 1);
    }


    for (int i = 0; i < x->len; i++) {
        uint carry = 0, j;
        for (j = 0; j < y->len; j++) {
            ulong tmp = (ulong) x->num[i] * y->num[j];
            // bn_mult_add(out, tmp, i + j);
            ulong tmp2 = (ulong) out->num[i + j] + (uint) tmp + carry;
            carry = (tmp >> 32) + (tmp2 >> 32);
            out->num[i + j] = tmp2;
        }
        while (carry) {
            ulong tmp2 = (ulong) out->num[i + j] + carry;
            carry = (tmp2 >> 32);
            out->num[i + j] = tmp2;
            j++;
        }
    }

    for (int i = out->size - 1; i >= 0; i--) {
        if (out->num[i]) {
            out->len = i + 1;
            break;
        }
    }

    return 1;
}

int bn_lshift(bign *bn, size_t shift)
{
    size_t z = bn_clz(bn);
    if (bn->size < bn->len + 2)
        bn_resize(bn, bn->size + 2);

    for (int i = bn->len; i > 0; i--)
        bn->num[i] = bn->num[i] << shift | bn->num[i - 1] >> (32 - shift);

    bn->num[0] <<= shift;

    if (!z)
        bn->len++;
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

static long long fib_sequence_bn_fast(long long k, char *buf, size_t buf_len)
{
    if (k == 0) {
        copy_to_user(buf, "0", 1);
        return 1;
    }

    bign f1, f2, t1, t2, t3;

    int ret, d = k / 38 + 5;
    char tmp[(k / 4) + 2];
    memset(tmp, 0, (k / 4) + 2);

    ret = bn_init(&f1, d, 1);
    if (ret == 0)
        goto bn_fast_err;
    ret = bn_init(&f2, d, 1);
    if (ret == 0)
        goto bn_fast_err;
    ret = bn_init(&t1, d, 0);
    if (ret == 0)
        goto bn_fast_err;
    ret = bn_init(&t2, d, 0);
    if (ret == 0)
        goto bn_fast_err;
    ret = bn_init(&t3, d, 0);
    if (ret == 0)
        goto bn_fast_err;


    ulong mask = 1UL << 63;
    while (!(k & mask))
        mask >>= 1;
    mask >>= 1;

    for (mask; mask > 0; mask >>= 1) {
        bn_mult(&t1, &f1, &f1);
        bn_mult(&t2, &f2, &f2);
        bn_add(&t3, &t1, &t2);

        bn_lshift(&f2, 1);
        bn_sub(&t1, &f2, &f1);
        bn_cpy(&t2, &f1);
        bn_mult(&f1, &t1, &t2);

        bn_cpy(&f2, &t3);

        if (k & mask) {
            bn_cpy(&t1, &f1);
            bn_cpy(&t2, &f2);
            bn_cpy(&f1, &f2);
            bn_add(&f2, &t2, &t1);
        }
    }
    ret = bn_to_string(&f1, tmp);
    if (ret == 0 || ret > buf_len) {
        ret = 0;
        goto bn_fast_err;
    }
    copy_to_user(buf, tmp, ret);

bn_fast_err:

    bn_free(&f1);
    bn_free(&f2);
    bn_free(&t1);
    bn_free(&t2);
    bn_free(&t3);
    return k;
}
