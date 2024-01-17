from bcc import BPF
prog = """
#include <uapi/linux/ptrace.h>
#include <linux/fs.h>

BPF_HASH(start, u64, u64);
BPF_HASH(args, u64, loff_t);

int probe_handler(struct pt_regs *ctx,
                  struct file *file,
                  char *buf,
                  size_t size,
                  loff_t *offset)

{
    u64 ts = bpf_ktime_get_ns();
    u64 pid = bpf_get_current_pid_tgid();
    loff_t arg = *offset;
    
    start.update(&pid, &ts);
    args.update(&pid, &arg);
    return 0;
}

int ret_handler(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    u64 pid = bpf_get_current_pid_tgid();
    u64 *tsp = start.lookup(&pid);
    loff_t *offset = args.lookup(&pid);

    if (tsp != 0 && offset != 0) {
        bpf_trace_printk("%lld, %llu\\n", *offset, ts - *tsp);
        start.delete(&pid);
        args.delete(&pid);
    }
    return 0;
}
"""

b = BPF(text=prog)
b.attach_kprobe(event="fib_read", fn_name="probe_handler")
b.attach_kretprobe(event="fib_read", fn_name="ret_handler")

while 1:
    try:
        res = b.trace_fields()
    except ValueError:
        continue
    print(res[5].decode("UTF-8"))