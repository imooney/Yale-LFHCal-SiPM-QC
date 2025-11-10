//  *--
//  systematic_analysis_summary.cpp
//
//  Produces some plots that aim to analyze
//  systematic dependences of the test stand
//  setup by Debrecen at Yale.
//
//  Created by Ryan Hamilton on 11/1/25.
//  *--

#include "global_vars.hpp"
#include "SiPMDataReader.hpp"
#include "sipm_analysis_helper.hpp"

//========================================================================== Global Variables

// flags to control some options in analysis
bool flag_use_all_trays_for_averages = false;       // Use all available trays' data to compute averages (Recommended ONLY when all trays are similar)
bool global_flag_run_at_25_celcius = false;
// TODO make class variable, I was silly and didn't think I would use this as much as I do...

// Some helpful label strings
const char string_tempcorr[2][50] = {"#color[2]{#bf{NOT}} Temperature corrected to 25C","Temperature corrected to 25C"};
const char string_tempcorr_short[2][10] = {"","_25C"};

// Global plot objects
TCanvas* gCanvas_solo;
TCanvas* gCanvas_cassetteplot;
TPad* cassette_pad;
std::vector<std::vector<TPad*> > cassette_pads;

// Global data collectors
const int nbins_residualhist = 21;
const int nbins_stdevhist = 10;
TH1D* gHist_rep_residual[2];        // Residuals for reproducibility comparisons among SiPMs
TH1D* gHist_rep_stdev[2];           // Stdev of SiPM repeated test distributions in a histogram

// Error estimators
const double error_confidence = 0.9; // TODO frequentist confidence interval
double gRepError_IV[2] = {0,0};     // Mean error from IV reproducibility, useful for other systematics/plots
double gRepError_SPS[2] = {0,0};    // Mean error from SPS reproducibility, useful for other measurements

// Plot limit controls
double voltplot_limits[2] = {37.6, 38.6};
double volthist_range[2] = {-0.06, 0.06};
double darkcurr_limits[2] = {0, 35};


// TODO refine color pallette
Int_t plot_colors[3] = {
  kViolet+1, kOrange+1, kRed+1
};
Int_t plot_colors_alt[3] = {
  kViolet+3, kOrange+2, kRed+2
};

// TODO make plotter class??? Or at least consider it...

//========================================================================== Forward declarations

//Reproducability
void initializeGlobalReproducabilityHists();
void makeReproducabilityHist(std::string base_tray_id);
void drawGlobalReproducabilityHists();

// Temperature
void makeTemperatureScan();

// Cycle

// Operating Voltage V_op
void makeOperatingVoltageScan();

//========================================================================== Macro Main

// Main macro method: generate SiPM data
void systematic_analysis_summary() {
  
  // *-- Analysis setup
  
  // Initialize SiPMDataReader
  SiPMDataReader* reader = new SiPMDataReader();
  
  // Initialize canvases
  gStyle->SetOptStat(0);
  
  gCanvas_solo = new TCanvas();
  
  // *-- Analysis tasks: Reproducibility
  
  // Read IV and SPS data for reproducibility tests
  reader->ReadFile("../batch_data_repsyst.txt");
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
    makeReproducabilityHist("250821-1302");
    makeReproducabilityHist("250821-1303");
    
    // Make composite plots with data from all repeated tests
    drawGlobalReproducabilityHists();
    
    cassette_pads.clear();
  }
  
  
  // *-- Analysis tasks: Operating Voltage
  
  // Read IV and SPS data for vop scan
  reader->SetFlatTrayString(); // Don't require parent directories end in -results
  reader->ReadFile("../batch_data_vopscan.txt");
  reader->ReadDataIV();
  reader->ReadDataSPS();
  
  makeOperatingVoltageScan();
  
}// End of systematic_analysis_summary::main

//========================================================================== Reproducibility tests



// Initialize the global histograms for reproducibility tests
// These keep track of residuals and reproducibility test stdev
// throughout all trays and available data when running makeReproducabilityHist()
void initializeGlobalReproducabilityHists() {
  char testtype[2][5] = {"IV","SPS"};
  for (int i_test = 0; i_test < 2; ++i_test) {
    gHist_rep_residual[i_test] = new TH1D(Form("hist_rep_residual_%s",testtype[i_test]),
                                          ";Reproducability Residual V_{br} - V_{br}^{Rep. Avg.} [mV];Count of SiPM Tests",
                                          nbins_residualhist, volthist_range[0]*1000,volthist_range[1]*1000);
    gHist_rep_residual[i_test]->SetLineColor(plot_colors[i_test]);
    gHist_rep_residual[i_test]->SetFillColorAlpha(plot_colors[i_test], 0.25);
    gHist_rep_residual[i_test]->SetMarkerColor(plot_colors[i_test]);
    
    gHist_rep_stdev[i_test] = new TH1D(Form("hist_rep_stdev_%s",testtype[i_test]),
                                       ";Reproducability StDev #sigma [mV];Count of SiPMs",
                                       nbins_stdevhist, 0, volthist_range[1]*1000);
    gHist_rep_stdev[i_test]->SetLineColor(plot_colors[i_test]);
    gHist_rep_stdev[i_test]->SetFillColorAlpha(plot_colors[i_test], 0.25);
    gHist_rep_stdev[i_test]->SetMarkerColor(plot_colors[i_test]);
  }return;
}// End of systematic_analysis_summary::initializeReproducabilityHists



// Draw and save the global histograms for residuals/stdev of reproducibility tests
// Note that these histograms are filled by running makeReproducabilityHist(), and the
// global hist will only contain data that has been run in that method (i.e. not all from gReader).
void drawGlobalReproducabilityHists() {
  
  // Helpful numbers to add to canvas
  int ntotal_sipms = static_cast<int>(gHist_rep_stdev[0]->GetEntries());
  int tests_per_sipm = static_cast<int>(gHist_rep_residual[0]->GetEntries()/gHist_rep_stdev[0]->GetEntries());
  
  // Reset the canvas
  gCanvas_solo->cd();
  gPad->SetRightMargin(0.04);
  gPad->SetLeftMargin(0.09);
  gPad->SetTicks(1,1);
  
  // Draw residual hists
  gHist_rep_residual[0]->GetXaxis()->SetTitleOffset(1.1);
  gHist_rep_residual[0]->Draw("hist");
  gHist_rep_residual[1]->Draw("hist same");
  
  // Label the plot with some descriptive text
  TLatex* top_tex[6];
  top_tex[0] = drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}",                  gPad->GetLeftMargin(), 0.91, false, kBlack, 0.04);
  top_tex[1] = drawText("#bf{ePIC} Test Stand",                                       gPad->GetLeftMargin(), 0.955, false, kBlack, 0.045);
  top_tex[2] = drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code),               1.-gPad->GetRightMargin(), 0.95, true, kBlack, 0.045);
  top_tex[3] = drawText(Form("%s", string_tempcorr[global_flag_run_at_25_celcius]),   1.-gPad->GetRightMargin(), 0.91, true, kBlack, 0.035);
  top_tex[4] = drawText(Form("%i total SiPMs",ntotal_sipms),                          0.9, 0.83, true, kBlack, 0.035);
  top_tex[5] = drawText(Form("Each tested %i times",tests_per_sipm),                  0.9, 0.78, true, kBlack, 0.035);
  
  // Legend to label which hists are IV/SPS
  TLegend* vbd_legend = new TLegend(0.14, 0.6, 0.4, 0.85);
  vbd_legend->SetLineWidth(0);
  vbd_legend->AddEntry(gHist_rep_residual[0], "V_{bd} from IV curve", "f");
  vbd_legend->AddEntry(gHist_rep_residual[1], "V_{bd} from SPS", "f");
  vbd_legend->Draw();
  
  // Save residual curve
  gCanvas_solo->SaveAs(Form("../plots/systematic_plots/reproducibility%s/batch_reproducibility_residual.pdf",
                            string_tempcorr_short[global_flag_run_at_25_celcius]));
  
  
  // Draw stdev hists
  double max_stdev_plotrange = gHist_rep_stdev[0]->GetBinContent(1) * 1.1;
  gHist_rep_stdev[0]->GetXaxis()->SetTitleOffset(1.1);
//  gHist_rep_stdev[0]->GetYaxis()->SetRangeUser(0, max_stdev_plotrange);
  gHist_rep_stdev[0]->Draw("hist");
  gHist_rep_stdev[1]->Draw("hist same");
  
  // Redraw the descriptive text
  for (int iTex = 0; iTex < 6; ++iTex) top_tex[iTex]->Draw();
  
  // Add lines to mark the average error
  gRepError_IV[global_flag_run_at_25_celcius] /= static_cast<double>(ntotal_sipms);
  gRepError_SPS[global_flag_run_at_25_celcius] /= static_cast<double>(ntotal_sipms);
  std::cout << "SPS error = " << gRepError_SPS[global_flag_run_at_25_celcius] << std::endl;
  
  TLine* avgerr_line_IV = new TLine();
  avgerr_line_IV->SetLineStyle(7);
  avgerr_line_IV->SetLineWidth(2);
  avgerr_line_IV->SetLineColor(plot_colors_alt[0]);
  avgerr_line_IV->DrawLine(1000*gRepError_IV[global_flag_run_at_25_celcius], 0,
                        1000*gRepError_IV[global_flag_run_at_25_celcius],
                        gHist_rep_stdev[0]->GetBinContent(gHist_rep_stdev[0]->FindBin(1000*gRepError_IV[global_flag_run_at_25_celcius])));
  
  TLine* avgerr_line_SPS = new TLine();
  avgerr_line_SPS->SetLineStyle(7);
  avgerr_line_SPS->SetLineWidth(2);
  avgerr_line_SPS->SetLineColor(plot_colors_alt[1]);
  avgerr_line_SPS->DrawLine(1000*gRepError_SPS[global_flag_run_at_25_celcius], 0,
                        1000*gRepError_SPS[global_flag_run_at_25_celcius],
                        gHist_rep_stdev[1]->GetBinContent(gHist_rep_stdev[1]->FindBin(1000*gRepError_SPS[global_flag_run_at_25_celcius])));
  
  // Draw legend + average line
  vbd_legend = new TLegend(0.58, 0.39, 0.9, 0.73);
  vbd_legend->SetLineWidth(0);
  vbd_legend->AddEntry(gHist_rep_residual[0], "V_{bd} from IV curve", "f");
  vbd_legend->AddEntry(gHist_rep_residual[1], "V_{bd} from SPS", "f");
  vbd_legend->AddEntry(avgerr_line_IV, Form("Average #sigma_{Vbd}^{IV} (#color[2]{%.3f} mV)",
                                            1000*gRepError_IV[global_flag_run_at_25_celcius]), "l");
  vbd_legend->AddEntry(avgerr_line_SPS, Form("Average #sigma_{Vbd}^{SPS} (#color[2]{%.3f} mV)",
                                             1000*gRepError_SPS[global_flag_run_at_25_celcius]), "l");
  vbd_legend->Draw();
  
  gCanvas_solo->SaveAs(Form("../plots/systematic_plots/reproducibility%s/batch_reproducibility_stdev.pdf",
                       string_tempcorr_short[global_flag_run_at_25_celcius]));
  
  // Clean hists for another run
  delete gHist_rep_residual[0];
  delete gHist_rep_residual[1];
  delete gHist_rep_stdev[0];
  delete gHist_rep_stdev[1];
}// End of systematic_analysis_summary::drawGlobalReproducabilityHist



// Compose histograms of SiPMs from repeated tests where data is available
// This method will gather the data, filling the global histograms defined above
// and produce an output composite plot with 32 histograms (one for each cassette
// slot) for any test sets which are repeated in the available data from gReader.
void makeReproducabilityHist(std::string base_tray_id) {
  
  // Find which test sets have repeated measurements
  std::vector<std::string>* tray_strings = gReader->GetTrayStrings();
  std::vector<std::string>::iterator iterator_main_test = tray_strings->end();
  int count_index = 0;
  for (std::vector<std::string>::iterator it = tray_strings->begin(); it != tray_strings->end(); ++it) {
    if (it->compare(base_tray_id) == 0) {iterator_main_test = it; break;}
    ++count_index;
  } if (iterator_main_test == tray_strings->end()) {
    std::cout << "Base tray not found in read data. Check input or add data to ../batch_data.txt" << std::endl;
    return;
  }
  
  
  // iterate through next ones while the substring is still the same.
  // Determine which sets of measurements are repeated among all repetitions
  // Should be the same but good to check
  std::vector<int> tray_indices;
  std::vector<bool> repeated;
  std::cout << "Base tray " << t_blu << base_tray_id << t_def << " found! Appending repeated tests..." << std::endl;
  for (std::vector<std::string>::iterator it = iterator_main_test; it != tray_strings->end(); ++it) {
    if (it->substr(0,base_tray_id.size()).compare(base_tray_id) == 0) {
      std::cout << t_blu << *it << t_def << " (index " << count_index << ")." << std::endl;
      tray_indices.push_back(count_index);
      
      if (tray_indices.size() == 1) {++count_index; continue;} // first should have all tests
      
      // Find which sets are repeated
      std::cout << "Repeated sets: { " << t_red;
      if (repeated.size() == 0) { // set repetitions
        for (int i = 0; i < 15; ++i) {
          if (gReader->HasSet(count_index, i)) repeated.push_back(1); // data available
          else                                 repeated.push_back(0); // no data
          if (repeated.back()) std::cout << i << ' ';
        }std::cout << t_def << "}" << std::endl;
      } else {// check that all subsequent tests have the same repeated measurements
        for (int i = 0; i < 15; ++i) if (repeated[i] == 1) {
          if ((repeated[i] && !gReader->HasSet(count_index, i)) ||
              (!repeated[i] && gReader->HasSet(count_index, i))) {
            std::cout << "Bad repeated sets on " << tray_strings->at(count_index) << "!" << std::endl;
            ++count_index;
            tray_indices.back() = -1;
            continue;
          }
        }// made it with all repetitions aligning--good tray.
        std::cout << t_grn << "same! " << t_def << '}' << std::endl;
        
      }
    }
    ++count_index;
  }
  
  if (tray_indices.size() < 2) {
    std::cout << "No further tests found for this tray, results will not be statistically meaningful. Terminating..." << std::endl;
    return;
  }
  
  // Repetition analysis loop -- one for each repeated test set
  double avg_this_tray_IV = getAvgVpeak(tray_indices[0], global_flag_run_at_25_celcius);
  double avg_this_tray_SPS = getAvgVbreakdown(tray_indices[0], global_flag_run_at_25_celcius);
  bool flag_padded = true;
  for (int r = 0; r < 15; ++r) {
    
    // Begin performing repetition analysis: gather data
    bool has_failed_measurements_IV[8][4];
    bool has_failed_measurements_SPS[8][4];
    for (int s = 0; s < 32; ++s) {
      has_failed_measurements_IV[s/4][s%4] = false;
      has_failed_measurements_SPS[s/4][s%4] = false;
    }
    TH1D* repetition_hists_IV[8][4];
    TH1D* repetition_hists_SPS[8][4];
    
    if (!repeated[r]) continue;
    float ylim = 0;
    int total_trays = 0;
    for (int s = 0; s < 32; ++s) {
      
      // find the average for these repeated measurements
      double avg_this_sipm_IV = 0;
      double avg_this_sipm_SPS = 0;
      
      for (int i = 0; i < tray_indices.size(); ++i) {
        if (tray_indices[i] == -1) continue; // flag for bad tray, in case I need it later
        
        // Check for failed measurements and tally for later (IV)
        if (gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius) == -999) {
          has_failed_measurements_IV[s/4][s%4] = true;
          std::pair<int,int> index = gReader->GetTrayIndexFromTestIndex(r, s);
          std::cout << t_red << "Bad IV measurement" << t_def << " in tray index " << tray_indices[i] << ", with SiPM (";
          std::cout << index.first << ',' << index.second << ")." << std::endl;
          continue;
        }avg_this_sipm_IV += gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius);
        
        // Check for failed measurements and tally for later (SPS)
        if (gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius) == -999) {
          has_failed_measurements_SPS[s/4][s%4] = true;
          std::pair<int,int> index = gReader->GetTrayIndexFromTestIndex(r, s);
          std::cout << t_red << "Bad SPS measurement" << t_def << " in tray " << gReader->GetTrayStrings()->at(tray_indices[i]);
          std::cout << ", with SiPM (" << index.first << ',' << index.second << ")." << std::endl;
          continue;
        }avg_this_sipm_SPS += gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius);
        
        // Tally total good SiPM tests
        if (s == 0) total_trays += 1;
      }// End of tray loop
      avg_this_sipm_IV /= total_trays;
      avg_this_sipm_SPS /= total_trays;
      ylim = total_trays + 1.5;
      
      // IV histogram
      repetition_hists_IV[s/4][s%4] = new TH1D(Form("hist_IV_Vbr_set%i_(%i,%i)",r,(r*32 + s)/23,(r*32 + s)%23),
                                               ";V_{br} [V];Counts", 12,
                                               avg_this_tray_IV + volthist_range[0],
                                               avg_this_tray_IV + volthist_range[1]);
      int color_to_use_IV = plot_colors[0];
      if (has_failed_measurements_IV[s/4][s%4]) color_to_use_IV = plot_colors[2];
      repetition_hists_IV[s/4][s%4]->SetLineColor(color_to_use_IV);
      repetition_hists_IV[s/4][s%4]->SetFillColorAlpha(color_to_use_IV, 0.25);
      repetition_hists_IV[s/4][s%4]->SetMarkerColor(color_to_use_IV);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetNdivisions(203);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetNdivisions(204);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetRangeUser(0, ylim);
      
      // SPS histogram
      repetition_hists_SPS[s/4][s%4] = new TH1D(Form("hist_SPS_Vbr_set%i_(%i,%i)",r,(r*32 + s)/23,(r*32 + s)%23),
                                                ";V_{br} [V];Counts", 12,
                                                avg_this_tray_SPS + volthist_range[0],
                                                avg_this_tray_SPS + volthist_range[1]);
      int color_to_use_SPS = plot_colors[1];
      if (has_failed_measurements_SPS[s/4][s%4]) color_to_use_SPS = plot_colors[2];
      repetition_hists_SPS[s/4][s%4]->SetLineColor(color_to_use_SPS);
      repetition_hists_SPS[s/4][s%4]->SetFillColorAlpha(color_to_use_SPS, 0.25);
      repetition_hists_SPS[s/4][s%4]->SetMarkerColor(color_to_use_SPS);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetNdivisions(203);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetNdivisions(204);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetRangeUser(0, ylim);
      
      // fill the single test histograms
      for (int i = 0; i < tray_indices.size(); ++i) {
        if (tray_indices[i] == -1) continue; // flag for bad tray, in case I need it later
        repetition_hists_IV[s/4][s%4]->Fill(gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius));
        repetition_hists_SPS[s/4][s%4]->Fill(gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius));
      }
      
      // Fill the global histograms with residual/stdev data
      double stdev[2] = {0,0};
      // only use SiPMs with all ok measurements for consistency
      if (!has_failed_measurements_IV[s/4][s%4] &&
          !has_failed_measurements_SPS[s/4][s%4]) {
        for (int i = 0; i < tray_indices.size(); ++i) {
          if (tray_indices[i] == -1) continue; // flag for bad tray, in case I need it later
          // Fill global residual histograms
          double dev_this_meas_IV = gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius) - avg_this_sipm_IV;
          double dev_this_meas_SPS = gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius) - avg_this_sipm_SPS;
          
          gHist_rep_residual[0]->Fill(dev_this_meas_IV*1000);
          gHist_rep_residual[1]->Fill(dev_this_meas_SPS*1000);
          
          stdev[0] += dev_this_meas_IV*dev_this_meas_IV;   // IV
          stdev[1] += dev_this_meas_SPS*dev_this_meas_SPS; // SPS
        }// End of Histogram fill
        
        // Fill global stdev histogram
        gHist_rep_stdev[0]->Fill(std::sqrt(stdev[0])*1000); // IV
        gHist_rep_stdev[1]->Fill(std::sqrt(stdev[1])*1000); // SPS
//        std::cout << "SPS error in cassette index " << s << " after computing : " << std::sqrt(stdev[1])*1000 << " mV" << std::endl;
        
        // Add stdev to error counter--for taking average later
        gRepError_IV[global_flag_run_at_25_celcius] += std::sqrt(stdev[0]); // IV
        gRepError_SPS[global_flag_run_at_25_celcius] += std::sqrt(stdev[1]); // SPS
        
        // Fitting to gaussian with LogLikelihood (best for low count hists)
        
        // TODO
      }// End of check for good SiPM tests
      
    }// End of cassette loop
    
    // *------- Visual plot elements
    
    
    TLine* avg_line = new TLine();
    avg_line->SetLineColorAlpha(kBlack, 0.5);
    
    TLine* dev_line = new TLine();
    dev_line->SetLineColorAlpha(kGray+1, 1);
    dev_line->SetLineStyle(7);
    
    TBox* forbidden_range_box = new TBox();
    forbidden_range_box->SetFillColorAlpha(kRed+2, 0.25);
    
    
    // *------- IV Plots
    
    for (int s = 0; s < 32; ++s) {
      cassette_pads[3-s%4][7-s/4]->cd();
      gPad->SetTicks(1,1);
      
      // Add extra padding to the canvases to split them from flush if desired
      if (flag_padded) {
        double extra_padding = 0.0185;
        gPad->SetLeftMargin(gPad->GetLeftMargin() + extra_padding);
        gPad->SetTopMargin(gPad->GetTopMargin() + extra_padding);
        gPad->SetRightMargin(gPad->GetRightMargin() + extra_padding);
        gPad->SetBottomMargin(gPad->GetBottomMargin() + extra_padding);
        if (s == 31) flag_padded = false;
      }
      
      // Ensure all pads have the same tick/text sizes
      double aspect_vert = (1 - gPad->GetTopMargin() - gPad->GetBottomMargin());
      double aspect_horiz = (1 - gPad->GetLeftMargin() - gPad->GetRightMargin());
      double aspect_ratio = aspect_vert / aspect_horiz;
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetTickLength(0.06 * aspect_ratio);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetTickLength(0.06 / aspect_ratio);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetLabelSize(0.08 * aspect_horiz);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetLabelOffset(0.02 / aspect_horiz/aspect_horiz);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetTitleSize(0.09 * aspect_horiz);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetTitleOffset(1 / aspect_horiz);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetLabelSize(0.08 * aspect_vert);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetLabelOffset(0.02 / aspect_vert);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetLabelOffset(0.02 / aspect_vert);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetTitleSize(0.09 * aspect_vert);
      
      // Draw the hist and helpful visual features
      repetition_hists_IV[s/4][s%4]->Draw("hist");
      avg_line->DrawLine(avg_this_tray_IV, 0, avg_this_tray_IV, ylim);
      
      forbidden_range_box->DrawBox(avg_this_tray_IV + volthist_range[0], 0, avg_this_tray_IV - 0.05, ylim);
      forbidden_range_box->DrawBox(avg_this_tray_IV + 0.05, 0, avg_this_tray_IV + volthist_range[1], ylim);
      
      dev_line->DrawLine(avg_this_tray_IV+0.05, 0, avg_this_tray_IV+0.05, ylim);
      dev_line->DrawLine(avg_this_tray_IV-0.05, 0, avg_this_tray_IV-0.05,  ylim);
      
      // Label this SiPM
      std::pair<int, int> sipm_tray_index = gReader->GetTrayIndexFromTestIndex(r, s);
      drawText(Form("(%i,%i)",sipm_tray_index.first, sipm_tray_index.second),
               gPad->GetLeftMargin() + 0.2*aspect_horiz, gPad->GetBottomMargin() + 0.86*aspect_vert,
               false, kBlack, 0.1*std::sqrt(aspect_horiz*aspect_vert));
    }
    
    // Draw some text giving info on the setup
    gCanvas_cassetteplot->cd();
    TLatex* top_tex[6];
    top_tex[0] = drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}",                 0.025, 0.91, false, kBlack, 0.04);
    top_tex[1] = drawText("#bf{ePIC} Test Stand",                                      0.025, 0.955, false, kBlack, 0.045);
    top_tex[2] = drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code),              0.995, 0.95, true, kBlack, 0.045);
    top_tex[3] = drawText(Form("%s", string_tempcorr[global_flag_run_at_25_celcius]),  0.995, 0.905, true, kBlack, 0.035);
    top_tex[4] = drawText("IV Reproducibility",                                        0.42, 0.95, false, kBlack, 0.045);
    top_tex[5] = drawText(Form("Tray #bf{%s}: (Set %i)#times#color[2]{%i}",
                               base_tray_id.c_str(), r,total_trays),                   0.40, 0.905, false, kBlack, 0.035);
    
    
    gCanvas_cassetteplot->SaveAs(Form("../plots/systematic_plots/reproducibility%s/%s-set%i-rep%lu-IV.pdf",
                                      string_tempcorr_short[global_flag_run_at_25_celcius],base_tray_id.c_str(), r, tray_indices.size()));
    
    // *------- SPS Plots
    
    for (int s = 0; s < 32; ++s) {
      cassette_pads[3-s%4][7-s/4]->cd();
      gPad->SetTicks(1,1);
      // Ensure all pads have the same tick sizes
      double aspect_vert = (1 - gPad->GetTopMargin() - gPad->GetBottomMargin());
      double aspect_horiz = (1 - gPad->GetLeftMargin() - gPad->GetRightMargin());
      double aspect_ratio = aspect_vert / aspect_horiz;
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetTickLength(0.06 * aspect_ratio);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetTickLength(0.06 / aspect_ratio);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetLabelSize(0.08 * aspect_horiz);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetLabelOffset(0.02 / aspect_horiz/aspect_horiz);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetTitleSize(0.09 * aspect_horiz);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetTitleOffset(1 / aspect_horiz);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetLabelSize(0.08 * aspect_vert);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetLabelOffset(0.02 / aspect_vert);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetLabelOffset(0.02 / aspect_vert);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetTitleSize(0.09 * aspect_vert);
      repetition_hists_SPS[s/4][s%4]->Draw("hist");
      avg_line->DrawLine(avg_this_tray_SPS, 0, avg_this_tray_SPS, ylim);
      
      forbidden_range_box->DrawBox(avg_this_tray_SPS + volthist_range[0], 0, avg_this_tray_SPS - 0.05, ylim);
      forbidden_range_box->DrawBox(avg_this_tray_SPS + 0.05, 0, avg_this_tray_SPS + volthist_range[1], ylim);
      
      dev_line->DrawLine(avg_this_tray_SPS+0.05, 0, avg_this_tray_SPS+0.05, ylim);
      dev_line->DrawLine(avg_this_tray_SPS-0.05, 0, avg_this_tray_SPS-0.05,  ylim);
      
      std::pair<int, int> sipm_tray_index = gReader->GetTrayIndexFromTestIndex(r, s);
      drawText(Form("(%i,%i)",sipm_tray_index.first, sipm_tray_index.second),
               gPad->GetLeftMargin() + 0.2*aspect_horiz, gPad->GetBottomMargin() + 0.86*aspect_vert,
               false, kBlack, 0.1*std::sqrt(aspect_horiz*aspect_vert));
    }
    
    // Correct the IV text to SPS
    gCanvas_cassetteplot->cd();
    top_tex[4]->Clear();
    top_tex[4] = drawText("SPS Reproducibility", 0.415, 0.95, false, kBlack, 0.045);
    
    gCanvas_cassetteplot->SaveAs(Form("../plots/systematic_plots/reproducibility%s/%s-set%i-rep%lu-SPS.pdf",
                                      string_tempcorr_short[global_flag_run_at_25_celcius],base_tray_id.c_str(), r, tray_indices.size()));
    
    // Clear latex for next run
    for (int iTex = 0; iTex < 6; ++iTex) top_tex[iTex]->Clear();
    
    // Clear repetition hists
    for (int s = 0; s < 32; ++s) {
      delete repetition_hists_IV[s/4][s%4];
      delete repetition_hists_SPS[s/4][s%4];
    }
  }// End of repetition/measurement set loop
  return;
}// End of systematic_analysis_summary::makeReproducabilityHist

//========================================================================== Temperature Systematics



// Analyse the data from the special temperature scan systematic
// This was a special one time test from when the lab was overheated
// Several measurements of the same 4 SiPMs were taken as the lab cooled
// enabling a scan over temperature which would not otherwise be possible
// in our setup. 
//
// This method assumes that the contiguous string "tempscan"
// is in the run notes/batch strings and only includes such data.
void makeTemperatureScan() {
  
  
  
  
}// End of systematic_analysis_summary::makeTemperatureScan

//========================================================================== Operating Voltage V_op Systematics



// Analyse the data from operating voltage V_op scan data
// This systematic comprises of a set of SiPMs
//
// This method assumes that the contiguous string "vopscan"
// is in the run notes/batch strings and only includes such data.
void makeOperatingVoltageScan() {
  // Find strings with vopscan
  std::vector<int> vop_tray_indices;
  std::vector<float> vop_tray_voltage;
  std::vector<std::string> vop_tray_strings;
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    if (gReader->GetTrayStrings()->at(i_tray).find("vopscan") != -1) {
      std::string swapstring = gReader->GetTrayStrings()->at(i_tray);
      vop_tray_indices.push_back(i_tray);
      vop_tray_strings.push_back(swapstring);
      vop_tray_voltage.push_back(42. + 0.01 * std::stoi(swapstring.substr(swapstring.size() - 2, swapstring.size())) );
      
      std::cout << "Good v_op scan tray found at index " << t_blu << i_tray << t_def;
      std::cout << " (" << t_grn << gReader->GetTrayStrings()->at(i_tray) << t_def;
      std::cout << ") with operating voltage " << t_red << vop_tray_voltage.back() << t_def << "." << std::endl;
    }
  }
  
  // Check that vopscan values were found, return if not
  if (vop_tray_indices.size() == 0) {
    std::cout << "Warning in systematic_analysis_summary::makeOperatingVoltageScan: No trays with \"vopscan\" found in dataset." << std::endl;
    std::cout << "Check input batch file to verify V_op scan data are available." << std::endl;
    return;
  }
  
  // Gather relevant data from gReader
  std::vector<int> sipm_row;
  std::vector<int> sipm_col;
  // Data
  std::vector<std::vector<float> > Vbr_IV;
  std::vector<std::vector<float> > Vbr_25_IV;
  std::vector<std::vector<float> > Vbr_SPS;
  std::vector<std::vector<float> > Vbr_25_SPS;
  // Error (folded in from other systematics)
//  std::vector<std::vector<float> > Vbr_IV_err;
//  std::vector<std::vector<float> > Vbr_25_IV_err;
//  std::vector<std::vector<float> > Vbr_SPS_err;
//  std::vector<std::vector<float> > Vbr_25_SPS_err;
  for (int i_vop = 0; i_vop < vop_tray_indices.size(); ++i_vop) {
    std::cout << "current tray :: " << gReader->GetIV()->at(vop_tray_indices[i_vop])->tray_note << std::endl;
    
    std::vector<int>* current_sipm_row = gReader->GetIV()->at(vop_tray_indices[i_vop])->row;
    std::vector<int>* current_sipm_col = gReader->GetIV()->at(vop_tray_indices[i_vop])->col;
    std::vector<float>* current_Vbr_IV = gReader->GetIV()->at(vop_tray_indices[i_vop])->IV_Vpeak;
    std::vector<float>* current_Vbr_25_IV = gReader->GetIV()->at(vop_tray_indices[i_vop])->IV_Vpeak_25C;
    std::vector<float>* current_Vbr_SPS = gReader->GetSPS()->at(vop_tray_indices[i_vop])->SPS_Vbd;
    std::vector<float>* current_Vbr_25_SPS = gReader->GetSPS()->at(vop_tray_indices[i_vop])->SPS_Vbd_25C;
    for (int i_sipm = 0; i_sipm < current_Vbr_IV->size(); ++i_sipm) {
      if (current_Vbr_IV->at(i_sipm) == -999) continue;
      std::cout << "IV V_br for SiPM " << i_sipm << " = " << current_Vbr_IV->at(i_sipm) << std::endl;
      
      // Append data so that each vector is one SiPM--for converting to TGraph later
      if (i_vop == 0) {
        sipm_row.push_back(current_sipm_row->at(i_sipm));
        sipm_col.push_back(current_sipm_col->at(i_sipm));
        Vbr_IV.push_back(std::vector<float>());
        Vbr_25_IV.push_back(std::vector<float>());
        Vbr_SPS.push_back(std::vector<float>());
        Vbr_25_SPS.push_back(std::vector<float>());
        std::cout << "push back new SiPM...(" << sipm_row.back() << ',' << sipm_col.back() << ")." << std::endl;
      }
      
      Vbr_IV[i_sipm].push_back(current_Vbr_IV->at(i_sipm));
      Vbr_25_IV[i_sipm].push_back(current_Vbr_25_IV->at(i_sipm));
      Vbr_SPS[i_sipm].push_back(current_Vbr_SPS->at(i_sipm));
      Vbr_25_SPS[i_sipm].push_back(current_Vbr_25_SPS->at(i_sipm));
    }// End of SiPM loop
  }// End of V_op scan file loop
  
  
  // Make TGraphs of data over the V_op scan
  char data_plot_option[5] = "pl";
  const int ntotal_scan = Vbr_IV[2].size();
  const int ntotal_sipm = Vbr_IV.size();
  std::cout << "ntotal_scan = " << ntotal_scan << std::endl;
  std::cout << "ntotal_sipm = " << ntotal_sipm << std::endl;
  TGraphErrors* sipm_vopscan_graph_IV[ntotal_sipm];
  TGraphErrors* sipm_vopscan_graph_IV_25[ntotal_sipm];
  TGraphErrors* sipm_vopscan_graph_SPS[ntotal_sipm];
  TGraphErrors* sipm_vopscan_graph_SPS_25[ntotal_sipm];
  TMultiGraph* sipm_vopscan_multigraph[ntotal_sipm];
  for (int i_sipm = 0; i_sipm < ntotal_sipm; ++i_sipm) {
    // Prepare the TGraph objects
    sipm_vopscan_multigraph[i_sipm] = new TMultiGraph();
    
    sipm_vopscan_graph_IV[i_sipm] = new TGraphErrors(ntotal_scan, 
                                                     vop_tray_voltage.data(), Vbr_IV[i_sipm].data());
    sipm_vopscan_graph_IV[i_sipm]->SetLineColor(plot_colors[0]);
    sipm_vopscan_graph_IV[i_sipm]->SetMarkerColor(plot_colors[0]);
    sipm_vopscan_graph_IV[i_sipm]->SetMarkerStyle(53);
    sipm_vopscan_graph_IV[i_sipm]->SetMarkerSize(1.5);
    sipm_vopscan_multigraph[i_sipm]->Add(sipm_vopscan_graph_IV[i_sipm], data_plot_option);
    
    sipm_vopscan_graph_IV_25[i_sipm] = new TGraphErrors(ntotal_scan, vop_tray_voltage.data(), Vbr_25_IV[i_sipm].data());
    sipm_vopscan_graph_IV_25[i_sipm]->SetLineColor(plot_colors_alt[0]);
    sipm_vopscan_graph_IV_25[i_sipm]->SetMarkerColor(plot_colors_alt[0]);
    sipm_vopscan_graph_IV_25[i_sipm]->SetMarkerStyle(20);
    sipm_vopscan_graph_IV_25[i_sipm]->SetMarkerSize(1.5);
    sipm_vopscan_multigraph[i_sipm]->Add(sipm_vopscan_graph_IV_25[i_sipm], data_plot_option);
    
    sipm_vopscan_graph_SPS[i_sipm] = new TGraphErrors(ntotal_scan, vop_tray_voltage.data(), Vbr_SPS[i_sipm].data());
    sipm_vopscan_graph_SPS[i_sipm]->SetLineColor(plot_colors[1]);
    sipm_vopscan_graph_SPS[i_sipm]->SetMarkerColor(plot_colors[1]);
    sipm_vopscan_graph_SPS[i_sipm]->SetMarkerStyle(54);
    sipm_vopscan_graph_SPS[i_sipm]->SetMarkerSize(1.5);
    sipm_vopscan_multigraph[i_sipm]->Add(sipm_vopscan_graph_SPS[i_sipm], data_plot_option);
    
    sipm_vopscan_graph_SPS_25[i_sipm] = new TGraphErrors(ntotal_scan, vop_tray_voltage.data(), Vbr_25_SPS[i_sipm].data());
    sipm_vopscan_graph_SPS_25[i_sipm]->SetLineColor(plot_colors_alt[1]);
    sipm_vopscan_graph_SPS_25[i_sipm]->SetMarkerColor(plot_colors_alt[1]);
    sipm_vopscan_graph_SPS_25[i_sipm]->SetMarkerStyle(21);
    sipm_vopscan_graph_SPS_25[i_sipm]->SetMarkerSize(1.5);
    sipm_vopscan_multigraph[i_sipm]->Add(sipm_vopscan_graph_SPS_25[i_sipm], data_plot_option);
    
    
    // Prepare the canvas
    gCanvas_solo->cd();
    gCanvas_solo->Clear();
    gPad->SetTicks(1,1);
    gPad->SetRightMargin(0.015);
    gPad->SetLeftMargin(0.09);
    
    // Plot the graphs--base layer
    sipm_vopscan_multigraph[i_sipm]->SetTitle(";Operating Voltage V_{op} [V];Measured V_{br} [V]");
    sipm_vopscan_multigraph[i_sipm]->GetYaxis()->SetRangeUser(voltplot_limits[0],voltplot_limits[1]);
    sipm_vopscan_multigraph[i_sipm]->GetYaxis()->SetTitleOffset(1.2);
    sipm_vopscan_multigraph[i_sipm]->GetXaxis()->SetTitleOffset(1.2);
    sipm_vopscan_multigraph[i_sipm]->Draw("a");
    
    // Add reference lines
    TLine* typ_line = new TLine();
    typ_line->SetLineStyle(8);
    typ_line->SetLineColor(kGray+1);
    typ_line->DrawLine(42.4, voltplot_limits[0], 42.4, voltplot_limits[1]);
    
    // Add legends
    TLegend* leg_data = new TLegend(0.15, 0.39, 0.45, 0.61);
    leg_data->SetLineWidth(0);
    leg_data->AddEntry(sipm_vopscan_graph_IV[i_sipm], "IV (ambient)", data_plot_option);
    leg_data->AddEntry(sipm_vopscan_graph_IV_25[i_sipm], "IV (25#circC)", data_plot_option);
    leg_data->AddEntry(sipm_vopscan_graph_SPS[i_sipm], "SPS (ambient)", data_plot_option);
    leg_data->AddEntry(sipm_vopscan_graph_SPS_25[i_sipm], "SPS (25#circC)", data_plot_option);
    leg_data->Draw();
    
    // Draw some informative text about the setup
    TLatex* top_tex[6];
    top_tex[0] = drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}",                 gPad->GetLeftMargin(), 0.91, false, kBlack, 0.04);
    top_tex[1] = drawText("#bf{ePIC} Test Stand",                                      gPad->GetLeftMargin(), 0.955, false, kBlack, 0.045);
    top_tex[2] = drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code),              1-gPad->GetRightMargin(), 0.95, true, kBlack, 0.045);
    top_tex[3] = drawText(Form("Tray %s SiPM (%i,%i)",
                               gReader->GetIV()->at(vop_tray_indices[0])->tray_note.substr(0,11).c_str(),
                               sipm_row[i_sipm],sipm_col[i_sipm]),                     1-gPad->GetRightMargin(), 0.91, true, kBlack, 0.035);
    top_tex[4] = drawText("Test Stand Systematics: V_{op}",                            gPad->GetLeftMargin() + 0.05, 0.83, false, kBlack, 0.04);
    
    // Redraw the graph::assure plots sit on top of all other features
//    sipm_vopscan_multigraph[i_sipm]->Draw("a same");
    
    gCanvas_solo->SaveAs(Form("../plots/systematic_plots/operating_voltage/vopscan_%i_%i_Vbr.pdf",
                               sipm_row[i_sipm],sipm_col[i_sipm]));
  }// End of SiPM plot construction loop
  return;
}// End of systematic_analysis_summary::makeOperatingVoltageScan
