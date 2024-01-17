from bcc import BPF 
prog = """ 
#include <uapi/linux/ptrace.h>

int probe_handler(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    bpf_trace_printk("Enter fib_read at  %llu\\n", ts);
    return 0;
}
"""

b = BPF(text=prog)
b.attach_kprobe(event="fib_read", fn_name="probe_handler")
b.trace_print()