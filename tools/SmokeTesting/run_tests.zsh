#!/usr/bin/env zsh

set -e

clients=("Official" "YoStarJP" "YoStarEN" "YoStarKR" "txwy")
error_clients=()
log_dir="./install/debug"
mkdir -p "$log_dir"

pids=()
client_map=()

for client in "${clients[@]}"; do
    echo "Starting test for $client"
    ./install/smoke_test "$client" > "$log_dir/asst_${client}.log" 2>&1 &
    pids+=($!)
    client_map+=("$client")
done

for i in {0..$(( ${#pids} - 1 ))}; do
    pid=${pids[$i]}
    client=${client_map[$i]}
    
    if ! wait $pid; then
        error_clients+=("$client")
    fi
done

rm -f "$log_dir/asst.log"
for client in "${clients[@]}"; do
    cat "$log_dir/asst_${client}.log" >> "$log_dir/asst.log"
done

if [[ ${#error_clients} -gt 0 ]]; then
    for client in "${error_clients[@]}"; do
        while IFS= read -r line; do
            if [[ "$line" == *"[ERR]"* ]]; then
                print -P "%F{red}${line}%f"
            elif [[ "$line" == *"[INF]"* ]]; then
                print -P "%F{cyan}${line}%f"
            else
                echo "$line"
            fi
        done < "$log_dir/asst_${client}.log"
    done
    exit 1
fi

print -P "%F{green}All tests passed%f"
