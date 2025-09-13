#!/usr/bin/env bash

set -e

clients=("Official" "YoStarJP" "YoStarEN" "YoStarKR" "txwy")
error_clients=()
log_dir="./debug"
mkdir -p "$log_dir"

declare -A pids
for client in "${clients[@]}"; do
    ./smoke_test "$client" > "$log_dir/asst_${client}.log" 2>&1 &
    pids["$client"]=$!
done

for client in "${clients[@]}"; do
    wait "${pids[$client]}" || error_clients+=("$client")
done

rm -f "$log_dir/asst.log"
for client in "${clients[@]}"; do
    cat "$log_dir/asst_${client}.log" >> "$log_dir/asst.log"
done

if [ "${#error_clients[@]}" -gt 0 ]; then
    pattern="\[ERR\]"
    for client in "${error_clients[@]}"; do
        while IFS= read -r line; do
            if [[ "$line" =~ $pattern ]]; then
                echo -e "\033[31m$line\033[0m"
            else
                echo "$line"
            fi
        done < "$log_dir/asst_${client}.log"
    done
    exit 1
fi

echo -e "\033[32mAll tests passed\033[0m"
