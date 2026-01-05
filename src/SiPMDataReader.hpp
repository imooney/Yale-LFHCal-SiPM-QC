//  *--
//  SiPMDataReader.hpp
//
//  Class for reading in SiPM test result data.
//  Also contains data storage struct definitions
//
//  Created by Ryan Hamilton on 10/19/25.
//  *--

#ifndef SiPMDataReader_t
#define SiPMDataReader_t

#include "global_vars.hpp"

// Global pointer for easy access
class SiPMDataReader;
SiPMDataReader* gReader = NULL;

//========================================================================== Storage Format Structs

// Results of IV measurement for a full tray
struct IV_data {
  // SiPM tray row/index identifiers
  std::string tray_note;
  std::vector<int>* row;
  std::vector<int>* col;
  
  // Temperature data
  std::vector<float>* avg_temp;
  std::vector<float>* stdev_temp;
  
  // IV V_peak measurement
  std::vector<float>* IV_Vpeak;
  std::vector<float>* IV_Vpeak_25C;
  
  // Dark current measurement
  std::vector<float>* Idark_3below;
  std::vector<float>* Idark_4above;
  std::vector<float>* Idark_temp;
  
  // Forward resistance measurement
  std::vector<float>* forward_res;
};// structdef :: IV_data



// Results of SPS measurement for a full tray
struct SPS_data {
  // SiPM tray row/index identifiers
  std::string tray_note;
  std::vector<int>* row;
  std::vector<int>* col;
  
  // Temperature data
  std::vector<float>* avg_temp;
  std::vector<float>* stdev_temp;
  
  // SPS spectrum info
  std::vector<int>* SPS_npeaks;
  std::vector<float>* SPS_peakwidth;
  
  // SPS V_breakdown measurement
  std::vector<float>* SPS_Vbd;
  std::vector<float>* SPS_Vbd_25C;
  std::vector<float>* SPS_Vbd_unc;
  std::vector<float>* SPS_chi2ndf;
  
  // SPS fit parameters P0, P1
  std::vector<float>* fit_parm_0;
  std::vector<float>* fit_parm_1;
};// structdef :: SPS_data

//========================================================================== SiPMDataReader class

class SiPMDataReader {
private:
  // *---------------- Class variables
  
  // Primary input file to read strings of trays to use in analysis
  std::string* batch_data_file;
  
  // Strings for Hamamatsu Tray numbers to access from directories
  std::vector<std::string>* tray_strings;
  
  // Data arrays in struct format
  std::vector<struct IV_data*>* IV_internal;
  std::vector<struct SPS_data*>* SPS_internal;
  
  // Flags for systematic analysis
  bool read_for_systematics; // TODO implement flags in reader
  bool has_subscript_results; // production data ends in "-results", systematics often don't.
  
  // flags for printing and debugging
  bool verbose_mode;           // Print a reasonable amount of info about processes as they occur
  bool print_IV_all_SiPMs;     // Print detailed IV test information for each SiPM in the batch list--large output
  bool print_SPS_all_SiPMs;    // Print detailed SPS test information for each SiPM in the batch list--large output
  
  // *---------------- Internal Helper Methods (Main I/O Handlers)
  
  // Read batch tray indices from text file
  // This specifies which trays are to be inspected in the current batch
  void GetBatchStrings() {
    if (verbose_mode) std::cout << "Reading intput file " << t_blu << *batch_data_file << t_def << " for Tray batch numbers...";
    std::ifstream infile(batch_data_file->c_str());
    std::string cline;
    
    // Read specified file...
    bool flag_start = false;
    while (getline(infile, cline)) {
      
      // First line declared after "$START"
      if (!flag_start) {
        if (cline[0] == '$') flag_start = true;
        continue;
      }
      
      if (cline[0] == '#') continue; //For comments
      
      if (read_for_systematics) {
        // Todo! Should make this more dynamic/less dependent on inputs
        std::cout << "Systematics mode TODO" << std::endl;
      }
      
      // Append new tray string to list
      this->tray_strings->push_back(cline);
    }// End of file read
    
    // Check that SiPMs were found in the file
    if (this->tray_strings->empty()) {
      std::cout << t_red << "Failed!" << t_def << " Check file " << batch_data_file << "." << std::endl;
      std::cerr << "Failed to read file for SiPM tray numbers. Exiting..." << std::endl;
      return;
    }
    
    // Read at least one tray successfully!
    if (verbose_mode) std::cout << "Success!" << std::endl;
    
    // Check that directories are valid
    std::vector<std::string>* valid_trays = new std::vector<std::string>();
    std::vector<std::string>* invalid_trays = new std::vector<std::string>();
    if (verbose_mode) std::cout << "Checking tray data..." << std::endl;
    
    // Check tray data
    if (verbose_mode) std::cout << "SiPM Trays in Batch: {";
    for (std::vector<std::string>::iterator it = this->tray_strings->begin(); it != this->tray_strings->end(); ++it) {
      // Check validity of current tray
      bool is_valid = CheckValidTray(*it);
      
      // report in verbose mode
      if (verbose_mode) {
        if (!is_valid) std::cout << t_red;
        else           std::cout << t_grn;
        std::cout << *it << t_def;
        if (it + 1 != this->tray_strings->end()) std::cout << ", ";
      }
      
      // append to valid/invalid
      if (is_valid) valid_trays->push_back(*it);
      else          invalid_trays->push_back(*it);
    }if (verbose_mode) std::cout << '}' << std::endl;
    
    if (!invalid_trays->empty()) {
      
      // If invalid trays found, report on which ones are invalid
      std::cout << t_red << "Warning " << t_def << ":: Trays {";
      for (std::vector<std::string>::iterator it = invalid_trays->begin(); it != invalid_trays->end(); ++it) {
        std::cout << t_red << *it << t_def;
        if (it + 1 != invalid_trays->end()) std::cout << ", ";
      } std::cout << "} do not exist or do not have the necessary files." << std::endl << std::endl;
      std::cout << "Note that each directory should be named [" << t_blu << "TRAY_INDEX" << t_def << "-results]," << std::endl;
      std::cout << "and should contain files {" << t_blu << "IV_result.txt" << t_def << ", ";
      std::cout << t_blu << "SPS_result_onlynumbers.txt" << t_def << "}." << std::endl;
//      std::cout << t_blu << "SPS_result.txt" << t_def << "}." << std::endl << std::endl; // not necessary
      
      // Make a note about requiring "-results" or not depending on current flag
      std::cout << "\nIf the above invalid trays are in the data directory, they may be missed due to name convention." << std::endl;
      if (this->has_subscript_results) {
        std::cout << "SiPMDataReader::has_subscript_results is currently " << t_grn << "true" << t_def << "." << std::endl;
        std::cout << " This means data should be stored as ../data/{Tray identifier}-results/{text files}." << std::endl;
        std::cout << "If this is not desired, disable the flag using SiPMDataReader::SetFlatTrayString()" << std::endl;
      } else {
        std::cout << "SiPMDataReader::has_subscript_results is currently " << t_red << "false" << t_def << "." << std::endl;
        std::cout << " This means data should be stored as ../data/{Tray identifier}/{text files}." << std::endl;
        std::cout << "If this is not desired, enable the flag by setting SiPMDataReader::SetDefTrayString()" << std::endl;
      }
      
      if (valid_trays->empty()) {
        std::cerr << t_red << "No valid trays. Terminating..." << t_def << std::endl;
        return;
      }
      
      tray_strings = valid_trays;
      std::cout << "The code will run with only trays {";
      for (std::vector<std::string>::iterator it = tray_strings->begin(); it != tray_strings->end(); ++it) {
        std::cout << t_grn << *it << t_def;
        if (it + 1 != tray_strings->end()) std::cout << ", ";
      }std::cout << "}." << std::endl;
    } else if (verbose_mode) std::cout << "All trays valid. Continuing to analysis..." << std::endl;
    
    // Valid trays assigned
    delete invalid_trays;
    return;
  }// End of SiPMDataReader::GetBatchStrings
  
  
  
  // Check that a given tray is valid (i.e. its directory exists and has the right files)
  bool CheckValidTray(std::string tray) {
    char dir[50];
    if (this->has_subscript_results) snprintf(dir, 50, "../data/%s-results", tray.c_str());
    else                             snprintf(dir, 50, "../data/%s", tray.c_str());
    
    // Files which each directory should have:
    const int num_subfiles = 2;
    const char filelist[num_subfiles][50] = {
      "IV_result.txt",
      "SPS_result_onlynumbers.txt"
      //"SPS_result.txt"                    // TODO do we need this file? Maybe could be removed from the requirement.
    };
    
    // Use the stat struct to check the directory exists
    
    struct stat check_dir;
    if (stat(dir, &check_dir) == 0) {
      // Directory OK, check files:
      for (int i_file = 0; i_file < num_subfiles; ++i_file) {
        
        // Get current subfile
        char subfile[100];
        snprintf(subfile, 100, "%s/%s",dir,filelist[i_file]);
        
        struct stat check_file;
        if (stat(subfile, &check_file) == 0 && !(check_file.st_mode & S_IFDIR)) continue;
        else goto failed;
      }
      
      // Found all result files.
      return true;
    }
    
    // Either failed to find directory or result files
  failed:
    return false;
  }// End of SiPMDataReader::CheckValidTray
  
  
  
  // TODO Reader for ORNL Format
  
  
  
public:
  // *---------------- Constructors
  
  SiPMDataReader() {
    this->read_for_systematics = false;
    this->has_subscript_results = true;
    
    this->verbose_mode = true;
    this->print_IV_all_SiPMs = false;
    this->print_SPS_all_SiPMs = false;
    
    this->tray_strings = new std::vector<std::string>();
    this->IV_internal = new std::vector<struct IV_data*>();
    this->SPS_internal = new std::vector<struct SPS_data*>();
    
    this->batch_data_file = new std::string();
    
    gReader = this;
  }
  
  SiPMDataReader(const char* batch_file) {
    this->read_for_systematics = false;
    this->has_subscript_results = true;
    
    this->verbose_mode = true;
    this->print_IV_all_SiPMs = false;
    this->print_SPS_all_SiPMs = false;
    
    this->tray_strings = new std::vector<std::string>();
    this->IV_internal = new std::vector<struct IV_data*>();
    this->SPS_internal = new std::vector<struct SPS_data*>();
    
    this->batch_data_file = new std::string(batch_file);
    GetBatchStrings();
    
    gReader = this;
  }
  
  // *---------------- Setters/Getters
  
  void                          GetDataDebrecen()     {GetBatchStrings();}
  void                          GetDataORNL()         {return;} // TODO
  std::vector<IV_data*>*        GetIV()               {return this->IV_internal;}
  std::vector<SPS_data*>*       GetSPS()              {return this->SPS_internal;}
  std::vector<std::string>*     GetTrayStrings()      {return this->tray_strings;}
  
  void                          SetSystematicMode()   {this->read_for_systematics = true;} // should be run before running GetDataDebrecen
  void                          SetFlatTrayString()   {this->has_subscript_results = false;} // do not automatically require "-results" in tray strings
  void                          SetDefTrayString()    {this->has_subscript_results = true;} // do require "-results" in tray strings (typical convention)
  void                          SetVerbose(bool v)    {this->verbose_mode = v;}
  void                          SetPrintIV()          {this->print_IV_all_SiPMs = true;}
  void                          SetPrintSPS()         {this->print_SPS_all_SiPMs = true;}

  // *---------------- Dynamic/Interfacing Getters
  
  void ReadFile(const char* filename) {
    this->tray_strings->clear();
    this->IV_internal->clear();
    this->SPS_internal->clear();
    
    this->batch_data_file = new std::string(filename);
    GetBatchStrings();
  }
  
  void AppendFile(const char* filename) {
    this->batch_data_file = new std::string(filename);
    GetBatchStrings();
  }
  
  std::pair<int,int> GetTrayIndexFromTestIndex(int set, int cassette_index) {
    return std::make_pair((32*set + cassette_index)/23, (32*set + cassette_index)%23);
  }
  
  std::pair<int,int> GetTestIndexFromTrayIndex(int row, int col) {
    return std::make_pair((23*row + col)/32, (23*row + col)%32);
  }
  
  float GetVbdTrayIndexIV(int tray_index, int row, int col, bool temperature_corrected = false) {
    if (temperature_corrected) {
      return this->IV_internal->at(tray_index)->IV_Vpeak_25C->at(23*row + col);
    }return this->IV_internal->at(tray_index)->IV_Vpeak->at(23*row + col);
  }
  
  float GetVbdTestIndexIV(int tray_index, int set, int cassette_index, bool temperature_corrected = false) {
    if (temperature_corrected) {
      return this->IV_internal->at(tray_index)->IV_Vpeak_25C->at(32*set + cassette_index);
    }return this->IV_internal->at(tray_index)->IV_Vpeak->at(32*set + cassette_index);
  }
  
  float GetVbdTrayIndexSPS(int tray_index, int row, int col, bool temperature_corrected = false) {
    if (temperature_corrected) {
      return this->SPS_internal->at(tray_index)->SPS_Vbd_25C->at(23*row + col);
    }return this->SPS_internal->at(tray_index)->SPS_Vbd->at(23*row + col);
  }
  
  float GetVbdTestIndexSPS(int tray_index, int set, int cassette_index, bool temperature_corrected = false) {
    if (temperature_corrected) {
      return this->SPS_internal->at(tray_index)->SPS_Vbd_25C->at(32*set + cassette_index);
    }return this->SPS_internal->at(tray_index)->SPS_Vbd->at(32*set + cassette_index);
  }
  
  bool HasSet(int tray_index, int set_index) {
    return (this->IV_internal->at(tray_index)->IV_Vpeak->at(32*set_index) != -999);
  }
  
  
  // *---------------- SPS/IV Data Readers
  
  
  
  // Read all IV data for the given valid batches obtained from I/O above
  // Output data is stored as a vector of pointers to data storage structs
  //
  // Note that IV data text file have the following column formats (Debrecen):IV HEADER:
  // TRAYID+NOTE, SIPMID, AVERAGE_TEMPERATURE, TEMPERATURE_DEVIATION, RAW_VPEAK, VPEAK(25C), IDARK(-3V)[nA], IDARK(+4V)[nA], TEMERATURE_BEFORE_IDARK_MEASUREMENT, FORWARD_RESISTANCE
  // -------------Example:
  // 250821-1301-2ndcassette 250821-1301_0_2 23.535 0.026 38.2979 38.3460 1.2 7.6 23.60 105.52
  void ReadDataIV() {
    // Store data as a vector of data struct pointers
    this->IV_internal = new std::vector<IV_data*>();
    
    if (verbose_mode) std::cout << "Gathering IV data for " << t_blu << tray_strings->size() << t_def << " trays." << std::endl;
    for (std::vector<std::string>::iterator it = tray_strings->begin(); it != tray_strings->end(); ++it) {
      
      // Form input file for this tray
      char IV_file[75];
      if (this->has_subscript_results) snprintf(IV_file, 75, "../data/%s-results/IV_result.txt", it->c_str());
      else                             snprintf(IV_file, 75, "../data/%s/IV_result.txt", it->c_str());
      
      if (verbose_mode) std::cout << "Gathering IV data for tray " << t_grn << *it << t_def << "...";
      
      // *-- data arrays to append to data struct
      IV_data* current_data = new IV_data();
      std::vector<int>*   row_local           = new std::vector<int>;
      std::vector<int>*   col_local           = new std::vector<int>;
      // Temperature data
      std::vector<float>* avg_temp_local      = new std::vector<float>;
      std::vector<float>* stdev_temp_local    = new std::vector<float>;
      // IV V_peak measurement
      std::vector<float>* IV_Vpeak_local      = new std::vector<float>;
      std::vector<float>* IV_Vpeak_25C_local  = new std::vector<float>;
      // Dark current measurement
      std::vector<float>* Idark_3below_local  = new std::vector<float>;
      std::vector<float>* Idark_4above_local  = new std::vector<float>;
      std::vector<float>* Idark_temp_local    = new std::vector<float>;
      // Forward resistance measurement
      std::vector<float>* forward_res_local   = new std::vector<float>;
      
      // initialize
      for (int i = 0; i < NROW*NCOL; ++i) {
        row_local->push_back(-999);
        col_local->push_back(-999);
        avg_temp_local->push_back(-999);
        stdev_temp_local->push_back(-999);
        IV_Vpeak_local->push_back(-999);
        IV_Vpeak_25C_local->push_back(-999);
        Idark_3below_local->push_back(-999);
        Idark_4above_local->push_back(-999);
        Idark_temp_local->push_back(-999);
        forward_res_local->push_back(-999);
        // -999: failed measurement or missing SiPM 
      }
      
      // *-- READ IV DATA FROM FILE
      std::ifstream infile(IV_file);
      
      std::string data_line;
      bool first_line = true;
      getline(infile, data_line); // skip first line--not data
      while (getline(infile, data_line)) {
        std::stringstream linestream(data_line);
        std::string entry;
        
        getline(linestream, entry, ' '); // 0: TRAYID+NOTE
        if (first_line) current_data->tray_note = entry;
        
        getline(linestream, entry, ' '); // 1: SIPMID
                                         // Final indices separated by underscores
        std::stringstream array_stream(entry);
        std::string c_underscore;
        std::vector<std::string> underscore_delimeter;
        while (getline(array_stream, c_underscore, '_')) underscore_delimeter.push_back(c_underscore);
        // Compute flattened array index from recovered column/row identifier
        int ccol = std::stoi(underscore_delimeter[underscore_delimeter.size()-2]);
        int crow = std::stoi(underscore_delimeter[underscore_delimeter.size()-1]);
        int flattened_index = crow*NCOL + ccol;
        row_local->at(flattened_index) = crow;
        col_local->at(flattened_index) = ccol;
        
        getline(linestream, entry, ' '); // 2: AVERAGE_TEMPERATURE[C]
        avg_temp_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 3: TEMPERATURE_DEVIATION[C]
        stdev_temp_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 4: RAW_VPEAK[V]
        IV_Vpeak_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 5: VPEAK(25C)[V]
        IV_Vpeak_25C_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 6: IDARK(-3V)[nA]
        Idark_3below_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 7: IDARK(+4V)[nA]
        Idark_4above_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 8: TEMERATURE_BEFORE_IDARK_MEASUREMENT[V]
        Idark_temp_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 9: FORWARD_RESISTANCE[Ω] (TODO CHECK THIS IS THE RIGHT UNITS)
        forward_res_local->at(flattened_index) = std::stof(entry);
        
        
        // Report IV results of each SiPM if requested
        if (print_IV_all_SiPMs) {
          std::cout << "SiPM " << *it << " (" << t_blu << crow << t_def << ',' << t_blu << ccol << t_def << ") [" << flattened_index << "] :: " << std::endl;
          std::cout << "Temp " << avg_temp_local->at(flattened_index) << "C +/- " << stdev_temp_local->at(flattened_index) << "C." << std::endl;
          std::cout << "V_peak = " << IV_Vpeak_local->at(flattened_index) << "V >>> " << t_grn << IV_Vpeak_25C_local->at(flattened_index) << t_def << "V @25C. " << std::endl;
          std::cout << "I_dark = " << Idark_3below_local->at(flattened_index) << "nA @(V_op-3), " << Idark_4above_local->at(flattened_index) << "nA @(V_op+4)" << std::endl;
          std::cout << "I_dark measured at " << Idark_temp_local->at(flattened_index) << "C, forward resistance = " << forward_res_local->at(flattened_index) << " Ohm.\n" << std::endl;
        }
        
        first_line = false;
      }// End of processing current SiPM line
      
      // Append array pointers and push back to data struct vector
      current_data->row           = row_local;
      current_data->col           = col_local;
      current_data->avg_temp      = avg_temp_local;
      current_data->stdev_temp    = stdev_temp_local;
      current_data->IV_Vpeak      = IV_Vpeak_local;
      current_data->IV_Vpeak_25C  = IV_Vpeak_25C_local;
      current_data->Idark_3below  = Idark_3below_local;
      current_data->Idark_4above  = Idark_4above_local;
      current_data->Idark_temp    = Idark_temp_local;
      current_data->forward_res   = forward_res_local;
      IV_internal->push_back(current_data);
      
      if (verbose_mode) std::cout << "Done." << std::endl;
    }// End of tray iterator
    
    // Assign global pointer and return
    if (verbose_mode) std::cout << "Finished gathering all IV data." << std::endl;
    return;
  }// End of SiPMDataReader::ReadDataIV
  
  
  
  // Read all SPS data for the given valid batches obtained from I/O above
  // Output data is stored as a vector of pointers to data storage structs
  //
  // Note that SPS data text file have the following column formats (Debrecen):
  // SIPMID, USED_PEAKS, FIT_WIDTH, ROW_VBD, AVERAGE_TEMPERATURE, TEMPERATURE_UNCERAINITY, VBD(25C), VBD_UNCERAINITY, chi2ndf, p0mean, p1mean
  // -------------Example:
  // 250821-1301_0_2 4 400 37.7953 24.9493 0.00188982 37.797 0.0225818 0.117961 -3565.56 94.3384
  void ReadDataSPS() {
    // Store data as a vector of data struct pointers
    this->SPS_internal = new std::vector<SPS_data*>();
    
    if (verbose_mode) std::cout << "Gathering SPS data for " << t_blu << tray_strings->size() << t_def << " trays." << std::endl;
    for (std::vector<std::string>::iterator it = tray_strings->begin(); it != tray_strings->end(); ++it) {
      
      // Form input file for this tray
      char SPS_file[75];
      if (this->has_subscript_results) snprintf(SPS_file, 75, "../data/%s-results/SPS_result_onlynumbers.txt", it->c_str());
      else                             snprintf(SPS_file, 75, "../data/%s/SPS_result_onlynumbers.txt", it->c_str());
      
      if (verbose_mode) std::cout << "Gathering SPS data for tray " << t_grn << *it << t_def << "...";
      
      // *-- data arrays to append to data struct
      SPS_data* current_data = new SPS_data();
      std::vector<int>*   row_local           = new std::vector<int>;
      std::vector<int>*   col_local           = new std::vector<int>;
      // Temperature data
      std::vector<float>* avg_temp_local      = new std::vector<float>;
      std::vector<float>* stdev_temp_local    = new std::vector<float>;
      // SPS spectrum info
      std::vector<int>*   SPS_npeaks_local    = new std::vector<int>;
      std::vector<float>* SPS_peakwidth_local = new std::vector<float>;
      // SPS V_breakdown measurement
      std::vector<float>* SPS_Vbd_local       = new std::vector<float>;
      std::vector<float>* SPS_Vbd_25C_local   = new std::vector<float>;
      std::vector<float>* SPS_Vbd_unc_local   = new std::vector<float>;
      std::vector<float>* SPS_chi2ndf_local   = new std::vector<float>;
      // SPS fit parameters P0, P1
      std::vector<float>* fit_parm_0_local    = new std::vector<float>;
      std::vector<float>* fit_parm_1_local    = new std::vector<float>;
      
      // initialize
      for (int i = 0; i < NROW*NCOL; ++i) {
        row_local->push_back(-999);
        col_local->push_back(-999);
        avg_temp_local->push_back(-999);
        stdev_temp_local->push_back(-999);
        SPS_npeaks_local->push_back(-999);
        SPS_peakwidth_local->push_back(-999);
        SPS_Vbd_local->push_back(-999);
        SPS_Vbd_25C_local->push_back(-999);
        SPS_Vbd_unc_local->push_back(-999);
        SPS_chi2ndf_local->push_back(-999);
        fit_parm_0_local->push_back(-999);
        fit_parm_1_local->push_back(-999);
        // -999: failed measurement or missing SiPM
      }
      
      // *-- READ SPS DATA FROM FILE
      std::ifstream infile(SPS_file);
      
      std::string data_line;
      bool first_line = true;
      while (getline(infile, data_line)) {
        std::stringstream linestream(data_line);
        std::string entry;
        
        getline(linestream, entry, ' '); // 0: SIPMID
                                         // Final indices are separated by underscores
        std::stringstream array_stream(entry);
        std::string c_underscore;
        std::vector<std::string> underscore_delimeter;
        while (getline(array_stream, c_underscore, '_')) underscore_delimeter.push_back(c_underscore);
        // Compute flattened array index from recovered column/row identifier
        int ccol = std::stoi(underscore_delimeter[underscore_delimeter.size()-2]);
        int crow = std::stoi(underscore_delimeter[underscore_delimeter.size()-1]);
        int flattened_index = crow*NCOL + ccol;
        row_local->at(flattened_index) = crow;
        col_local->at(flattened_index) = ccol;
        
        // Measurement message is everything before this which may contain underscores
        if (first_line) {
          int n_to_subtract =  4 + (crow / 10 >= 1) + (ccol / 10 >= 1);
          current_data->tray_note = entry.substr(0, entry.size() - n_to_subtract);
          first_line = false;
        }
        
        getline(linestream, entry, ' '); // 1: USED_PEAKS[NUMBER OF PEAKS FIT IN SPS]
        SPS_npeaks_local->at(flattened_index) = std::stoi(entry);
        
        getline(linestream, entry, ' '); // 2: FIT_WIDTH[SPS PEAK WIDTH ASSUMED BY FITTER]
        SPS_peakwidth_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 3: ROW_VBD[V] (AT MEASURED TEMPERATURE)
        SPS_Vbd_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 4: AVERAGE_TEMPERATURE[C]
        avg_temp_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 5: TEMPERATURE_UNCERAINITY[C]
        stdev_temp_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 6: VBD(25C)[V]
        SPS_Vbd_25C_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 7: VBD_UNCERAINITY[V] (MONTE CARLO ERROR ESTIMATION PROCEDURE)
        SPS_Vbd_unc_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 8: CHI^2/NDF[LINEAR SPS GAIN FIT QUALITY]
        SPS_chi2ndf_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 9: p0mean[AVG p0 AMONG SPS PEAKS]
        fit_parm_0_local->at(flattened_index) = std::stof(entry);
        
        getline(linestream, entry, ' '); // 10: p0mean[AVG p1 AMONG SPS PEAKS]
        fit_parm_1_local->at(flattened_index) = std::stof(entry);
        
        // Report SPS results of each SiPM if requested
        if (print_SPS_all_SiPMs) {
          std::cout << "SiPM " << *it << " (" << t_blu << crow << t_def << ',' << t_blu << ccol << t_def << ") [" << flattened_index << "] :: " << std::endl;
          std::cout << "Temp " << avg_temp_local->at(flattened_index) << "C +/- " << stdev_temp_local->at(flattened_index) << "C." << std::endl;
          std::cout << "SPS V_bd = " << SPS_Vbd_local->at(flattened_index) << "V >>> " << t_grn << SPS_Vbd_25C_local->at(flattened_index) << t_def << "V @25C (";
          std::cout << "+/- " << t_grn << SPS_Vbd_unc_local->at(flattened_index) << t_def << ")." << std::endl;
          std::cout << "SPS Extrapolation fit info :: # of peaks = " << SPS_npeaks_local->at(flattened_index) << " using width " << SPS_peakwidth_local->at(flattened_index);
          std::cout << ", chi^2/ndf = " << SPS_chi2ndf_local->at(flattened_index) << "." << std::endl;
          std::cout << "Fit parm means :: p0 = " << fit_parm_0_local->at(flattened_index) << ", p1 = " << fit_parm_1_local->at(flattened_index) << std::endl << std::endl;
        }
      }// End of processing current SiPM line
      
      // Append array pointers and push back to data struct vector
      current_data->row           = row_local;
      current_data->col           = col_local;
      current_data->avg_temp      = avg_temp_local;
      current_data->stdev_temp    = stdev_temp_local;
      current_data->SPS_npeaks    = SPS_npeaks_local;
      current_data->SPS_peakwidth = SPS_peakwidth_local;
      current_data->SPS_Vbd       = SPS_Vbd_local;
      current_data->SPS_Vbd_25C   = SPS_Vbd_25C_local;
      current_data->SPS_Vbd_unc   = SPS_Vbd_unc_local;
      current_data->SPS_chi2ndf   = SPS_chi2ndf_local;
      current_data->fit_parm_0    = fit_parm_0_local;
      current_data->fit_parm_1    = fit_parm_1_local;
      SPS_internal->push_back(current_data);
      
      if (verbose_mode) std::cout << "Done." << std::endl;
    }// End of tray iterator
    
    // Assign global pointer and return
    if (verbose_mode) std::cout << "Finished gathering all SPS data." << std::endl;
    return;
  }// End of SiPMDataReader::ReadDataSPS
  
  // *---------------- Simple output formatters
  
  
  void WriteCompressedFile(int tray_index) {
    if (tray_index > this->tray_strings->size()) return;
    
    char outfile_dir[100];
    snprintf(outfile_dir, 100, "../data/%s-results/results-condensed.txt",tray_strings->at(tray_index).c_str());
    
    std::ofstream outfile(outfile_dir);
    
    IV_data* tray_IV_data = IV_internal->at(tray_index);
    SPS_data* tray_SPS_data = SPS_internal->at(tray_index);
    
    for (int i = 0; i < tray_IV_data->IV_Vpeak->size(); ++i) {
//      outfile << tray_IV_data->row->at(i) << '\t';
//      outfile << tray_IV_data->col->at(i) << '\t';
      outfile << tray_SPS_data->SPS_Vbd->at(i) << '\t';
      outfile << tray_IV_data->IV_Vpeak->at(i) << std::endl;
    }
    
    outfile.close();
  }
  
  
  // *---------------- Destructor
  
  ~SiPMDataReader() {
    delete this->IV_internal;
    delete this->SPS_internal;
  }// End of SiPMDataReader::~SiPMDataReader
  
};

#endif /* SiPMDataReader_t */
