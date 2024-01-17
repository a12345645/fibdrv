from bcc import BPF 
prog = """ 
#include <uapi/linux/ptrace.h>
#define TASK_COMM_LEN 64

BPF_HASH(start, u64, u64);

int probe_handler(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    u64 pid = bpf_get_current_pid_tgid();
    start.update(&pid, &ts);
    return 0;
}

int ret_handler(struct pt_regs *ctx)
{
    u64 ts = bpf_ktime_get_ns();
    u64 pid = bpf_get_current_pid_tgid();
    u64 *tsp = (start.lookup(&pid));
    if (tsp != 0) {
        bpf_trace_printk("duration: %llu\\n", ts - *tsp);
        start.delete(&pid);
    }
    return 0;
}
"""

b = BPF(text=prog)
b.attach_kprobe(event="fib_read", fn_name="probe_handler")
b.attach_kretprobe(event="fib_read", fn_name="ret_handler")
b.trace_print()