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
int                           countOutliersVpeak(int tray_index,
                                                 bool flag_run_at_25_celcius = true);
int                           countOutliersVbreakdown(int tray_index,
                                                      bool flag_run_at_25_celcius = true);
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



// Compute the average V_peak (IV curve) for a gReader->GetIV()en tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
//
// Input tray index -1 to average ALL available data
int countOutliersVbreakdown(int tray_index, bool flag_run_at_25_celcius) {
  if (tray_index < -1 || tray_index >= gReader->GetSPS()->size()) {
    std::cerr << "Warning in <sipm_batch_summary_sheet_hpp::getAvgVbreakdown>: Invalid index. Default input -1 (all data) will be used" << std::endl;
    tray_index = -1;
  }
  
  int count_outliers = 0;
  if (tray_index == -1) {// Recursive approach--slow??
    for (int i = 0; i < gReader->GetSPS()->size(); ++i) count_outliers += countOutliersVbreakdown(i);
    return count_outliers;
  }
  
  // Compare against the average +/- 50mv
  double V_avg = getAvgVbreakdownAllTrays(flag_run_at_25_celcius);
//  double V_avg = getAvgVbreakdown(tray_index, flag_run_at_25_celcius);
  
  SPS_data* tray_to_analyze = gReader->GetSPS()->at(tray_index);
  double avg_Vbreakdown = 0;
  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd_25C->begin();
         it != tray_to_analyze->SPS_Vbd_25C->end(); ++it) {
      if (std::fabs(*it - V_avg) >= declare_Vbd_outlier_range) ++count_outliers;
      avg_Vbreakdown += *it;
    }
    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd_25C->size());
  } else {// At recoreded temperature
    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd->begin();
         it != tray_to_analyze->SPS_Vbd->end(); ++it)
      if (std::fabs(*it - V_avg) >= declare_Vbd_outlier_range) ++count_outliers;
    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd->size());
  }return count_outliers;
}// End of sipm_analysis_helper::countOutliersVpeak


// TODO complete
// Compute the average V_breakdown (SPS curve) for a given tray
// The computation can be done at the recorded temperatures (which vary)
// or under the extrapolation to 25 degrees Celcius.
//int countOutliersVbreakdown(int tray_index, bool flag_run_at_25_celcius) {
//  if (tray_index < 0 || tray_index >=  gReader->GetSPS()->size()) {
//    std::cerr << "Error in <sipm_batch_summary_sheet_hpp::getAvgVbreakdown>: Invalid index." << std::endl;
//    return -1;
//  }
//
//  // Compare against the average +/- 50mv
//  double V_avg = getAvgVbreakdown(tray_index, flag_run_at_25_celcius);
//  int count_outliers = 0;
//  SPS_data* tray_to_analyze = gReader->GetSPS()->at(tray_index);
//  double avg_Vbreakdown = 0;
//  if (flag_run_at_25_celcius) {// Extrapolated to 25 degrees Celcius
//    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd_25C->begin();
//         it != tray_to_analyze->SPS_Vbd_25C->end(); ++it) {
//      if (std::fabs(*it - V_avg) >= declare_Vbd_outlier_range) ++count_outliers;
//      avg_Vbreakdown += *it;
//    }
//    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd_25C->size());
//  } else {// At recoreded temperature
//    for (std::vector<float>::iterator it = tray_to_analyze->SPS_Vbd->begin();
//         it != tray_to_analyze->SPS_Vbd->end(); ++it)
//      if (std::fabs(*it - V_avg) >= declare_Vbd_outlier_range) ++count_outliers;
//    avg_Vbreakdown /= static_cast<double>(tray_to_analyze->SPS_Vbd->size());
//  }return count_outliers;
//}// End of sipm_analysis_helper::countOutliersVbreakdown



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
