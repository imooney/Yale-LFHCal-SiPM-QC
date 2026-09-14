# Yale-LFHCal-SiPM-QC
Post-measurement scripts for analyzing SiPM characteristics

Before running the code, test results should be uplaoded in text file format to the data/[TRAY #]-results/ subdirectory. There should be three text files:
- IV\_result.txt [Results of IV Testing]
- SPS\_result\_onlynumbers.txt [Results of SPS Testing]
- SPS\_result.txt [Verbose Results of SPS Testing]
When these conditions are met, the Hamamatsu Tray Number can be added to the batch\_data.txt text file, and added to the analysis run. The code will then automatically gather and read in the data. 

The resulting data can be analyzed and plotted via interacting with the SiPMDataReader class. Some exaples of this are shown in the files sipm\_analysis\_helper.hpp and sipm\_batch\_summary\_sheet.cpp. 


## Instructions for SiPM-Microscope (code by Levente Pirint)
1. Place the tray carefully in the right position and carefully tighten the screw.
2. Open Terminal and type in these commands:
         cd sipm\_microscope
         source venv/bin/activate
         python3 '/home/operator2/sipm\_project/Sipm\_inspector\_No12.py' 
3. Now you'll see the GUI. Change the COM3 to /dev/ttyACM0 and click on connect. (TOP LEFT CORNER)
4. Click on Start Live View.
5. Locate the first SiPM with the manual control(DO NOT use orange home buttons, unless you want to check the XSLIDE factory home). If you want to make bigger steps, change the step size. FIRST SIPM IS IN THE TOP LEFT CORNER IN THE TRAY
6. When the first SiPM is located, click on Sync Zero. This will set your first SiPM as a 0,0 point.
7.  Go to the Calibration and Tray Setup tab and click on Generate 460 Points. (Do NOT change the step size there)
8. Go back to Inspection Dashboard tab, set the right tray number and click on Create and Set Folder.
9. Check everything is done and you can click on Start Inspection.

### IMPORTANT THINGS LIST
- 0.1 inch = 1016 step
- 1mm = 400 step
- 1 SiPM place = 1520 step horizontally, 1720 step vertically
