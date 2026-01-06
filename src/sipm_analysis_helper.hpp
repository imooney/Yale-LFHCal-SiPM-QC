//  *--
//  SiPMDataReader.hpp
//
//  Helper methods for performing small analysis tasks on SiPM data
//  Assumes the global pointer gReader has been constructed properly:
//   *   SiPMDataReader* reader = new SiPMDataReader();
//   *   reader->Read[IV or SPS]Data();
//  these will be enough to instantiate a reader and assign it to the global pointer.
//
//  Created by Ryan Hamilton on 10/28/25
//  *--

#include "global_vars.hpp"
#include "SiPMDataReader.hpp"

#ifndef sipm_analysis_helper_h
#define sipm_analysis_helper_h

//========================================================================== Forward declarations

// Small/general utils
bool                          checkReader();
float                         getAvgFromVector(std::vector<float>& vec);
float                         getAvgFromVectorPointer(std::vector<float>* vec);


// Small Analysis Subroutines: Counting/Tallying
int                           countSiPMsAllTrays();
int                           countValidSiPMs(int tray_index);
int                           countValidSiPMsBatch(std::string batch_label);
int                           countOutliersVpeak(int tray_index,
                                                 bool flag_run_at_25_celcius = true,
                                                 float extra_tolerance = 0);
int                           countOutliersVbreakdown(int tray_index,
                                                      bool flag_run_at_25_celcius = true,
                                                      float extra_tolerance = 0);
int                           countOutliersVpeakBatch(std::string batch_label,
                                                      bool flag_run_at_25_celcius = true,
                                                      float extra_tolerance = 0);
int                           countOutliersVbreakdownBatch(std::string batch_label,
                                                           bool flag_run_at_25_celcius = true,
                                                           float extra_tolerance = 0);
int                           countDarkCurrentOverLimitAllTrays(float limit);

// Small Analysis Subroutines: Averaging
double                        getAvgVpeak(int tray_index,
                                          bool flag_run_at_25_celcius = true);
double                        getAvgVpeakAllTrays(bool flag_run_at_25_celcius = true);
double                        getAvgVbreakdown(int tray_index,
                                               bool flag_run_at_25_celcius = true);
double                        getAvgVbreakdownAllTrays(bool flag_run_at_25_celcius = true);

// Small Analysis Subroutines: RMS/STDev/Error

//========================================================================== General

bool checkReader() {
  if (gReader == NULL) {
    std::cerr << t_red << "Error in <sipm_analysis_helper>: gReader is undefined! Define a SiPMDataReader to proceed." << t_def << std::endl;
    return false;
  }return true;
}

float getAvgFromVector(std::vector<float>& vec) {
  float avg = 0;
  for (std::vector<float>::iterator it = vec.begin(); it != vec.end(); ++it) {
    avg += *it;
  }return avg / vec.size();
}

float getAvgFromVectorPointer(std::vector<float>* vec) {
  float avg = 0;
  for (std::vector<float>::iterator it = vec->begin(); it != vec->end(); ++it) {
    avg += *it;
  }return avg / vec->size();
}


//========================================================================== Counting/Tallying



// Count the SiPMs in all available trays
// Important since the data could vary if some trays are incomplete
int countSiPMsAllTrays() {
  if (!checkReader()) return 0;
  int count_SiPM = 0;
  for (std::vector<IV_data*>::iterator tray_to_analyze = gReader->GetIV()->begin();
       tray_to_analyze != gReader->GetIV()->end(); ++tray_to_analyze) {
    for (std::vector<float>::iterator it = (*tray_to_analyze)->IV_Vpeak->begin();
         it != (*tray_to_analyze)->IV_Vpeak->end(); ++it) {
      if (*it == -999) continue; // -999: failed measurement or missing SiPM
      ++count_SiPM;
    }
  }return count_SiPM;
}// End of sipm_analysis_helper::countSiPMsAllTrays

// Count the number of SiPMs available in a given tray
// Useful in case the number of SiPMs in a tray isn't 460
int countValidSiPMs(int tray_index) {
  if (!checkReader()) return 0;
  if (tray_index < 0 || tray_index >= gReader->GetTrayStrings()->size()) return 0;
  
  int count_SiPM = 0;
  for (std::vector<float>::iterator it = gReader->GetIV()->at(tray_index)->IV_Vpeak->begin();
       it != gReader->GetIV()->at(tray_index)->IV_Vpeak->end(); ++it) {
    if (*it == -999) continue; // -999: failed measurement or missing SiPM
    ++count_SiPM;
  }return count_SiPM;
}// End of sipm_analysis_helper::countValidSiPMs

// Count the number of valid SiPMsin a given batch of SiPMs
// This considers trays with a substring "[batch label]" in the
// tray string, i.e. "250821-1301" in batch "250821".
int countValidSiPMsBatch(std::string batch_label) {
  std::vector<std::string>* batch_strings = gReader->GetTrayStrings();
  
  int total_valid_sipm = 0;
  int count = -1;
  for (std::vector<std::string>::iterator it = batch_strings->begin(); it != batch_strings->end(); ++it) {
    ++count;
    
    if (it->find(batch_label) == std::string::npos) continue;
    
    total_valid_sipm += countValidSiPMs(count);
  }// End of string loop
  return total_valid_sipm;
}// End of sipm_analysis_helper::countValidSiPMs


// Compute the average V_peak (IV curve) for a single tray in gReader
// The IV data must be set by reading in a file to the SiPMDataReader class.
//
// The computation can be done at the recorded temperatures (raw data)
// or under the correction to nominal 25 degrees Celcius.
// Note that the non-temperature corrected values may vary widely if the test
// temperature is not under good control (SiPMs are very temperature sensative)
//
// Use input tray index -1 to average ALL available data.
int countOutliersVpeak(int tray_index, bool flag_run_at_25_celcius, float extra_tolerance) {
  if (tray_index < -1 || tray_index >= gReader->GetIV()->size()) {
    std::cerr << "Warning in <sipm_analysis_helper::countOutliersVpeak>: Invalid index given. Default input -1 (all data) will be used" << std::endl;
    tray_index = -1;
  }
  
  // Check for input -1, average all trays if so via recursion on this method.
  int count_outliers = 0;
  if (tray_index == -1) {// Recursion at depth 1--gather each tray and add them together
    for (int i = 0; i < gReader->GetIV()->size(); ++i) count_outliers += countOutliersVbreakdown(i);
    return count_outliers;
  }
  
  // Compare against the average +/- 50mv
  double V_avg;
  if (flag_use_all_trays_for_averages) 
    V_avg = getAvgVpeakAllTrays(flag_run_at_25_celcius);
  else
    V_avg = getAvgVpeak(tray_index, flag_run_at_25_celcius);
  
  // Use quadrature sum if desired
  double V_outlier;
  if (use_quadrature_sum_for_syst_error)
    V_outlier = std::sqrt(declare_Vbd_outlier_range * declare_Vbd_outlier_range 
                          + extra_tolerance*extra_tolerance);
  else
    V_outlier = declare_Vbd_outlier_range + extra_tolerance;
  
  // Begin tallying outliers against the chosen average
  IV_data* tray_to_analyze = gReader->GetIV()->at(tray_index);
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak_25C->begin();
         it != tray_to_analyze->IV_Vpeak_25C->end(); ++it) {
      
      if (std::fabs(*it - V_avg) >= V_outlier) ++count_outliers;
    }// End of loop over 25C-corrected IV data
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak->begin();
         it != tray_to_analyze->IV_Vpeak->end(); ++it) {
      
      if (std::fabs(*it - V_avg) >= V_outlier) ++count_outliers;
    }// End of loop over raw IV data
  }return count_outliers;
}// End of sipm_analysis_helper::countOutliersVpeak


// Compute the average V_breakdown (Single Photon Spectra) for a single tray in gReader
// The SPS data must be set by reading in a file to the SiPMDataReader class.
//
// The computation can be done at the recorded temperatures (raw data)
// or under the correction to nominal 25 degrees Celcius.
// Note that the non-temperature corrected values may vary widely if the test
// temperature is not under good control (SiPMs are very temperature sensative)
//
// Use input tray index -1 to average ALL available data.
int countOutliersVbreakdown(int tray_index, bool flag_run_at_25_celcius, float extra_tolerance) {
  if (tray_index < -1 || tray_index >= gReader->GetSPS()->size()) {
    std::cerr << "Warning in <sipm_analysis_helper::countOutliersVbreakdown>: Invalid index. Default input -1 (all data) will be used" << std::endl;
    tray_index = -1;
  }
  
  // Check for input -1, average all trays if so via recursion on this method.
  int count_outliers = 0;
  if (tray_index == -1) {// Recursion at depth 1--gather each tray and add them together
    for (int i = 0; i < gReader->GetSPS()->size(); ++i) count_outliers += countOutliersVbreakdown(i);
    return count_outliers;
  }
  
  // Compare against the average +/- 50mv
  double V_avg;
  if (flag_use_all_trays_for_averages)
    V_avg = getAvgVbreakdownAllTrays(flag_run_at_25_celcius);
  else
    V_avg = getAvgVbreakdown(tray_index, flag_run_at_25_celcius);
  
  // Use quadrature sum if desired
  double V_outlier;
  if (use_quadrature_sum_for_syst_error)
    V_outlier = std::sqrt(declare_Vbd_outlier_range * declare_Vbd_outlier_range
                          + extra_tolerance*extra_tolerance);
  else
    V_outlier = declare_Vbd_outlier_range + extra_tolerance;
  
  // Begin tallying outliers against the chosen average
  SPS_data* tray_to_analyze = gReader->GetSPS()->at(tray_index);
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd_25C->begin();
         it != tray_to_analyze->SPS_Vbd_25C->end(); ++it) {
      
      if (std::fabs(*it - V_avg) >= V_outlier) ++count_outliers;
    }// End of loop over 25C-corrected SPS data
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd->begin();
         it != tray_to_analyze->SPS_Vbd->end(); ++it) {
      
      if (std::fabs(*it - V_avg) >= V_outlier) ++count_outliers;
    }// End of loop over raw SPS data
  }return count_outliers;
}// End of sipm_analysis_helper::countOutliersVbreakdown


// Count the number of IV outliers for a set of trays in a batch
// This considers trays with a substring "[batch label]" in the
// tray string, i.e. "250821-1301" in batch "250821".
int countOutliersVpeakBatch(std::string batch_label, bool flag_run_at_25_celcius, float extra_tolerance) {
  std::vector<std::string>* batch_strings = gReader->GetTrayStrings();
  
  int total_outliers = 0;
  int count = -1;
  for (std::vector<std::string>::iterator it = batch_strings->begin(); it != batch_strings->end(); ++it) {
    ++count;
    
    if (it->find(batch_label) == std::string::npos) continue;
    
    total_outliers += countOutliersVpeak(count, flag_run_at_25_celcius, extra_tolerance);
  }// End of string loop
  return total_outliers;
}// End of sipm_analysis_helper::countOutliersVpeakBatch

// Count the number of SPS outliers for a set of trays in a batch
// This considers trays with a substring "[batch label]" in the
// tray string, i.e. "250821-1301" in batch "250821".
int countOutliersVbreakdownBatch(std::string batch_label, bool flag_run_at_25_celcius, float extra_tolerance) {
  std::vector<std::string>* batch_strings = gReader->GetTrayStrings();
  
  int total_outliers = 0;
  int count = -1;
  for (std::vector<std::string>::iterator it = batch_strings->begin(); it != batch_strings->end(); ++it) {
    ++count;
    
    if (it->find(batch_label) == std::string::npos) continue;
    
    total_outliers += countOutliersVbreakdown(count, flag_run_at_25_celcius, extra_tolerance);
  }// End of string loop
  return total_outliers;
}// End of sipm_analysis_helper::countOutliersVbreakdownBatch



// Tally the number of SiPMs with dark current at 4 overvolt above some limit
// Useful for comparing against spec sheet limits
int countDarkCurrentOverLimitAllTrays(float limit) {
  int count_above = 0;
  for (std::vector<IV_data*>::iterator tray_to_analyze = gReader->GetIV()->begin();
       tray_to_analyze != gReader->GetIV()->end(); ++tray_to_analyze) {
    for (std::vector<float>::iterator it = (*tray_to_analyze)->Idark_4above->begin();
         it != (*tray_to_analyze)->Idark_4above->end(); ++it) {
      if (*it > limit) ++count_above;
    }
  }return count_above;
}// End of sipm_analysis_helper::countDarkCurrentOverLimitAllTrays

//========================================================================== Averaging



// Compute the average V_peak (IV curve) for a given tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getAvgVpeak(int tray_index, bool flag_run_at_25_celcius) {
  if (tray_index < 0 || tray_index >=  gReader->GetIV()->size()) {
    std::cerr << "Error in <sipm_batch_summary_sheet_hpp::getAvgVpeak>: Invalid index." << std::endl;
    return -1;
  }
  
  IV_data* tray_to_analyze = gReader->GetIV()->at(tray_index);
  double avg_Vpeak = 0;
  int count_failed_measurements = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak_25C->begin();
         it != tray_to_analyze->IV_Vpeak_25C->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      avg_Vpeak += *it;
    }
    avg_Vpeak /= static_cast<double>(tray_to_analyze->IV_Vpeak_25C->size() - count_failed_measurements);
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak->begin();
         it != tray_to_analyze->IV_Vpeak->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      avg_Vpeak += *it;
    }
    avg_Vpeak /= static_cast<double>(tray_to_analyze->IV_Vpeak->size() - count_failed_measurements);
  }return avg_Vpeak;
}// End of sipm_analysis_helper::getAvgVpeak



// Compute the average V_peak (IV curve) for all available trays
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getAvgVpeakAllTrays(bool flag_run_at_25_celcius) {
  double avg_Vpeak = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<IV_data*>::iterator tray_to_analyze = gReader->GetIV()->begin();
         tray_to_analyze != gReader->GetIV()->end(); ++tray_to_analyze) {
      for (std::vector<float>::iterator it = (*tray_to_analyze)->IV_Vpeak_25C->begin();
           it != (*tray_to_analyze)->IV_Vpeak_25C->end(); ++it) {
        if (*it == -999) continue;
        avg_Vpeak += *it;
      }
    }
  } else {// At recoreded temperature
    for (std::vector<IV_data*>::iterator tray_to_analyze = gReader->GetIV()->begin();
         tray_to_analyze != gReader->GetIV()->end(); ++tray_to_analyze) {
      for (std::vector<float>::iterator it = (*tray_to_analyze)->IV_Vpeak->begin();
           it != (*tray_to_analyze)->IV_Vpeak->end(); ++it) {
        if (*it == -999) continue;
        avg_Vpeak += *it;
      }
    }
  }return avg_Vpeak / countSiPMsAllTrays();
}// End of sipm_analysis_helper::getAvgVpeakAllTrays



// Compute the average V_breakdown (SPS curve) for a given tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getAvgVbreakdown(int tray_index, bool flag_run_at_25_celcius) {
  if (tray_index < 0 || tray_index >=  gReader->GetSPS()->size()) {
    std::cerr << "Error in <sipm_batch_summary_sheet_hpp::getAvgVbreakdown>: Invalid index." << std::endl;
    return -1;
  }
  
  SPS_data* tray_to_analyze = gReader->GetSPS()->at(tray_index);
  double avg_Vbreakdown = 0;
  int count_failed_measurements = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd_25C->begin();
         it != tray_to_analyze->SPS_Vbd_25C->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      avg_Vbreakdown += *it;
    }
    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd_25C->size() - count_failed_measurements);
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd->begin();
         it != tray_to_analyze->SPS_Vbd->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      avg_Vbreakdown += *it;
    }
    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd->size() - count_failed_measurements);
  }return avg_Vbreakdown;
}// End of sipm_analysis_helper::getAvgVbreakdown



// Compute the average V_breakdown (SPS curve) for all available trays
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getAvgVbreakdownAllTrays(bool flag_run_at_25_celcius) {
  double avg_Vbreakdown = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<SPS_data*>::iterator tray_to_analyze = gReader->GetSPS()->begin();
         tray_to_analyze != gReader->GetSPS()->end(); ++tray_to_analyze) {
      for (std::vector<float>::iterator it = (*tray_to_analyze)->SPS_Vbd_25C->begin();
           it != (*tray_to_analyze)->SPS_Vbd_25C->end(); ++it) {
        if (*it == -999) continue;
        avg_Vbreakdown += *it;
      }
    }
  } else {// At recoreded temperature
    for (std::vector<SPS_data*>::iterator tray_to_analyze = gReader->GetSPS()->begin();
         tray_to_analyze != gReader->GetSPS()->end(); ++tray_to_analyze) {
      for (std::vector<float>::iterator it = (*tray_to_analyze)->SPS_Vbd->begin();
           it != (*tray_to_analyze)->SPS_Vbd->end(); ++it) {
        if (*it == -999) continue;
        avg_Vbreakdown += *it;
      }
    }
  }return avg_Vbreakdown / countSiPMsAllTrays();
}// End of sipm_analysis_helper::getAvgVbreakdownAllTrays

//========================================================================== RMS/STDev/Error

// Compute the stdev V_peak (IV curve) for a given tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getStdevVpeak(int tray_index, bool flag_run_at_25_celcius) {
  if (tray_index < 0 || tray_index >=  gReader->GetIV()->size()) {
    std::cerr << "Error in <sipm_batch_summary_sheet_hpp::getStdevVpeak>: Invalid index." << std::endl;
    return -1;
  }
  
  IV_data* tray_to_analyze = gReader->GetIV()->at(tray_index);
  double avg_Vpeak = getAvgVpeak(tray_index, flag_run_at_25_celcius);
  double stdev_Vpeak = 0;
  int count_failed_measurements = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak_25C->begin();
         it != tray_to_analyze->IV_Vpeak_25C->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      stdev_Vpeak += (*it - avg_Vpeak)*(*it - avg_Vpeak);
    }
    stdev_Vpeak /= static_cast<double>(tray_to_analyze->IV_Vpeak_25C->size() - count_failed_measurements);
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->IV_Vpeak->begin();
         it != tray_to_analyze->IV_Vpeak->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      stdev_Vpeak += (*it - avg_Vpeak)*(*it - avg_Vpeak);
    }
    stdev_Vpeak /= static_cast<double>(tray_to_analyze->IV_Vpeak->size() - count_failed_measurements);
  }return std::sqrt(stdev_Vpeak);
}// End of sipm_analysis_helper::getStdevVpeak



// Compute the stdev V_breakdown (SPS curve) for a given tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
double getStdevVbreakdown(int tray_index, bool flag_run_at_25_celcius) {
  if (tray_index < 0 || tray_index >=  gReader->GetSPS()->size()) {
    std::cerr << "Error in <sipm_batch_summary_sheet_hpp::getStdevVbreakdown>: Invalid index." << std::endl;
    return -1;
  }
  
  SPS_data* tray_to_analyze = gReader->GetSPS()->at(tray_index);
  double avg_Vbreakdown = getAvgVbreakdown(tray_index, flag_run_at_25_celcius);
  double stdev_Vbreakdown = 0;
  int count_failed_measurements = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd_25C->begin();
         it != tray_to_analyze->SPS_Vbd_25C->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      stdev_Vbreakdown += (*it - avg_Vbreakdown)*(*it - avg_Vbreakdown);
    }
    stdev_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd_25C->size() - count_failed_measurements);
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd->begin();
         it != tray_to_analyze->SPS_Vbd->end(); ++it) {
      if (*it == -999) {++count_failed_measurements; continue;}
      stdev_Vbreakdown += (*it - avg_Vbreakdown)*(*it - avg_Vbreakdown);
    }
    stdev_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd->size() - count_failed_measurements);
  }return std::sqrt(stdev_Vbreakdown);
}// End of sipm_analysis_helper::getStdevVbreakdown


#endif /* sipm_analysis_helper_h */
