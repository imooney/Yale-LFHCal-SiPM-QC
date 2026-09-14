// Simple code for makine a blank template file for SiPM inspections.
// These files are used to track surface defects observed on SiPMs
// and check for systematic deviaton due to these defects

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cout << "\033[31m" << "Error in <make-blank-area-file.cc>: Not enough arguments!" << "\033[39m" << std::endl;
    std::cout << "arg 1: Tray number to generate blank file for." << std::endl;
    return 0;
  }// End of input check
  
  // Check that the file doesn't already exist
  char filename[100];
  snprintf(filename, 100, "%s-area.txt", argv[1]);
  struct stat check_file;
  if (stat(filename, &check_file) == 0 && !(check_file.st_mode & S_IFDIR)) {
    std::cout << "\033[31m" << "Warning in <make-blank-area-file.cc>: File exists! Exiting to avoid overwrite..." << "\033[39m" << std::endl;
    return 0;
  }// End of file check
  
  
  // Write the file
  std::ofstream outstream(filename);
  for (int r = 0; r < 20; ++r) {
    for (int c = 0; c < 23; ++c) {
      outstream << r << ' ' << c << ' ' << std::endl;
    }
  }
  
  outstream.close();
  return 1;
}
