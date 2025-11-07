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
TH1D* gHist_rep_residual[2];    // Residuals for reproducibility comparisons among SiPMs
TH1D* gHist_rep_stdev[2];       // Stdev of SiPM repeated test distributions in a histogram

// Plot limit controls
double voltplot_limits[2] = {37.6, 38.6};
double volthist_range[2] = {-0.06, 0.06};
double darkcurr_limits[2] = {0, 35};


// TODO refine color pallette
Int_t plot_colors[3] = {
  kViolet+1, kOrange+1, kRed+2
};

// TODO make plotter class??? Or at least consider it...

//========================================================================== Forward declarations

//Reproducability
void initializeGlobalReproducabilityHists();
void makeReproducabilityHist(std::string base_tray_id);
void drawGlobalReproducabilityHists();

//========================================================================== Macro Main

// Main macro method: generate SiPM data
void systematic_analysis_summary() {
  // Read in trays to treat as current batch
  SiPMDataReader* reader = new SiPMDataReader();
  reader->ReadFile("../batch_data_syst.txt");

  // Read IV and SPS data
  reader->ReadDataIV();
  reader->ReadDataSPS();
  
  // *-- Analysis setup
  
  // Initialize global hists
  initializeGlobalReproducabilityHists();
  
  // Initialize canvases
  gCanvas_solo = new TCanvas();
  gStyle->SetOptStat(0);
  
  
  gCanvas_cassetteplot = new TCanvas();
  gCanvas_cassetteplot->SetCanvasSize(1500,830);
  cassette_pad = buildPad("cassette_pad", 0, 0, 1, 0.75/0.83);
  cassette_pad->cd();
  cassette_pads = divideFlush(gPad, 8, 4, 0.025, 0.005, 0.05, 0.01);
  
  // *-- Analysis tasks:
  
  // Make plots from reproducibility tests
  makeReproducabilityHist("250821-1302");
  makeReproducabilityHist("250821-1303");
  
  // Make composite plots with data from all repeated tests
  drawGlobalReproducabilityHists();
  
}// End of systematic_analysis_summary::main



//========================================================================== Reproducibility tests

// Initialize the global bin hists for reproducibility tests
// These keep track of residuals and reproducibility test stdev
// throughout all trays and available data
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



// Draw and save the histograms for residuals/stdev of reproducibility tests
void drawGlobalReproducabilityHists() {
  
  gCanvas_solo->cd();
  gPad->SetRightMargin(0.04);
  gPad->SetLeftMargin(0.09);
  gPad->SetTicks(1,1);
  gHist_rep_residual[0]->GetXaxis()->SetTitleOffset(1.1);
  gHist_rep_residual[0]->Draw("hist");
  gHist_rep_residual[1]->Draw("hist same");
  
  TLatex* top_tex[6];
  top_tex[0] = drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}",                 gPad->GetLeftMargin(), 0.91, false, kBlack, 0.04);
  top_tex[1] = drawText("#bf{ePIC} Test Stand",                                      gPad->GetLeftMargin(), 0.955, false, kBlack, 0.045);
  top_tex[2] = drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code),              1.-gPad->GetRightMargin(), 0.95, true, kBlack, 0.045);
  top_tex[3] = drawText(Form("%s", string_tempcorr[global_flag_run_at_25_celcius]),  1.-gPad->GetRightMargin(), 0.91, true, kBlack, 0.035);
  top_tex[4] = drawText(Form("%i total SiPMs",
                             static_cast<int>(gHist_rep_stdev[0]->GetEntries())), 0.9, 0.83, true, kBlack, 0.035);
  top_tex[5] = drawText(Form("Each tested %i times",
                             static_cast<int>(gHist_rep_residual[0]->GetEntries()/gHist_rep_stdev[0]->GetEntries())), 0.9, 0.78, true, kBlack, 0.035);
  
  TLegend* vbd_legend = new TLegend(0.14, 0.6, 0.4, 0.85);
  vbd_legend->SetLineWidth(0);
  vbd_legend->AddEntry(gHist_rep_residual[0], "V_{bd} from IV curve", "f");
  vbd_legend->AddEntry(gHist_rep_residual[1], "V_{bd} from SPS", "f");
  vbd_legend->Draw();
  
  gCanvas_solo->SaveAs(Form("../plots/systematic_plots/reproducibility%s/batch_reproducibility_residual.pdf",
                            string_tempcorr_short[global_flag_run_at_25_celcius]));
  
  gHist_rep_stdev[0]->GetXaxis()->SetTitleOffset(1.1);
  gHist_rep_stdev[0]->Draw("hist");
  gHist_rep_stdev[1]->Draw("hist same");
  
  vbd_legend = new TLegend(0.64, 0.4, 0.9, 0.65);
  vbd_legend->SetLineWidth(0);
  vbd_legend->AddEntry(gHist_rep_residual[0], "V_{bd} from IV curve", "f");
  vbd_legend->AddEntry(gHist_rep_residual[1], "V_{bd} from SPS", "f");
  vbd_legend->Draw();
  
  for (int iTex = 0; iTex < 6; ++iTex) top_tex[iTex]->Draw();
  
  gCanvas_solo->SaveAs(Form("../plots/systematic_plots/reproducibility%s/batch_reproducibility_stdev.pdf",
                       string_tempcorr_short[global_flag_run_at_25_celcius]));
  
  
}// End of systematic_analysis_summary::drawGlobalReproducabilityHist



// Compose histograms of SiPMs from repeated tests where data is available
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
      std::cout << "Repeated sets: {";
      if (repeated.size() == 0) { // set repetitions
        for (int i = 0; i < 15; ++i) {
          if (gReader->HasSet(count_index, i)) repeated.push_back(1); // data available
          else                                 repeated.push_back(0); // no data
          if (repeated.back()) std::cout << i << ' ';
        }std::cout << "}" << std::endl;
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
        std::cout << t_grn << "same!" << t_def << '}' << std::endl;
        
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
        avg_this_sipm_IV += gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius);
        avg_this_sipm_SPS += gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius);
        if (s == 0) total_trays += 1;
      }
      avg_this_sipm_IV /= total_trays;
      avg_this_sipm_SPS /= total_trays;
      ylim = total_trays + 1.5;
      
      // IV histogram
      repetition_hists_IV[s/4][s%4] = new TH1D(Form("hist_IV_Vbr_set%i_(%i,%i)",r,(r*32 + s)/23,(r*32 + s)%23),
                                               ";V_{br} [V];Counts", 12,
                                               avg_this_tray_IV + volthist_range[0],
                                               avg_this_tray_IV + volthist_range[1]);
      repetition_hists_IV[s/4][s%4]->SetLineColor(plot_colors[0]);
      repetition_hists_IV[s/4][s%4]->SetFillColorAlpha(plot_colors[0], 0.25);
      repetition_hists_IV[s/4][s%4]->SetMarkerColor(plot_colors[0]);
      repetition_hists_IV[s/4][s%4]->GetXaxis()->SetNdivisions(203);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetNdivisions(204);
      repetition_hists_IV[s/4][s%4]->GetYaxis()->SetRangeUser(0, ylim);
      
      // SPS histogram
      repetition_hists_SPS[s/4][s%4] = new TH1D(Form("hist_SPS_Vbr_set%i_(%i,%i)",r,(r*32 + s)/23,(r*32 + s)%23),
                                                ";V_{br} [V];Counts", 12,
                                                avg_this_tray_SPS + volthist_range[0],
                                                avg_this_tray_SPS + volthist_range[1]);
      repetition_hists_SPS[s/4][s%4]->SetLineColor(plot_colors[1]);
      repetition_hists_SPS[s/4][s%4]->SetFillColorAlpha(plot_colors[1], 0.25);
      repetition_hists_SPS[s/4][s%4]->SetMarkerColor(plot_colors[0]);
      repetition_hists_SPS[s/4][s%4]->GetXaxis()->SetNdivisions(203);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetNdivisions(204);
      repetition_hists_SPS[s/4][s%4]->GetYaxis()->SetRangeUser(0, ylim);
      
      // fill the histograms
      double stdev[2] = {0,0};
      for (int i = 0; i < tray_indices.size(); ++i) {
        if (tray_indices[i] == -1) continue; // flag for bad tray, in case I need it later
        repetition_hists_IV[s/4][s%4]->Fill(gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius));
        repetition_hists_SPS[s/4][s%4]->Fill(gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius));
        
        // Fill global residual histograms
        double dev_this_meas_IV = gReader->GetVbdTestIndexIV(tray_indices[i], r, s, global_flag_run_at_25_celcius) - avg_this_sipm_IV;
        double dev_this_meas_SPS = gReader->GetVbdTestIndexSPS(tray_indices[i], r, s, global_flag_run_at_25_celcius) - avg_this_sipm_SPS;
        gHist_rep_residual[0]->Fill(dev_this_meas_IV*1000);
        gHist_rep_residual[1]->Fill(dev_this_meas_SPS*1000);
        stdev[0] += dev_this_meas_IV*dev_this_meas_IV;
        stdev[1] += dev_this_meas_SPS*dev_this_meas_SPS;
      }// End of Histogram fill
      
      // Fill global stdev histogram
      gHist_rep_stdev[0]->Fill(std::sqrt(stdev[0])*1000);
      gHist_rep_stdev[1]->Fill(std::sqrt(stdev[1])*1000);
      
      // Fitting to gaussian with LogLikelihood (best for low count hists)
      
      
      // TODO
      
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
  }// End of repetition loop
  
  // Draw stacked residual and stdev histograms
  
  return;
}// End of systematic_analysis_summary::makeReproducabilityHist

