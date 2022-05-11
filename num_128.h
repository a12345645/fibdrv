#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/slab.h>

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