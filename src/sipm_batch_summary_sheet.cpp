//  *--
//  sipm_batch_summary_sheet.cpp
//  
//  Produces a summary sheet of relevant
//  plots to summarize a tray of SiPMs tested
//  with the Debrecen designed testing setup
//  stationed at Yale.
//
//  Created by Ryan Hamilton on 10/19/25.
//  *--

#include "global_vars.hpp"
#include "SiPMDataReader.hpp"
#include "sipm_analysis_helper.hpp"

//========================================================================== Global Variables

// flags to control some options in analysis
bool flag_use_all_trays_for_averages = false;       // Use all available trays' data to compute averages (Recommended ONLY when all trays are similar)
// TODO make class variable, I was silly and didn't think I would use this as much as I do...

// Some helpful label strings
const char string_tempcorr[2][50] = {"#color[2]{#bf{NOT}} Temperature corrected to 25C","Temperature corrected to 25C"};
const char string_tempcorr_short[2][10] = {"","_25C"};

// Global plot objects
TCanvas* gCanvas_solo;

// Plot limit controls
double voltplot_limits[2] = {37.6, 38.6};
double darkcurr_limits[2] = {0, 35};


// TODO refine color pallette
Int_t plot_colors[3] = {
  kViolet+1, kOrange+1, kRed+2
};

// TODO make plotter class??? Or at least consider it...

//========================================================================== Forward declarations

// V_Breakdown and V_peak analysis
void makeHist_IV_Vpeak(bool flag_run_at_25_celcius = true);
void makeHist_SPS_Vbreakdown(bool flag_run_at_25_celcius = true);
void makeIndexSeries(bool flag_run_at_25_celcius = true);
void makeIndexedTray(bool flag_run_at_25_celcius);
void makeIndexCassette(bool flag_run_at_25_celcius = true);
void makeTrayMapVpeak(bool flag_run_at_25_celcius = true);
void makeTrayMapVbreakdown(bool flag_run_at_25_celcius = true);
void makeTestMapVpeak(bool flag_run_at_25_celcius = true);
void makeTestMapVbreakdown(bool flag_run_at_25_celcius = true);

// Dark current analysis
void makeHist_DarkCurrent();

//========================================================================== Macro Main

// Main macro method: generate SiPM data
void sipm_batch_summary_sheet() {
  // Read in trays to treat as current batch
  SiPMDataReader* reader = new SiPMDataReader();

  
  // Read IV and SPS data
  reader->ReadDataIV();
  reader->ReadDataSPS();
  
  // Initialize canvases
  gCanvas_solo = new TCanvas();
  gStyle->SetOptStat(0);
  
  
  // Test averaging methods
//  int tray_index = 0;
//  std::cout << "Average V_peak for tray " << gReader->GetTrayStrings()->at(tray_index) << " \t\t:: " << getAvgVpeak(tray_index, false) << std::endl;
//  std::cout << "Average V_peak (25C) for tray " << gReader->GetTrayStrings()->at(tray_index) << " \t:: " << getAvgVpeak(tray_index, true) << std::endl;
//  std::cout << "Average V_bd for tray " << gReader->GetTrayStrings()->at(tray_index) << " \t\t:: " << getAvgVbreakdown(tray_index, false) << std::endl;
//  std::cout << "Average V_bd (25C) for tray " << gReader->GetTrayStrings()->at(tray_index) << " \t:: " << getAvgVbreakdown(tray_index, true);
//  std::cout << " (" << t_red << countOutliersVbreakdown(tray_index, true) << t_def << " Outliers beyond tray avg +/-" << declare_Vbd_outlier_range << "V)" << std::endl;
//  
//  std::cout << "For all trays ::" << std::endl;
//  std::cout << "Average V_peak for all trays \t\t\t:: " << getAvgVpeakAllTrays(false) << std::endl;
//  std::cout << "Average V_peak for all trays (25C) \t\t:: " << getAvgVpeakAllTrays(true) << std::endl;
//  std::cout << "Average V_bd for all trays \t\t\t:: " << getAvgVbreakdownAllTrays(false) << std::endl;
//  std::cout << "Average V_bd for all trays (25C) \t\t:: " << getAvgVbreakdownAllTrays(true) << std::endl;
  
  int n_trays = gReader->GetTrayStrings()->size();
  for (int i_tray = 0; i_tray < n_trays; ++i_tray) {
    std::cout << "Average V_bd (25C) for tray " << gReader->GetTrayStrings()->at(i_tray) << " \t:: " << getAvgVbreakdown(i_tray, true);
    std::cout << " (" << t_red << countOutliersVbreakdown(i_tray, true) << t_def << " Outliers beyond tray avg +/-" << declare_Vbd_outlier_range << "V)" << std::endl;
  }
  
  // Make series at ambient temp and 25C corrected
  makeIndexSeries(true);
  makeIndexSeries(false);
  
  makeHist_DarkCurrent();
  
  // Make 2D mappings to invesitage strange deviations
  makeTrayMapVpeak();
  makeTrayMapVbreakdown();
  makeTestMapVpeak();
  makeTestMapVbreakdown();
  
  makeTrayMapVpeak(false);
  makeTrayMapVbreakdown(false);
  makeTestMapVpeak(false);
  makeTestMapVbreakdown(false);
  
//  gReader->WriteCompressedFile(0);
//  gReader->WriteCompressedFile(1);
//  gReader->WriteCompressedFile(2);
//  gReader->WriteCompressedFile(3);
  
  makeIndexedTray(true);
  
}



//========================================================================== Solo plot generating Macros: Histograms of Result Data

// Construct histograms of IV V_peak for the trays in storage
// Accesses data via global pointers in the header file
//
// TODO return TObjectArray for summary sheet
void makeHist_IV_Vpeak(bool flag_run_at_25_celcius) {
  
  // Make histograms
  
  // plot histograms
  
  //save histograms
  
  return;
}

// Construct histograms of Dark Current for the trays in storage
// Accesses data via global pointers in the header file
//
// TODO return TObjectArray for summary sheet
void makeHist_DarkCurrent() {
  const float Hamamatsu_spec_max = 20.;
  
  // TODO currently runs over all trays but would be good to check it for single trays
  
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(800, 600);
  gPad->SetLogy();
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.03);
  gPad->SetTicks(1,1);
  gPad->SetTopMargin(0.1);
  
  
  // Make histograms
  TH1D* hist_dark_current_undervoltage = new TH1D("hist_dark_current_undervoltage",
                                                  ";Dark Current I_{dark} [nA];Count of SiPMs",
                                                  2*darkcurr_limits[1], darkcurr_limits[0], darkcurr_limits[1]);
  TH1D* hist_dark_current_overvoltage = new TH1D("hist_dark_current_overvoltage",
                                                 ";Dark Current I_{dark} [nA];Count of SiPMs",
                                                 2*darkcurr_limits[1], darkcurr_limits[0], darkcurr_limits[1]);
  
  // Gather all tray data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
    for (int i_IV = 1; i_IV <= IV_size; ++i_IV) {
      hist_dark_current_undervoltage->Fill(gReader->GetIV()->at(i_tray)->Idark_3below->at(i_IV - 1));
      hist_dark_current_overvoltage->Fill(gReader->GetIV()->at(i_tray)->Idark_4above->at(i_IV - 1));
    }
  }
  
  // plot histograms
  hist_dark_current_undervoltage->SetLineColor(plot_colors[0]);
  hist_dark_current_undervoltage->SetFillColorAlpha(plot_colors[0], 0.4);
  hist_dark_current_overvoltage->SetLineColor(plot_colors[1]);
  hist_dark_current_overvoltage->SetFillColorAlpha(plot_colors[1], 0.4);
  
  
  double range_Idark_plot[2] = {0.5, hist_dark_current_undervoltage->GetBinContent(1) * 3.75};
  hist_dark_current_undervoltage->GetYaxis()->SetRangeUser(range_Idark_plot[0], range_Idark_plot[1]);
  hist_dark_current_undervoltage->GetXaxis()->SetTitleOffset(1.2);
  
  hist_dark_current_undervoltage->Draw("hist");
  hist_dark_current_overvoltage->Draw("hist same");
  
  // Draw lines representing spec sheet limits
  TLine* contract_line = new TLine();
  contract_line->SetLineColor(kBlack);
  contract_line->DrawLine(Hamamatsu_spec_max, range_Idark_plot[0],
                          Hamamatsu_spec_max, range_Idark_plot[1]);
  
  float total_margin_horizontal = gPad->GetRightMargin() + gPad->GetLeftMargin();
  float specmax_position = gPad->GetLeftMargin() + (1-total_margin_horizontal)*(Hamamatsu_spec_max / darkcurr_limits[1]) + 0.005;
  drawText("Hamamatsu Spec Maximum at V_{^{op}}", specmax_position, 0.85, false, kBlack, 0.03)->SetTextAngle(270);
  
  // Legend for the two histograms
  TLegend* dark_current_legend = new TLegend(0.2, 0.68, 0.5, 0.85);
  dark_current_legend->SetLineWidth(0);
  dark_current_legend->AddEntry(hist_dark_current_undervoltage, "I_{dark} at V = (V_{br} #minus 3)", "f");
  dark_current_legend->AddEntry(hist_dark_current_overvoltage, "I_{dark} at V = (V_{br} + 4)", "f");
  dark_current_legend->Draw();
  
  // Add a note for which SiPM trays are included in the data
  float hamamatsu_tray_xpos = specmax_position + 0.06;
  float hamamatsu_tray_ypos = 1 - gPad->GetTopMargin() - 0.06;
  drawText(Form("Hamamatsu #bf{%s}",Hamamatsu_SiPM_Code), 0.97, hamamatsu_tray_ypos+0.075, true, kBlack, 0.035);
  drawText("Data SiPM Tray IDs:", hamamatsu_tray_xpos-0.02, hamamatsu_tray_ypos, false, kBlack, 0.034);
  for (int i_tray = 1; i_tray <= gReader->GetTrayStrings()->size(); ++i_tray) {
    if (i_tray % 2 == 0) {
      drawText(Form("%s", gReader->GetTrayStrings()->at(i_tray-1).c_str()), hamamatsu_tray_xpos + 0.14, hamamatsu_tray_ypos - 0.03*std::floor((i_tray+1)/2), false, kBlack, 0.03);
    } else {
      drawText(Form("%s", gReader->GetTrayStrings()->at(i_tray-1).c_str()), hamamatsu_tray_xpos, hamamatsu_tray_ypos - 0.03*std::floor((i_tray+1)/2), false, kBlack, 0.03);
    }
  }
  
  // Mark note counts over spec maxiumum
  float overint_ypos = hamamatsu_tray_ypos - 0.03*std::floor((gReader->GetTrayStrings()->size()+1)/2) - 0.08;
  drawText("SiPMs over spec max", hamamatsu_tray_xpos-0.02, overint_ypos, false, kBlack, 0.035);
  drawText(Form("(%.1f nA at V_{br} + 4): ",Hamamatsu_spec_max), hamamatsu_tray_xpos-0.02, overint_ypos-0.04, false, kBlack, 0.035);
  int count_overmax = countDarkCurrentOverLimitAllTrays(Hamamatsu_spec_max);
  int count_total = countSiPMsAllTrays();
  float percent_overmax = 100. * count_overmax / count_total;
  drawText(Form("#color[2]{#bf{%i}} of %i (#color[2]{#bf{%.1f}}%%)",count_overmax, count_total, percent_overmax),
           hamamatsu_tray_xpos-0.02, overint_ypos - 0.1, false, kBlack, 0.035);
  
  
  // Draw some text giving info on the setup
  drawText("#bf{ePIC} Test Stand", 0.1, 0.957, false, kBlack, 0.04);
  drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.1, 0.915, false, kBlack, 0.04);
  
  //save histograms
  gCanvas_solo->SaveAs("../plots/single_plots/IV_scan/dark_current_histograms.pdf");
  
  return;
}// End of sipm_batch_summary_sheet::makeHist_DarkCurrent

//========================================================================== Solo plot generating Macros: Indexed series



// Construct a scatter plot of V_peak and V_bd vs SiPM index for each SiPM tray
// This enables one to clearly see systematic trends/compare outliers over testing time
//
// TODO return TObjectArray for summary sheet
void makeIndexSeries(bool flag_run_at_25_celcius) {
  
  // TODO handle SPS/IV not same number of trays
  
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(1500, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetRightMargin(0.02);
  gPad->SetLeftMargin(0.06);
  gPad->SetTopMargin(0.11);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
    int SPS_size = gReader->GetSPS()->at(i_tray)->SPS_Vbd->size();
    if (IV_size != SPS_size) {
      std::cout << "Warning in <sipm_batch_summary_sheet::makeIndexSeries>: SPS and IV arrays are unequal size!" << std::endl;
      std::cout << "All data will be plotted and indices will be assumed regularly correlated, take caution that this is handled correctly." << std::endl;
    }
    
    // Make histograms
    TH1F* hist_indexed_Vpeak = new TH1F("hist_indexed_Vpeak", 
                                        ";SiPM index [flattened];V_{br} [V]",
                                        IV_size, 0, IV_size);
    TH1F* hist_indexed_Vbreakdown = new TH1F("hist_indexed_Vbreakdown",
                                            ";SiPM index [flattened];V_{br} [V]",
                                            SPS_size, 0, SPS_size);
    
    // Append all data
    if (flag_run_at_25_celcius) {
      for (int i_IV = 1; i_IV <= IV_size; ++i_IV) {
//        if (gReader->GetIV()->at(i_tray)->IV_Vpeak_25C->at(i_IV) == 0) continue;
        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak_25C->at(i_IV - 1));
      }
      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS) {
//        if (gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1) == 0) continue;
        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
      }
    } else {
      for (int i_IV = 1; i_IV <= IV_size; ++i_IV) {
//        if (gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV - 1) == 0) continue;
        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV - 1));
      }
      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS) {
//        if (gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1) == 0) continue;
        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
      }
    }
    
    // plot histograms
    gCanvas_solo->cd();
    hist_indexed_Vpeak->GetYaxis()->SetRangeUser(voltplot_limits[0], voltplot_limits[1]);
    hist_indexed_Vpeak->GetYaxis()->SetTitleOffset(0.85);
    hist_indexed_Vpeak->SetMarkerColor(plot_colors[0]);
    hist_indexed_Vpeak->SetMarkerStyle(20);
    hist_indexed_Vpeak->Draw("hist p");
    
    hist_indexed_Vbreakdown->SetMarkerColor(plot_colors[1]);
    hist_indexed_Vbreakdown->SetMarkerStyle(21);
    
    // Draw reference averaged +/- 50 MV lines
    
    double avg_voltages[2];
    if (flag_use_all_trays_for_averages) {
      avg_voltages[0] = getAvgVpeakAllTrays(flag_run_at_25_celcius); //IV
      avg_voltages[1] = getAvgVbreakdownAllTrays(flag_run_at_25_celcius); //SPS
    } else {
      avg_voltages[0] = getAvgVpeak(i_tray, flag_run_at_25_celcius); //IV
      avg_voltages[1] = getAvgVbreakdown(i_tray, flag_run_at_25_celcius); //SPS
    }
    
    // TObjects for drawing
    TLine* avg_line = new TLine();
    
    // Average line: V_peak (IV)
    avg_line->SetLineColor(kBlack);
    avg_line->DrawLine(0, avg_voltages[0], IV_size, avg_voltages[0]);
    avg_line->SetLineColor(kGray+2);
    avg_line->SetLineStyle(7);
    avg_line->DrawLine(0, avg_voltages[0]+0.05, IV_size, avg_voltages[0]+0.05);
    avg_line->DrawLine(0, avg_voltages[0]-0.05, IV_size, avg_voltages[0]-0.05);
    
    // Average line: V_breakdown (SPS)
    avg_line->SetLineStyle(1);
    avg_line->SetLineColor(kBlack);
    avg_line->DrawLine(0, avg_voltages[1], IV_size, avg_voltages[1]);
    avg_line->SetLineColor(kGray+2);
    avg_line->SetLineStyle(7);
    avg_line->DrawLine(0, avg_voltages[1]+0.05, IV_size, avg_voltages[1]+0.05);
    avg_line->DrawLine(0, avg_voltages[1]-0.05, IV_size, avg_voltages[1]-0.05);
    
    // Cassette test lines
    TLine* cassette_line = new TLine();
    cassette_line->SetLineColor(kGray+1);
    cassette_line->SetLineStyle(6);
    for (int i = 1; i <= 14; ++i) cassette_line->DrawLine(32*i, voltplot_limits[0], 32*i, voltplot_limits[1]);
    
    // assure points sit on top of lines
    hist_indexed_Vpeak->Draw("hist p same");
    hist_indexed_Vbreakdown->Draw("hist p same");
    
    // Legend for labeling the two V_breakdown measurement types
    float leg_extra_space = 0;
    if (flag_run_at_25_celcius) leg_extra_space = 0.04;
    TLegend* vbd_legend = new TLegend(0.635, 0.36 + leg_extra_space, 0.90, 0.51 + leg_extra_space);
    vbd_legend->SetLineWidth(0);
    vbd_legend->AddEntry(hist_indexed_Vpeak, "IV V_{bd} (also called V_{peak})", "p");
    vbd_legend->AddEntry(hist_indexed_Vbreakdown, "SPS V_{bd}", "p");
    vbd_legend->Draw();
    
    // Legend for the lines marking tray average, test sets
    TLegend* line_legend = new TLegend(0.15, 0.315 + leg_extra_space, 0.45, 0.55 + leg_extra_space);
    line_legend->SetLineWidth(0);
    hist_indexed_Vpeak->SetLineColor(kBlack);
    if (flag_use_all_trays_for_averages) line_legend->AddEntry(hist_indexed_Vpeak, "Average over all trays", "l");
    else                                 line_legend->AddEntry(hist_indexed_Vpeak, "Average over tray", "l");
    line_legend->AddEntry(avg_line, "Average #pm 50mV", "l");
    line_legend->AddEntry(cassette_line, "Test Runs (32 SiPM per test)", "l");
    line_legend->Draw();
    
    
    // Draw some text giving info on the setup
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.06, 0.91, false, kBlack, 0.045);
    drawText("#bf{ePIC} Test Stand", 0.06, 0.955, false, kBlack, 0.045);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 0.98, 0.95, true, kBlack, 0.05);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.04);
    
    
    //save histograms
    gCanvas_solo->SaveAs(Form("../plots/single_plots/indexed%s/%s_indexed_Vbd%s.pdf",
                              string_tempcorr_short[flag_run_at_25_celcius],
                              gReader->GetTrayStrings()->at(i_tray).c_str(),
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    delete hist_indexed_Vpeak;
    delete hist_indexed_Vbreakdown;
  }
  
  return;
}// End of sipm_batch_summary_sheet::makeIndexSeries



// Make brief indexed tray V_breakdown measurement summary
// This will show the average V_brakdown from IV and SPS measurements
// From each tray with errors representing the spread of data for each tray
//
// TODO return TObjectArray for summary sheet
void makeIndexedTray(bool flag_run_at_25_celcius) {
  
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(750, 600); // TODO size of canvas varies with number of trays
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetRightMargin(0.03);
  gPad->SetLeftMargin(0.11);
  gPad->SetTopMargin(0.11);
  
  // Histograms
  const int n_trays = gReader->GetIV()->size();
  TH1F* hist_indexed_Vpeak_tray = new TH1F("hist_indexed_Vpeak_tray",
                                           ";Hamamatsu Tray Number;V_{br} [V]",
                                           n_trays, 0, n_trays);
  TH1F* hist_indexed_Vbreakdown_tray = new TH1F("hist_indexed_Vbreakdown",
                                                ";Hamamatsu Tray Number;V_{br} [V]",
                                                n_trays, 0, n_trays);
  
  // Gather avg data and add to histogram
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    hist_indexed_Vpeak_tray->GetXaxis()->SetBinLabel(i_tray + 1, gReader->GetTrayStrings()->at(i_tray).c_str());
    
    hist_indexed_Vpeak_tray->SetBinContent(i_tray + 1, getAvgVpeak(i_tray, flag_run_at_25_celcius));
    hist_indexed_Vpeak_tray->SetBinError(i_tray + 1, getStdevVpeak(i_tray, flag_run_at_25_celcius));

    hist_indexed_Vbreakdown_tray->SetBinContent(i_tray + 1, getAvgVbreakdown(i_tray, flag_run_at_25_celcius));
    hist_indexed_Vbreakdown_tray->SetBinError(i_tray + 1, getStdevVbreakdown(i_tray, flag_run_at_25_celcius));
  }
  
  // plot histograms
  gCanvas_solo->cd();
  hist_indexed_Vpeak_tray->GetYaxis()->SetRangeUser(voltplot_limits[0], voltplot_limits[1]);
  hist_indexed_Vpeak_tray->GetYaxis()->SetTitleOffset(1.35);
  hist_indexed_Vpeak_tray->SetLineColor(plot_colors[0]);
  hist_indexed_Vpeak_tray->SetLineWidth(2);
  hist_indexed_Vpeak_tray->SetFillColorAlpha(plot_colors[0],0);
  hist_indexed_Vpeak_tray->SetMarkerColor(plot_colors[0]);
  hist_indexed_Vpeak_tray->SetMarkerStyle(20);
  hist_indexed_Vpeak_tray->SetMarkerSize(2);
  hist_indexed_Vpeak_tray->Draw("b p e1 x0");
  
  hist_indexed_Vbreakdown_tray->SetLineColor(plot_colors[1]);
  hist_indexed_Vbreakdown_tray->SetLineWidth(2);
  hist_indexed_Vbreakdown_tray->SetFillColorAlpha(plot_colors[1],0);
  hist_indexed_Vbreakdown_tray->SetMarkerColor(plot_colors[1]);
  hist_indexed_Vbreakdown_tray->SetMarkerStyle(21);
  hist_indexed_Vbreakdown_tray->SetMarkerSize(2);
  
  
  // Draw reference averaged +/- 50 MV lines, average over all trays
  
  double avg_voltages[2];
  avg_voltages[0] = getAvgVpeakAllTrays(flag_run_at_25_celcius); //IV
  avg_voltages[1] = getAvgVbreakdownAllTrays(flag_run_at_25_celcius); //SPS
  
  // TObjects for drawing
  TLine* avg_line = new TLine();
  TLine* dev_line = new TLine();
  
  // Average line: V_peak (IV)
  avg_line->SetLineColor(kBlack);
  avg_line->DrawLine(0, avg_voltages[0], n_trays, avg_voltages[0]);
  dev_line->SetLineColor(kGray+2);
  dev_line->SetLineStyle(7);
  dev_line->DrawLine(0, avg_voltages[0]+0.05, n_trays, avg_voltages[0]+0.05);
  dev_line->DrawLine(0, avg_voltages[0]-0.05, n_trays, avg_voltages[0]-0.05);
  
  // Average line: V_breakdown (SPS)
  avg_line->DrawLine(0, avg_voltages[1], n_trays, avg_voltages[1]);
  dev_line->DrawLine(0, avg_voltages[1]+0.05, n_trays, avg_voltages[1]+0.05);
  dev_line->DrawLine(0, avg_voltages[1]-0.05, n_trays, avg_voltages[1]-0.05);
  
  // SiPM batch delimeter lines (if relevant) TODO make dynamic (include tray storter in SiPMDataReader)
  TLine* batch_line = new TLine();
  batch_line->SetLineColor(kGray+1);
  batch_line->SetLineStyle(6);
  batch_line->DrawLine(4, voltplot_limits[0], 4, voltplot_limits[1]);
  
  // assure points sit on top of lines
  hist_indexed_Vpeak_tray->Draw("b p e1 x0 same");
  hist_indexed_Vbreakdown_tray->Draw("b p e1 x0 same");
  
  // Legend for labeling the two V_breakdown measurement types
  float leg_extra_space = 0;
  if (flag_run_at_25_celcius) leg_extra_space = 0.04;
  TLegend* vbd_legend = new TLegend(0.625, 0.36 + leg_extra_space, 0.94, 0.51 + leg_extra_space);
  vbd_legend->SetLineWidth(0);
  vbd_legend->AddEntry(hist_indexed_Vpeak_tray, "IV V_{bd} (also called V_{peak})", "p");
  vbd_legend->AddEntry(hist_indexed_Vbreakdown_tray, "SPS V_{bd}", "p");
  vbd_legend->Draw();
  
  // Draw some text giving info on the setup
  drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.06, 0.91, false, kBlack, 0.04);
  drawText("#bf{ePIC} Test Stand", 0.06, 0.955, false, kBlack, 0.045);
  drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code), 0.98, 0.95, true, kBlack, 0.045);
  drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.035);
  
  // Draw some text about the batches
  drawText(Form("Batch %s", gReader->GetTrayStrings()->at(0).substr(0,6).c_str()), 0.155, 0.81, false, kBlack, 0.04);
  drawText(Form("Batch %s (ORNL)", gReader->GetTrayStrings()->at(4).substr(0,6).c_str()), 0.63, 0.81, false, kBlack, 0.04);
  
  
  // Legend for the lines marking tray average, test sets
  TLegend* line_legend = new TLegend(0.15, 0.315 + leg_extra_space, 0.5, 0.55 + leg_extra_space);
  line_legend->SetLineWidth(0);
  line_legend->AddEntry(avg_line, Form("Average all trays #left[#splitline{IV      %.2f}{SPS  %.2f}#right]",avg_voltages[0],avg_voltages[1]), "l");
  line_legend->AddEntry(dev_line, "Average #pm 50mV", "l");
  line_legend->AddEntry(batch_line, "Batch Delimeter", "l");
  line_legend->Draw();
  
  gCanvas_solo->SaveAs(Form("../plots/batch_plots/batch_Vbr_trayavg%s.pdf",string_tempcorr_short[flag_run_at_25_celcius]));
  
  delete hist_indexed_Vpeak_tray;
  delete hist_indexed_Vbreakdown_tray;
  
}// End of sipm_batch_summary_sheet::makeIndexedTray



// Construct a scatter plot of V_peak and V_bd vs SiPM cassette index (during testing)
// This enables one to clearly see systematic trends/compare trends during a single test
//
// TODO return TObjectArray for summary sheet
void makeIndexCassette(bool flag_run_at_25_celcius) {
  
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(750, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetRightMargin(0.03);
  gPad->SetTopMargin(0.11);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
    int SPS_size = gReader->GetSPS()->at(i_tray)->SPS_Vbd->size();
    
    // Make histograms
    TH1F* hist_indexed_Vpeak_cassette = new TH1F("hist_indexed_Vpeak_cassette",
                                                 ";SiPM index [flattened];V_{br} [V]",
                                                 32, 0, 32);
    TH1F* hist_indexed_Vbreakdown_cassette = new TH1F("hist_indexed_Vbreakdown",
                                                      ";SiPM index [flattened];V_{br} [V]",
                                                      32, 0, 32);
    
    // Append all data
//    if (flag_run_at_25_celcius) {
//      for (int i_IV = 1; i_IV <= IV_size; ++i_IV)
//        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak_25C->at(i_IV - 1));
//      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS)
//        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
//    } else {
//      for (int i_IV = 1; i_IV <= IV_size; ++i_IV)
//        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV - 1));
//      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS)
//        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
//    }
//    
//    // plot histograms
//    gCanvas_solo->cd();
//    hist_indexed_Vpeak->GetYaxis()->SetRangeUser(voltplot_limits[0], voltplot_limits[1]);
//    hist_indexed_Vpeak->SetMarkerColor(plot_colors[0]);
//    hist_indexed_Vpeak->SetMarkerStyle(20);
//    hist_indexed_Vpeak->Draw("hist p");
//    
//    hist_indexed_Vbreakdown->SetMarkerColor(plot_colors[1]);
//    hist_indexed_Vbreakdown->SetMarkerStyle(21);
//    
//    // Draw reference averaged +/- 50 MV lines
//    
//    double avg_voltages[2];
//    if (flag_use_all_trays_for_averages) {
//      avg_voltages[0] = getAvgVpeakAllTrays(flag_run_at_25_celcius); //IV
//      avg_voltages[1] = getAvgVbreakdownAllTrays(flag_run_at_25_celcius); //SPS
//    } else {
//      avg_voltages[0] = getAvgVpeak(i_tray, flag_run_at_25_celcius); //IV
//      avg_voltages[1] = getAvgVbreakdown(i_tray, flag_run_at_25_celcius); //SPS
//    }
//    
//    // TObjects for drawing
//    TLine* avg_line = new TLine();
//    
//    // Average line: V_peak (IV)
//    avg_line->SetLineColor(kBlack);
//    avg_line->DrawLine(0, avg_voltages[0], IV_size, avg_voltages[0]);
//    avg_line->SetLineColor(kGray+2);
//    avg_line->SetLineStyle(7);
//    avg_line->DrawLine(0, avg_voltages[0]+0.05, IV_size, avg_voltages[0]+0.05);
//    avg_line->DrawLine(0, avg_voltages[0]-0.05, IV_size, avg_voltages[0]-0.05);
//    
//    // Average line: V_breakdown (SPS)
//    avg_line->SetLineStyle(1);
//    avg_line->SetLineColor(kBlack);
//    avg_line->DrawLine(0, avg_voltages[1], IV_size, avg_voltages[1]);
//    avg_line->SetLineColor(kGray+2);
//    avg_line->SetLineStyle(7);
//    avg_line->DrawLine(0, avg_voltages[1]+0.05, IV_size, avg_voltages[1]+0.05);
//    avg_line->DrawLine(0, avg_voltages[1]-0.05, IV_size, avg_voltages[1]-0.05);
//    
//    // Cassette test lines
//    TLine* cassette_line = new TLine();
//    cassette_line->SetLineColor(kGray+1);
//    cassette_line->SetLineStyle(6);
//    for (int i = 1; i <= 14; ++i) cassette_line->DrawLine(32*i, voltplot_limits[0], 32*i, voltplot_limits[1]);
//    
//    // assure points sit on top of lines
//    hist_indexed_Vpeak->Draw("hist p same");
//    hist_indexed_Vbreakdown->Draw("hist p same");
//    
//    // Legend for labeling the two plots
//    TLegend* vbd_legend = new TLegend(0.635, 0.4, 0.93, 0.55);
//    vbd_legend->SetLineWidth(0);
//    vbd_legend->AddEntry(hist_indexed_Vpeak, "IV V_{bd} (also called V_{peak})", "p");
//    vbd_legend->AddEntry(hist_indexed_Vbreakdown, "SPS V_{bd}", "p");
//    vbd_legend->Draw();
//    
//    // Draw some text giving info on the setup
//    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.1, 0.92, false, kBlack, 0.05);
//    drawText(Form("Hamamatsu Tray #%s", gReader->GetTrayStrings()->at(i_tray).c_str()), 0.97, 0.94, true, kBlack, 0.05);
//    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.97, 0.90, true, kBlack, 0.04);
//    
//    
//    //save histogramschar string_tempcorr[2][50]
//    gCanvas_solo->SaveAs(Form("../plots/single_plots/indexed%s/%s_indexed_Vbd%s.pdf",
//                              string_tempcorr_short[flag_run_at_25_celcius],
//                              gReader->GetTrayStrings()->at(i_tray).c_str(),
//                              string_tempcorr_short[flag_run_at_25_celcius]));
//    
//    delete hist_indexed_Vpeak;
//    delete hist_indexed_Vbreakdown;
  }
  
  return;
}// End of sipm_batch_summary_sheet::makeIndexCassette



//========================================================================== Solo plot generating Macros: SiPM Tray/Test Mappings



// Map the IV V_peak results to the tray poisitons in a 2D grid
// Helpful for checking if strange trends are manufacturing flaws or
// systematic/statistical errors in the testing procedure
void makeTrayMapVpeak(bool flag_run_at_25_celcius = true) {
  
  gStyle->SetPalette(kSunset);
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(750, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetLogy(0);
  gPad->SetRightMargin(0.17);
  gPad->SetLeftMargin(0.085);
  gPad->SetTopMargin(0.11);
  gPad->SetBottomMargin(0.095);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    // Get averages for z axis range
    double avg_voltage = getAvgVpeak(i_tray, flag_run_at_25_celcius);
    
    // Make map histogram
    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
    TH2F* map_tray_Vpeak = new TH2F("map_tray_Vpeak",
                                    ";SiPM Tray Column;SiPM Tray Row;Deviation from Tray Avg. #color[2]{#bf{IV}} V_{br} [V]",
                                    NCOL, 0, NCOL, NROW, 0, NROW);
    for (int i_IV = 0; i_IV < IV_size; ++i_IV) {
      map_tray_Vpeak->Fill(gReader->GetIV()->at(i_tray)->col->at(i_IV),
                           gReader->GetIV()->at(i_tray)->row->at(i_IV),
                           gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV) - avg_voltage);
    }
    
    // Plot the map
    map_tray_Vpeak->GetZaxis()->SetRangeUser(-0.16, 0.16);
    map_tray_Vpeak->GetZaxis()->SetTitleOffset(1.7);
    map_tray_Vpeak->GetYaxis()->SetTitleOffset(0.86);
    map_tray_Vpeak->Draw("colz");
    
    // Draw some text giving info on the setup
    drawText("#bf{ePIC} Test Stand", gPad->GetLeftMargin(), 0.95, false, kBlack, 0.035);
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", gPad->GetLeftMargin(), 0.903, false, kBlack, 0.035);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 0.99, 0.955, true, kBlack, 0.035);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.99, 0.915, true, kBlack, 0.03);
    
    // Save the map
    gCanvas_solo->SaveAs(Form("../plots/single_plots/mapped_tray%s/%s_traymap_IV_Vbr%s.pdf",
                              string_tempcorr_short[flag_run_at_25_celcius],
                              gReader->GetTrayStrings()->at(i_tray).c_str(),
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    delete map_tray_Vpeak;
  }
  return;
}// End of sipm_batch_summary_sheet::makeTrayMapVpeak



// Map the SPS V_breakdown results to the tray poisitons in a 2D grid
// Helpful for checking if strange trends are manufacturing flaws or
// systematic/statistical errors in the testing procedure
void makeTrayMapVbreakdown(bool flag_run_at_25_celcius = true) {
  
  gStyle->SetPalette(kSunset);
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(750, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetLogy(0);
  gPad->SetRightMargin(0.17);
  gPad->SetLeftMargin(0.085);
  gPad->SetTopMargin(0.11);
  gPad->SetBottomMargin(0.095);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    // Get averages for z axis range
    double avg_voltage = getAvgVbreakdown(i_tray, flag_run_at_25_celcius);
    
    // Make map histogram
    int SPS_size = gReader->GetSPS()->at(i_tray)->SPS_Vbd->size();
    TH2F* map_tray_Vbreakdown = new TH2F("map_tray_Vbreakdown",
                                         ";SiPM Tray Column;SiPM Tray Row;Deviation from Tray Avg. #color[2]{#bf{SPS}} V_{br} [V]",
                                         NCOL, 0, NCOL, NROW, 0, NROW);
    for (int i_SPS = 0; i_SPS < SPS_size; ++i_SPS) {
      map_tray_Vbreakdown->Fill(gReader->GetSPS()->at(i_tray)->col->at(i_SPS),
                                gReader->GetSPS()->at(i_tray)->row->at(i_SPS),
                                gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS) - avg_voltage);
    }
    
    // Plot the map
    map_tray_Vbreakdown->GetZaxis()->SetRangeUser(-0.16, 0.16);
    map_tray_Vbreakdown->GetZaxis()->SetTitleOffset(1.7);
    map_tray_Vbreakdown->GetYaxis()->SetTitleOffset(0.86);
    map_tray_Vbreakdown->Draw("colz");
    
    // Draw some text giving info on the setup
    drawText("#bf{ePIC} Test Stand", gPad->GetLeftMargin(), 0.95, false, kBlack, 0.035);
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", gPad->GetLeftMargin(), 0.903, false, kBlack, 0.035);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 0.99, 0.955, true, kBlack, 0.035);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.99, 0.915, true, kBlack, 0.03);
    
    // Save the map
    gCanvas_solo->SaveAs(Form("../plots/single_plots/mapped_tray%s/%s_traymap_SPS_Vbr%s.pdf",
                              string_tempcorr_short[flag_run_at_25_celcius],
                              gReader->GetTrayStrings()->at(i_tray).c_str(),
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    delete map_tray_Vbreakdown;
  }
  return;
  
}// End of sipm_batch_summary_sheet::makeTrayMapVbreakdown



// Map the IV V_peak results to the cassette test poisitons in a 2D grid
// Helpful for checking if strange trends are manufacturing flaws or
// systematic/statistical errors in the testing procedure
void makeTestMapVpeak(bool flag_run_at_25_celcius = true) {
  
  gStyle->SetPalette(kSunset);
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(1200, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetLogy(0);
  gPad->SetRightMargin(0.13);
  gPad->SetLeftMargin(0.05);
  gPad->SetBottomMargin(0.08);
  gPad->SetTopMargin(0.11);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    // Get averages for z axis range
    double avg_voltage = getAvgVpeak(i_tray, flag_run_at_25_celcius);
    
    // Make map histogram
    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
    TH2F* map_test_Vpeak = new TH2F("map_test_Vpeak",
                                    ";Cassette Index;IV Test Set;Deviation from Tray Avg. #color[2]{#bf{IV}} V_{br} [V]",
                                    32, 0, 32, 15, 0, 15);
    for (int i_IV = 0; i_IV < IV_size; ++i_IV) {
      map_test_Vpeak->Fill(i_IV % 32, i_IV / 32,
                           gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV) - avg_voltage);
    }for (int i_fill = IV_size; i_fill < 32*15; ++i_fill)
      map_test_Vpeak->SetBinContent(i_fill % 32 + 1, i_fill / 32 + 1, -1);
    
    // Plot the map
    map_test_Vpeak->GetZaxis()->SetRangeUser(-0.16, 0.16);
    map_test_Vpeak->GetZaxis()->SetTitleOffset(1.1);
    map_test_Vpeak->GetYaxis()->SetTitleOffset(0.6);
    map_test_Vpeak->Draw("colz");
    
    // Draw some text giving info on the setup
    drawText("#bf{ePIC} Test Stand", gPad->GetLeftMargin(), 0.95, false, kBlack, 0.045);
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", gPad->GetLeftMargin(), 0.903, false, kBlack, 0.045);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 1-gPad->GetRightMargin(), 0.955, true, kBlack, 0.045);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 1-gPad->GetRightMargin(), 0.907, true, kBlack, 0.04);
    
    // Save the map
    gCanvas_solo->SaveAs(Form("../plots/single_plots/mapped_test%s/%s_testmap_IV_Vbr%s.pdf",
                              string_tempcorr_short[flag_run_at_25_celcius],
                              gReader->GetTrayStrings()->at(i_tray).c_str(),
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    delete map_test_Vpeak;
  }
  return;
}// End of sipm_batch_summary_sheet::makeTestMapVpeak



// Map the SPS V_breakdown results to the tray poisitons in a 2D grid
// Helpful for checking if strange trends are manufacturing flaws or
// systematic/statistical errors in the testing procedure
void makeTestMapVbreakdown(bool flag_run_at_25_celcius = true) {
  
  gStyle->SetPalette(kSunset);
  gCanvas_solo->Clear();
  gCanvas_solo->SetCanvasSize(1200, 600);
  gCanvas_solo->cd();
  gPad->SetTicks(1,1);
  gPad->SetLogy(0);
  gPad->SetRightMargin(0.13);
  gPad->SetLeftMargin(0.05);
  gPad->SetBottomMargin(0.08);
  gPad->SetTopMargin(0.11);
  
  // Iterate over all available data
  for (int i_tray = 0; i_tray < gReader->GetTrayStrings()->size(); ++i_tray) {
    
    // Get averages for z axis range
    double avg_voltage = getAvgVbreakdown(i_tray, flag_run_at_25_celcius);
    
    // Make map histogram
    int SPS_size = gReader->GetSPS()->at(i_tray)->SPS_Vbd->size();
    TH2F* map_test_Vbreakdown = new TH2F("map_test_Vbreakdown",
                                         ";Cassette Index;SPS Test Set;Deviation from Tray Avg. #color[2]{#bf{SPS}} V_{br} [V]",
                                         32, 0, 32, 15, 0, 15);
    for (int i_SPS = 0; i_SPS < SPS_size; ++i_SPS) {
      map_test_Vbreakdown->Fill(i_SPS % 32, i_SPS / 32,
                                gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS) - avg_voltage);
    } for (int i_fill = SPS_size; i_fill < 32*15; ++i_fill)
      map_test_Vbreakdown->SetBinContent(i_fill % 32 + 1, i_fill / 32 + 1, -1);
    
    // Plot the map
    map_test_Vbreakdown->GetZaxis()->SetRangeUser(-0.16, 0.16);
    map_test_Vbreakdown->GetZaxis()->SetTitleOffset(1.1);
    map_test_Vbreakdown->GetYaxis()->SetTitleOffset(0.6);
    map_test_Vbreakdown->Draw("colz");
    
    // Draw some text giving info on the setup
    drawText("#bf{ePIC} Test Stand", gPad->GetLeftMargin(), 0.95, false, kBlack, 0.045);
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", gPad->GetLeftMargin(), 0.903, false, kBlack, 0.045);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 1-gPad->GetRightMargin(), 0.955, true, kBlack, 0.045);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 1-gPad->GetRightMargin(), 0.907, true, kBlack, 0.04);
    
    // Save the map
    gCanvas_solo->SaveAs(Form("../plots/single_plots/mapped_test%s/%s_testmap_SPS_Vbr%s.pdf",
                              string_tempcorr_short[flag_run_at_25_celcius],
                              gReader->GetTrayStrings()->at(i_tray).c_str(),
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    delete map_test_Vbreakdown;
  }
  return;
  
}// End of sipm_batch_summary_sheet::makeTestMapVbreakdown
