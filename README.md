# Yale-LFHCal-SiPM-QC
Post-measurement scripts for analyzing SiPM characteristics

Before running the code, test results should be uplaoded in text file format to the data/[TRAY #]-results/ subdirectory. There should be three text files:
- IV_result.txt [Results of IV Testing]
- SPS_result_onlynumbers.txt [Results of SPS Testing]
- SPS_result.txt [Verbose Results of SPS Testing]
When these conditions are met, the Hamamatsu Tray Number can be added to the batch_data.txt text file, and added to the analysis run. The code will then automatically gather and read in the data. 

The resulting data can be analyzed and plotted via interacting with the SiPMDataReader class. Some exaples of this are shown in the files sipm_analysis_helper.hpp and sipm_batch_summary_sheet.cpp. 
