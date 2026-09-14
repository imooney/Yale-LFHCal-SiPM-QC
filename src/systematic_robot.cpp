//  *--
//  systematic_robot.cpp
//
//  Produces some plots that aim to analyze
//  systematic dependences of the new robotic
//  test stand by Debrecen, especially as relative to the previous one
//
//  Created by Ryan Hamilton on 6/1/2026
//  *--

#include "systematic_analysis_summary.cpp"

//========================================================================== Global Variables





//========================================================================== Forward declarations





//========================================================================== Macro Main

// Main macro method: generate SiPM data
void systematic_robot() {
  
  // *-- Analysis setup
  
  // Initialize SiPMDataReader
  SiPMDataReader* reader = new SiPMDataReader();
  
  // Initialize canvases
  gStyle->SetOptStat(0);
  gStyle->SetPalette(TH2_palette);
  gErrorIgnoreLevel = kWarning;
  
  gCanvas_solo = new TCanvas();
  
  // *-- Analysis tasks: Reproducibility
  
  // Read IV and SPS data for reproducibility tests
  reader->SetSubDirectory("robotsyst");
  reader->ReadFile("../data/syst_traylist_robot.txt");
  reader->ReadDataIV();
  reader->ReadDataSPS();
  
  // Run for both temperature correction states
  for (int i_tempcorr = 0; i_tempcorr < 2; ++i_tempcorr) {
    // Initialize padded canvas
    gCanvas_cassetteplot = new TCanvas();
    gCanvas_cassetteplot->SetCanvasSize(1500,830);
    cassette_pad = buildPad("cassette_pad", 0, 0, 1, 0.75/0.83);
    cassette_pad->cd();
    cassette_pads = divideFlush(gPad, 8, 4, 0.025, 0.005, 0.05, 0.01);
    
    global_flag_run_at_25_celcius = i_tempcorr;
    
    // Initialize global hists
    initializeGlobalReproducabilityHists();
    
    // Make plots from reproducibility tests
    makeReproducabilityHist("250821-1302", true);
    
    std::cout << "debug!" << std::endl;
    // Make composite plots with data from all repeated tests
    drawGlobalReproducabilityHists("robot");
    
    cassette_pads.clear();
  }
  
}// End of systematic_robot::main
