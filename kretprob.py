from bcc import BPF 
prog = """ 
#include <uapi/linux/ptrace.h>
#define TASK_COMM_LEN 64

int probe_handler(struct pt_regs *ctx)
{
    char comm[TASK_COMM_LEN];
    u64 ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&comm, sizeof(comm));
    bpf_trace_printk("in: %s %llu\\n", comm, ts);
    return 0;
}

int ret_handler(struct pt_regs *ctx)
{
    char comm[TASK_COMM_LEN];
    u64 ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&comm, sizeof(comm));
    bpf_trace_printk("out:%s %llu\\n", comm, ts);
    return 0;
}
"""

b = BPF(text=prog)
b.attach_kprobe(event="fib_read", fn_name="probe_handler")
b.attach_kretprobe(event="fib_read", fn_name="ret_handler")
b.trace_print()