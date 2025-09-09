#!/usr/bin/env bash
# filepath: tools/SmokeTesting/run_tests.sh

set -e

clients=("Official" "YostarJP" "YostarEN" "YostarKR" "txwy")
error_clients=()
log_dir="./install/debug"
mkdir -p "$log_dir"

# Run tests in parallel and collect PIDs
declare -A pids
for client in "${clients[@]}"; do
    ./install/smoke_test "$client" > "$log_dir/asst_${client}.log" 2>&1 &
    pids["$client"]=$!
done

# Wait for all jobs and collect errors
for client in "${clients[@]}"; do
    wait "${pids[$client]}" || error_clients+=("$client")
done

# Combine logs
rm -f "$log_dir/asst.log"
for client in "${clients[@]}"; do
    cat "$log_dir/asst_${client}.log" >> "$log_dir/asst.log"
done

# Print errors if any
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

echo -e
