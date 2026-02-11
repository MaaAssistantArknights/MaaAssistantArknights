#!/bin/sh

set -e

clients="Official YoStarJP YoStarEN YoStarKR txwy"
error_clients=""
log_dir="./install/debug"
mkdir -p "$log_dir"

pids=""
for client in $clients; do
    ./install/smoke_test "$client" > "$log_dir/asst_${client}.log" 2>&1 &
    pid=$!
    pids="$pids $pid"

    echo "$pid $client" >> "$log_dir/pid_client_map.tmp"
done

for pid in $pids; do
    if wait "$pid"; then
        :
    else
        client=$(grep "^$pid " "$log_dir/pid_client_map.tmp" | cut -d' ' -f2)
        if [ -z "$error_clients" ]; then
            error_clients="$client"
        else
            error_clients="$error_clients $client"
        fi
    fi
done

rm -f "$log_dir/pid_client_map.tmp"

rm -f "$log_dir/asst.log"
for client in $clients; do
    cat "$log_dir/asst_${client}.log" >> "$log_dir/asst.log"
done

if [ -n "$error_clients" ]; then
    for client in $error_clients; do
        while IFS= read -r line; do
            case "$line" in
                *"[ERR]"*)
                    printf "\033[31m%s\033[0m\n" "$line"
                    ;;
                *"[INF]"*)
                    printf "\033[36m%s\033[0m\n" "$line"
                    ;;
                *)
                    echo "$line"
                    ;;
            esac
        done < "$log_dir/asst_${client}.log"
    done
    exit 1
fi

printf "\033[32mAll tests passed\033[0m\n"
