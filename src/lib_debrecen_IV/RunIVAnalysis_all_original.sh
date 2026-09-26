#!/bin/bash

base_path=/home/user/meas_data

if [ -z "$1" ]
then
      echo "empty folder name error!!"
	  exit -1
fi

#for filename in /home/user/data_2024_06/$1/IV/*.bin; do
for folder in $base_path/$1/IV/; do
echo $folder
	rm $folder/result/IV_result.txt
	readarray -d '' entries < <(printf '%s\0' $folder/*.bin | sort -zV)
	for entry in "${entries[@]}"; do
		echo $entry
	   /home/user/IV_anal_yale/IV_analysis "$entry" #$base_path/common_result
	done
done
