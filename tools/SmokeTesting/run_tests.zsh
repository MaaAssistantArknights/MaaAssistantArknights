#!/usr/bin/env zsh

set -e

clients=("Official" "YoStarJP" "YoStarEN" "YoStarKR" "txwy")
error_clients=()
log_dir="./install/debug"
mkdir -p "$log_dir"

typeset -A pids
for client in "${clients[@]}"; do
    ./install/smoke_test "$client" > "$log_dir/asst_${client}.log" 2>&1 &
    pids[$client]=$!
done

for client in "${clients[@]}"; do
    wait ${pids[$client]} || error_clients+=("$client")
done

rm -f "$log_dir/asst.log"
for client in "${clients[@]}"; do
    cat "$log_dir/asst_${client}.log" >> "$log_dir/asst.log"
done

if [[ ${#error_clients} -gt 0 ]]; then
    for client in "${error_clients[@]}"; do
        while IFS= read -r line; do
            if [[ "$line" =~ "\[ERR\]" ]]; then
                print -P "%F{red}${line}%f"
            else
                echo "$line"
            fi
        done < "$log_dir/asst_${client}.log"
    done
    exit 1
fi

print -P "%F{green}All tests passed%f"
