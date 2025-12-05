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

select_id=$(ausyscall --exact pselect6 2>/dev/null || ausyscall --exact pselect 2>/dev/null || ausyscall --exact select 2>/dev/null)

echo "Ignoring pselect6 syscall (id: $select_id)"

# Run bpftrace and capture its output (backgrounded so we can exercise the server)
bpftrace -e '
tracepoint:raw_syscalls:sys_enter {
    @id[tid] = args->id;
}

tracepoint:raw_syscalls:sys_exit {
    delete(@id[tid]); 
}

tracepoint:sched:sched_switch
/@id[args->prev_pid] && args->prev_state != 0 && args->prev_comm == "server" && @id[args->prev_pid] != '$select_id' /
{
    printf("BLOCK in syscall=%d comm=%s tid=%d pid=%d\n",
        @id[args->prev_pid],
        args->prev_comm,
        args->prev_pid,
        pid);
    exit();
}
' > "$tmpfile" 2>&1 &

# bpftrace PID so we can wait for it after running echo against the server
bpftrace_pid=$!

sleep 0.5

# Send SOCKS5 greeting to the local server while the trace is running
# Use || true to avoid exiting if nc fails (we still want to collect bpftrace output)
echo -e "\x05\x01\x00" | ncat localhost 1080 || true

# Wait for bpftrace to finish
wait "$bpftrace_pid"

# If output contains BLOCK, return non-zero
if grep -q "BLOCK" "$tmpfile"; then
    cat "$tmpfile" >&2
    
    # Extract syscall number and print name
    syscall_nr=$(grep "BLOCK" "$tmpfile" | head -n1 | sed -E 's/.*syscall=([0-9]+).*/\1/')
    if [[ -n "$syscall_nr" ]]; then
        syscall_name=$(ausyscall "$syscall_nr")
        echo "Blocking syscall: $syscall_name ($syscall_nr)" >&2
    fi

    echo "bpftrace detected BLOCK" >&2
    exit 1
fi

# No BLOCK detected
exit 0