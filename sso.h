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

#ifndef _unsign_
#define _unsign_
#define ulong unsigned long
#define uint unsigned int
#define uint8_t unsigned char
#endif

#define SSO_CAP sizeof(SSO)
#define MSB 0x80000000

typedef union {
    struct {
        char data[15];
        uint8_t sso_size : 6, sso_size_msb : 1, sso_capacity_msb : 1;
    };
    struct {
        char *ptr;
        ulong size : 31, capacity : 31, size_msb : 1, capacity_msb : 1;
    };
} SSO;

int sso_init(SSO *str, ulong n)
{
    uint size = n == 0, i;
    ulong tmp = n;
    memset(str, 0, sizeof(SSO));
    while (tmp > 0) {
        size++;
        tmp /= 10;
    }

    if (size <= SSO_CAP) {
        for (i = 0; i < size; i++) {
            str->data[i] = n % 10;
            n /= 10;
        }
        str->sso_size = SSO_CAP - size;
    } else {
        int capacity = size + (size % 8);
        char *ptr = kcalloc(capacity, sizeof(char), GFP_KERNEL);
        if (ptr == NULL)
            return 0;

        for (i = 0; i < size; i++) {
            ptr[i] = n % 10;
            n /= 10;
        }

        str->ptr = ptr;
        str->capacity = capacity;
        str->capacity_msb = (capacity & MSB) > 0;
        str->size = size;
        str->size_msb = !(size & MSB);
    }
    return 1;
}

void get_sso_content(SSO *n, char **data, int *size, int *capacity)
{
    if (n->capacity_msb == 0 && n->size_msb == 0) {
        *data = (char *) n;
        *capacity = SSO_CAP;
        *size = *capacity - n->sso_size;
        return;
    }

    *data = n->ptr;
    *size = n->size;
    if (!n->size_msb)
        *size |= MSB;
    *capacity = n->capacity;
    if (n->capacity_msb)
        *capacity |= MSB;
}

void sso_set_size(SSO *n, int s)
{
    if (n->capacity_msb == 0 && n->size_msb == 0) {
        n->sso_size = SSO_CAP - s;
        return;
    }
    n->size = s;
    n->size_msb = !(s & MSB);
}
int sso_realloc(SSO *n, int c)
{
    if (c < SSO_CAP)
        return c;
    if (n->capacity_msb == 0 && n->size_msb == 0) {
        n->ptr = kcalloc(c, sizeof(char), GFP_KERNEL);
        if (n->ptr == NULL)
            return 0;

        n->capacity = c;
        n->capacity_msb = (c & MSB) > 0;
        n->size = 0;
        n->size_msb = 1;
    }
    n->ptr = krealloc(n->ptr, c, GFP_KERNEL);
    if (n->ptr == NULL)
        return 0;
    n->capacity = c;
    n->capacity_msb = (c & MSB) > 0;
    return c;
}
int sso_add(SSO *output, SSO *x, SSO *y)
{
    char *ans, *xn, *yn;
    int ans_size, xn_size, yn_size, ans_cap, xn_cap, yn_cap;

    get_sso_content(output, &ans, &ans_size, &ans_cap);
    get_sso_content(x, &xn, &xn_size, &xn_cap);
    get_sso_content(y, &yn, &yn_size, &yn_cap);

    if (ans_cap < (xn_size + 2) || ans_cap < (yn_size + 2)) {
        if (sso_realloc(output, ans_cap + ans_cap % 8 + 8) == 0)
            return 0;
        get_sso_content(output, &ans, &ans_size, &ans_cap);
    }

    if (xn_size > yn_size)
        yn[yn_size] = 0;

    int carry = 0, j;
    for (j = 0; j < xn_size; j++) {
        ans[j] = xn[j] + yn[j] + carry;
        carry = ans[j] >= 10;
        ans[j] %= 10;
    }
    if (carry)
        ans[j++] = carry;

    sso_set_size(output, j);

    return j;
}

void sso_free(SSO *n)
{
    if (n->capacity_msb == 0 && n->size_msb == 0)
        return;
    kfree(n->ptr);
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

static long long fib_sequence_sso(long long k, char *buf, size_t buf_len)
{
    SSO f[3];
    int ret;
    ret = sso_init(&f[0], 0);
    if (ret == 0)
        return 0;
    ret = sso_init(&f[1], 1);
    if (ret == 0)
        return 0;
    ret = sso_init(&f[2], 0);
    if (ret == 0)
        return 0;

    for (int i = 2; i <= k; i++) {
        ret = sso_add(&f[i % 3], &f[(i - 1) % 3], &f[(i - 2) % 3]);
        if (ret == 0)
            return 0;
    }

    SSO *ans = &f[k % 3];
    char *data;
    int size, cap;

    get_sso_content(ans, &data, &size, &cap);

    for (int i = 0; i < size; i++)
        data[i] += '0';

    if (size > buf_len)
        goto sso_err;

    reverse_str(data, size);

    ret = copy_to_user(buf, data, size);

sso_err:
    for (int i = 0; i < 3; i++)
        sso_free(&f[i]);

    return k;
}