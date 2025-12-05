#!/bin/bash
set -eou pipefail

# Temporary file to capture bpftrace output
tmpfile=$(mktemp /tmp/bpftrace.out.XXXXXX)

cleanup() {
    # Kill background server if still running
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    rm -f "$tmpfile"
}
trap 'cleanup' EXIT

# Start server in background (unbuffered)
LD_PRELOAD=$(gcc -print-file-name=libasan.so) stdbuf -o0 -e0 ./bin/server &
server_pid=$!

# Run bpftrace and capture its output
bpftrace -e '
tracepoint:raw_syscalls:sys_enter {
    @id[tid] = args->id;
}

tracepoint:raw_syscalls:sys_exit {
    delete(@id[tid]);
}

tracepoint:sched:sched_switch
/@id[args->prev_pid] && args->prev_state != 0 && args->prev_comm == "server" /
{
    printf("BLOCK in syscall=%d comm=%s tid=%d pid=%d\n",
        @id[args->prev_pid],
        args->prev_comm,
        args->prev_pid,
        pid);
    exit();
}
' > "$tmpfile" 2>&1

# If output contains BLOCK, return non-zero
if grep -q "BLOCK" "$tmpfile"; then
    cat "$tmpfile" >&2
    echo "bpftrace detected BLOCK" >&2
    exit 1
fi

# No BLOCK detected
exit 0