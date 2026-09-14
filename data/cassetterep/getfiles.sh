#!/bin/bash

basedir="/media/user/12TB_Backup/Yale_LFHCal_SiPM_QC_data/repeated_meas/"

# Request the password so it doesn't need to be re-entered every time
# Requires sshpass module to store the password and hand it to ssh
# Should exist on most linux installations, and on mac can be obtained
# with homebrew: $brew install sshpass
read -s -p "Password for local host :: " password

# Loop over files on the given base directory on the remote server
for i in $( sshpass -p"${password}" ssh -o StrictHostKeyChecking=no user@10.66.7.53 "ls ${basedir}" ); do 
    # Make a directory to mirror the one on the remote server
    if [ -d ${i} ]; then
        rm -r ${i}
    fi
    mkdir "${i}-results"
    
    # SFTP to get the various result files
    cd "${i}-results"
    sshpass -p"${password}" sftp -o StrictHostKeyChecking=no  "user@10.66.7.53:${basedir}${i}/IV/result/*"
    sshpass -p"${password}" sftp -o StrictHostKeyChecking=no  "user@10.66.7.53:${basedir}${i}/SPS/result/*"
    sshpass -p"${password}" sftp -o StrictHostKeyChecking=no  "user@10.66.7.53:${basedir}${i}/SPS/result_playground/*"
    
    
    cd ..
done
