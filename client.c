#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

void reverse_str(char *str, size_t n)
{
    for (int i = 0; i < (n >> 1); i++) {
        char tmp = str[n - i - 1];
        str[n - i - 1] = str[i];
        str[i] = tmp;
    }
}

int uint128_to_string(__uint128_t t, char *buf, size_t buf_size)
{
    memset(buf, 0, buf_size);

    buf_size--;  // '\0'

    int digits = 0;
    buf[digits] = '0';

    while (t > 0 && digits < buf_size) {
        buf[digits++] = t % 10 + '0';
        t /= 10;
    }
    if (t)
        return -1;
    reverse_str(buf, digits);
    return digits;
}

typedef struct BigN {
    unsigned long long lower, upper;
} BigN;


int bn_to_string(BigN *bn, char *buf, size_t buf_size)
{
    memset(buf, 0, buf_size);

    buf_size--;  // '\0'

    unsigned long mask = 1L << ((sizeof(long) << 3) - 1);
    int digits = 1;

    for (mask; mask != 0; mask >>= 1) {
        char carry = (mask & bn->upper) > 0;
        for (int i = 0; i < digits; i++) {
            buf[i] = (buf[i] << 1) + carry;
            carry = buf[i] >= 10;
            buf[i] %= 10;
        }
        if (carry) {
            if ((digits + 1) > buf_size)
                return 0;
            buf[digits++] = carry;
            carry = 0;
        }
    }

    mask = 1L << ((sizeof(long) << 3) - 1);

    for (mask; mask != 0; mask >>= 1) {
        char carry = (mask & bn->lower) > 0;
        for (int i = 0; i < digits; i++) {
            buf[i] = (buf[i] << 1) + carry;
            carry = buf[i] >= 10;
            buf[i] %= 10;
        }
        if (carry) {
            if ((digits + 1) > buf_size)
                return 0;
            buf[digits++] = carry;
            carry = 0;
        }
    }
    for (int i = 0; i < digits; i++)
        buf[i] += '0';
    reverse_str(buf, digits);

    return digits;
}

typedef struct _str_num {
    char *str;
    int len;
    int digits;
} str_num;

int main()
{
    long long sz;

    char buf[1024];
    char write_buf[1024], read_buf[1024];
    int offset = 500; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        // printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    for (int i = 0; i <= offset; i++) {
        memset(read_buf, 0, sizeof(read_buf));

        lseek(fd, i, SEEK_SET);
        sz = read(fd, read_buf, 1024);

        // __uint128_t *t = (__uint128_t *) read_buf;
        // int ret = uint128_to_string(*t, buf, sizeof(buf));

        // BigN *bign = (BigN *)read_buf;
        // int ret = bn_to_string(bign, buf, sizeof(buf));

        // printf("%d %s\n", i, buf);

        printf("%d %s\n", i, read_buf);

        // printf("Reading from " FIB_DEV
        //        " at offset %d, returned the sequence "
        //        "%llu.\n",
        //        i, sz);

        sz = write(fd, write_buf, strlen(write_buf));
        // printf("%d %lld\n", i, sz);
        usleep(10);
    }

    close(fd);
    return 0;
}
