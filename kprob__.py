from bcc import BPF 
prog = """ 
#include <uapi/linux/ptrace.h>

int kprobe__fib_read(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    bpf_trace_printk("in: %llu\\n", ts);
    return 0;
}

int kretprobe__fib_read(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    bpf_trace_printk("out:%llu\\n", ts);
    return 0;
}
"""

b = BPF(text=prog)
b.trace_print()