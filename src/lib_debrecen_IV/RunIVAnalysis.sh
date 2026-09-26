#!/bin/bash

rm /home/user/data_2024_06/$1/result/IV_result.txt

#for filename in /home/user/data_2024_06/$1/IV/*.bin; do
readarray -d '' entries < <(printf '%s\0' /home/user/data_2024_06/$1/IV/*.bin | sort -zV)
for entry in "${entries[@]}"; do
    echo $entry
   ./IV_analysis "$entry" 
done
