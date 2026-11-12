//  *--
//  analyze_third_derivative.cpp
//
//  Analyzes IV binary files to extract an
//  estimate for the IV breakdown voltage
//  of a SiPM from the third derivative of current
//  w.r.t. voltage, following the model desribed in ref.
//  https://arxiv.org/abs/1606.07805
//  https://doi.org/10.1016/j.nima.2017.01.002
//
// No genAI tools were used for the creation of this file.
//
//  Changelog ::
//    - 09/14/2026    : Created by Ryan Hamilton
//    - 11/03/2026    : Working method using peak fixing/parameter solver
//    - 11/05/2026    : Fully integrated with SiPMDataReader to produce main plots/residuals
//    - 11/10/2026    : Temperature correction integrated / toggle-able
//  *--

#include <filesystem>
#include "global_vars.hpp"
#include "lib_debrecen_IV/IVBinaryFile.h"
#include "SiPMDataReader.hpp"
#include "sipm_analysis_helper.hpp"

// Toggle between fit modes:
//    fit1 :: Default (as described in ref. with 3rd der from SG, no smoothing)
//    fit2 :: fixpeak (fit only to peaks, and use the extracted peaks to solve for parameters)
//    fit3 :: Perform an integration to estimate the hysteresis in a distribution-independent manner (TODO)
//               -> Saving this for a future publication/study
#define fit2

//========================================================================== Global Variables


// Switch to use DMM or SMU voltage measurement
bool use_DMM_over_SMU = true;
char voltage_type_str[2][5] = {"SMU","DMM"};
double voltplot_limits_static[2] = {30.95, 39.3};

bool flag_run_at_25_celcius = false;
double tempcoeff = 0.034; // 34 mV / C
char string_tempcorr[2][50] = {"#color[2]{#bf{NOT}} Temperature corrected to 25C","Temperature corrected to 25C"};
char string_tempcorr_short[2][10] = {"raw","tcorr"};

// Controls for the Savitzky-Golay filter
int SG_poly_order = 3;
int SG_window_oneside = 5; // Full convolution window size is 2r + 1
const int nsmooths = 1;

// Controlls for gaussian fit in fixpeak mode
int nbin_fit_range_gauss = 3;

// Debug flag
bool _debug = false;
bool _print_fit_info = false;
bool _make_solo_plots = false;


// Root global pointers
TCanvas* gCanvas_solo;
TCanvas* gCanvas_double;
std::vector<std::vector<TPad*> > cpads;

//========================================================================== Forward declarations

double* getSavitzkyGolayCoefs(int poly_order, int interp_window, int nderivs);

double* performListConvolution(double* listA, int sizeA,
                               double* listB, int sizeB);


//========================================================================== Macro Main
void analyze_third_derivative() {
  
  // ----------------------------------------------------------------- Canvas setup
  
  // Initialize canvases
  gStyle->SetOptStat(0);
  gErrorIgnoreLevel = kWarning;
  
  TObjArray* obj_list = new TObjArray();
  gCanvas_solo = new TCanvas();
  gPad->SetTicks(1,1);
  
  gCanvas_double = new TCanvas();
  gCanvas_double->SetCanvasSize(1000, 500);
  gCanvas_double->Divide(2, 1);
  
  gCanvas_double->cd(1);
  gPad->SetTicks(1,1);
  gPad->SetRightMargin(0.03);
  gPad->SetLeftMargin(0.13);
  gPad->SetBottomMargin(0.085);
  gPad->SetTopMargin(0.09);
  gCanvas_double->SetLeftMargin(gPad->GetLeftMargin() / 2);
  gCanvas_double->SetTopMargin(gPad->GetTopMargin() / 2);
  gCanvas_double->SetBottomMargin(gPad->GetBottomMargin() / 2);
  
  gCanvas_double->cd(2);
  gPad->SetTicks(1,1);
  gPad->SetRightMargin(0.03);
  gPad->SetLeftMargin(0.11);
  gPad->SetBottomMargin(0.085);
  gPad->SetTopMargin(0.09);
  gCanvas_double->SetRightMargin(gPad->GetRightMargin() / 2);
  
  
  
  // ----------------------------------------------------------------- Setup that should be done only once outside the SiPM loop
  
  // Get the convolution kernel for the desired SG filter
  int ntotal_sg_coefs = 2*SG_window_oneside + 1;
  double* sg_coefs = getSavitzkyGolayCoefs(SG_poly_order, SG_window_oneside, 3);
  std::cout << "SG convolution kernel : { ";
  for (int c = 0; c < ntotal_sg_coefs; ++c) {
    std::cout << sg_coefs[c] << ' ';
  }std::cout << "}." << std::endl;
  
  // And for smoothing, if desired
  double* sg_smooth = getSavitzkyGolayCoefs(SG_poly_order, SG_window_oneside, 0);
  std::cout << "SG smoothing kernel : { ";
  for (int c = 0; c < ntotal_sg_coefs; ++c) {
    std::cout << sg_coefs[c] << ' ';
  }std::cout << "}." << std::endl;
  
  // Set up the SiPM data reader
  const char* traylist_identifier = "production";
  SiPMDataReader* finalReader = new SiPMDataReader();
  finalReader->SetSubDirectory(traylist_identifier);
  finalReader->ReadFile(Form("../data/batch_traylist_%s.txt",traylist_identifier));
  finalReader->ReadDataIV();
  finalReader->ReadDataSPS();
  
  
  // Set up histograms for fit parameters
  int ntotal_possible[2] = {0};
  int ntotal_SiPMs[2] = {0};
  int ntotal_goodfits[2] = {0};
  int nbin_params = 100;
  TH1D* hist_params[2][4];
  TH1D* hist_hratio[2];
  TH1D* hist_hratio_failed[2];
  TH1D* hist_residuals_3derV[2];
  TH1D* hist_residuals_3derH[2];
  TH1D* hist_residuals_NullV[2];
  TH1D* hist_residuals_NullH[2];
  std::vector<double> corr_global_3derV[2];
  std::vector<double> corr_global_3derH[2];
  std::vector<double> corr_global_SPS[2];
  char string_robotmode[2][20] = {"cassette","robot"};
  int color_robotmode[2] = {kBlue + 2, kRed + 2};
  for (int i_robot = 0; i_robot < 2; ++i_robot) {
    hist_params[i_robot][0] = new TH1D(Form("hist_A_%s", string_robotmode[i_robot]), 
                                       ";A [A / V^{3}];1/N^{fit} dN^{fit}/dA [V^{3}/A]",
                              nbin_params, 0, 1.2e-6);
    hist_params[i_robot][0]->SetLineColor(color_robotmode[i_robot]);
    hist_params[i_robot][0]->SetMarkerColor(color_robotmode[i_robot]);
    
    hist_params[i_robot][1] = new TH1D(Form("hist_V_%s", string_robotmode[i_robot]), 
                                       ";V_{01} [V];1/N^{fit} dN^{fit}/dV [1/V]",
                              nbin_params, 37.5, 39.5);
    hist_params[i_robot][1]->SetLineColor(color_robotmode[i_robot]);
    hist_params[i_robot][1]->SetMarkerColor(color_robotmode[i_robot]);
    
    hist_params[i_robot][2] = new TH1D(Form("hist_s_%s", string_robotmode[i_robot]), 
                                       ";#sigma [V];1/N^{fit} dN^{fit}/d#sigma [1/V]",
                              nbin_params, 0.105, 0.16);
    hist_params[i_robot][2]->SetLineColor(color_robotmode[i_robot]);
    hist_params[i_robot][2]->SetMarkerColor(color_robotmode[i_robot]);
    
    // For h, make a log-normal scale instead
    double range_h[2] = {1e-1, 1e5};
    std::vector<double> vector_log_binedge = std::vector<double>();
    double logbinedge[2] = {TMath::Log(range_h[0]), TMath::Log(range_h[1])};
    double delta_log_binedge = (logbinedge[1] - logbinedge[0]) / nbin_params;
    for (int i = 0; i <= nbin_params; ++i) vector_log_binedge.push_back(TMath::Exp(logbinedge[0] + i * delta_log_binedge));
    hist_params[i_robot][3] = new TH1D(Form("hist_h_%s", string_robotmode[i_robot]), 
                                       ";h [V];1/N^{fit} dN^{fit}/dh [1/V]",
                              nbin_params, vector_log_binedge.data());
    hist_params[i_robot][3]->SetLineColor(color_robotmode[i_robot]);
    hist_params[i_robot][3]->SetMarkerColor(color_robotmode[i_robot]);
    
    
    // Histogram of ratios h+/h-
    hist_hratio[i_robot] = new TH1D(Form("hist_hratio_%s",string_robotmode[i_robot]),
                                    ";h_{+}/h_{-};Counts",
                                    nbin_params, -2, 0);
    hist_hratio[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_hratio[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    
    hist_hratio_failed[i_robot] = new TH1D(Form("hist_hratio_failed_%s",string_robotmode[i_robot]),
                                    ";h_{+}/h_{-};Counts",
                                    nbin_params, -2, 0);
    hist_hratio_failed[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_hratio_failed[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    hist_hratio_failed[i_robot]->SetFillColorAlpha(color_robotmode[i_robot],0.2);
    
    
    // Histograms for residuals against SPS measurements
    hist_residuals_3derV[i_robot] = new TH1D(Form("hist_residuals_3derV_%s",string_robotmode[i_robot]),
                                             ";3^{rd} Derivative V_{br} - SPS V_{br} [V];Counts",
                                             nbin_params, 0.55, 1.2);
    hist_residuals_3derV[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_residuals_3derV[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    
    hist_residuals_3derH[i_robot] = new TH1D(Form("hist_residuals_3derH_%s",string_robotmode[i_robot]),
                                             ";3^{rd} Derivative V_{br} - SPS V_{br} [V];Counts",
                                             nbin_params, -40, 0);
    hist_residuals_3derH[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_residuals_3derH[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    
    
    // NUll hypothesis histograms
    hist_residuals_NullV[i_robot] = new TH1D(Form("hist_residuals_NullV_%s",string_robotmode[i_robot]),
                                             ";Null Residual 39.0V - SPS V_{br} [V];Counts",
                                             nbin_params, 0.55, 1.2);
    hist_residuals_NullV[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_residuals_NullV[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    hist_residuals_NullV[i_robot]->SetFillColorAlpha(color_robotmode[i_robot],0.2);
    
    hist_residuals_NullH[i_robot] = new TH1D(Form("hist_residuals_NullH_%s",string_robotmode[i_robot]),
                                             ";Null Residual 30.0V - SPS V_{br} [V];Counts",
                                             nbin_params, -40, 0);
    hist_residuals_NullH[i_robot]->SetLineColor(color_robotmode[i_robot]);
    hist_residuals_NullH[i_robot]->SetMarkerColor(color_robotmode[i_robot]);
    hist_residuals_NullH[i_robot]->SetFillColorAlpha(color_robotmode[i_robot],0.2);
    
  }// End of parameter histogram setup
  
 
  
  // ----------------------------------------------------------------- Start of SiPM loop
  
  bool drawn_text = false;
  for (int i_tray = 0; i_tray < finalReader->GetTrayStrings()->size(); ++i_tray) {
    
    // *----------------- Set up / check key directories
    std::cout << "Beginning analysis for tray number " << finalReader->GetTrayStrings()->at(i_tray) << std::endl;
    const char* trayno = finalReader->GetTrayStrings()->at(i_tray).c_str();
    
    // Check if this tray is robot or cassette data
    bool is_robot = (1 == finalReader->GetTrayModes()->at(i_tray));
    // Base relative location of IV binary files
    char bin_base_directory[100];
    snprintf(bin_base_directory, 100, "../data/IV_bin/production/%s_production",string_robotmode[is_robot]);\
    
    
    // Stat input directory for this tray
    char basedir[100];
    snprintf(basedir, 100, "%s/%s/binaries", bin_base_directory, trayno);
    struct stat check_dir;
    if (stat(basedir, &check_dir) != 0) {
      std::cout << "\033[31mError\033[39m :: Failed to find binary directory for tray {" << trayno << "}." << std::endl;
      continue;
    }
    
    // Found binary
    ntotal_possible[is_robot] += 460 - 400 * (finalReader->GetTrayStrings()->at(i_tray).compare("260409-1703") == 0);
    
    // Stat output directories for this tray
    char outdir[100];
    snprintf(outdir, 100, "../plots/3rdder/%s", trayno);
    if (stat(basedir, &check_dir) == 0) {
      std::filesystem::create_directory(outdir);
      
      char subdirs[1][100];
      snprintf(subdirs[0], 100, "%s/%s_%ismooth", outdir, string_robotmode[is_robot], nsmooths);
      for (int i = 0; i < 1; ++i) {
        std::filesystem::create_directory(subdirs[i]);
      }
      std::cout << "Successfully created directories for tray number \033[32m" << trayno << "\033[39m." << std::endl;
    }// End of out directory construction
    
    
    // Set up arrays to keep track of the estimated Vbd across the tray
    double parm_A[460];
    double parm_V[460];
    double parm_s[460];
    double parm_h[460];
    std::vector<double> corrdata_3derV;
    std::vector<double> corrdata_3derH;
    std::vector<double> corrdata_IV;
    std::vector<double> corrdata_SPS;
    
    
    // Begin SiPM loop for this tray
    for (int i_SiPM = 0; i_SiPM < 460; ++i_SiPM) {
      
      // ----------------------------------------------------------------- Read IV data from binary file
      
      // Stat the desired file to check the data exists
//      int row_index = i_SiPM / 20;
//      int col_index = i_SiPM % 20;
      int row_index = i_SiPM % 23;
      int col_index = i_SiPM / 23;
      char fulldir[200];
      snprintf(fulldir, 200, "%s/IV_%s_%i_%i.bin", basedir, trayno, row_index, col_index);
      
      struct stat check_file;
      if (stat(fulldir, &check_file) != 0 || (check_file.st_mode & S_IFDIR)) {
        if (gReader->GetTrayStrings()->at(i_tray).compare("260409-1703") != 0 || i_SiPM < 60) {
          std::cout << "\033[33mSkipping\033[39m SiPM (" << col_index << ',' << row_index << ")[" << i_SiPM;
          std::cout << "] :: No binary file found." << std::endl;
        }
        parm_A[i_SiPM] = -999;
        parm_V[i_SiPM] = -999;
        parm_s[i_SiPM] = -999;
        parm_h[i_SiPM] = -999;
        corrdata_3derV.push_back(-999);
        corrdata_3derH.push_back(-999);
        corr_global_3derV[is_robot].push_back(-999);
        corr_global_3derH[is_robot].push_back(-999);
        continue;
      }
      
      ++ntotal_SiPMs[is_robot];
      
      // Initialize the IV reader class with a sample output file
      IVBinaryFile* reader = new IVBinaryFile(fulldir);
      
      
      // Load in the values to the array
      uint32_t n_points = reader->NFineMeasurements();
      double* array_I = reader->FineCurrent();
      double* array_V;
      if (use_DMM_over_SMU) array_V = reader->FineVoltage();
      else                  array_V = reader->FineVoltageSMU();
      
      
      // Debug print
      if (_debug) {
        reader->Print();
        for (int i = 0; i < n_points; ++i) {
          std::cout << "(I,V) = {" << *(array_I + i) << ",\t" << *(array_V + i) << "}." << std::endl;
        }
      }// End of debug print
      
      // Convert current from mA to A
      // TODO understand if this is correct
      for (int i = 0; i < n_points; ++i) *(array_I+i) *= 1e-3;
      
      
      // ----------------------------------------------------------------- Computing the 3rd Derivative with SG Coefficients
      
      
      // Perform the convolution to estimate the third derivative
      double* sg_3der_result = performListConvolution(array_I, n_points, sg_coefs, ntotal_sg_coefs);
      
      // Smooth as many times as specified in the settings
      for (int s = 0; s < nsmooths; ++s) sg_3der_result = performListConvolution(sg_3der_result, n_points, sg_smooth, ntotal_sg_coefs);
      
      
      // Scale by the finite difference cubed to make it a proper derivative
      double avg_difference = 0;
      for (int i = 1; i < n_points; ++i) {
        avg_difference += *(array_V + i) - *(array_V + i - 1);
      }avg_difference /= (n_points - 1);
      
      double diff_cubed = avg_difference * avg_difference * avg_difference;
      for (int i = SG_window_oneside; i < n_points - SG_window_oneside; ++i) *(sg_3der_result+i) /= diff_cubed;
      
      
      // Find the max/min of the third derivative
      double extrema[2] = {1e10,-1e10};
      for (int i = SG_window_oneside; i < n_points - SG_window_oneside; ++i) {
        if (*(sg_3der_result+i) < extrema[0]) extrema[0] = *(sg_3der_result+i);
        if (*(sg_3der_result+i) > extrema[1]) extrema[1] = *(sg_3der_result+i);
      }
      if (_debug) std::cout << "min and max third der :: {" << extrema[0] << ',' << extrema[1] << "}" << std::endl;
      
      
      // Estimate the errors on the data
      // We assume the error on the voltage are roughly constant, see
      // https://www.testequipmenthq.com/datasheets/NATIONAL%20INSTRUMENTS-PXIE-4081-Datasheet.pdf
      double error_V[n_points];
      double error_V_const = 0.005;
      for (int i = 0; i < n_points; ++i) error_V[i] = error_V_const;
      
      // For the current, assume the error is proportional to the current,
      // Again see pg. 8 of the spec sheet in ref.
      // https://www.testequipmenthq.com/datasheets/NATIONAL%20INSTRUMENTS-PXIE-4081-Datasheet.pdf
      // For some justification of this.
      double error_I[n_points];
      double coeff_error_I = 1e-4;
      for (int i = 0; i < n_points; ++i) error_I[i] = coeff_error_I * array_I[i] / diff_cubed;
      
      
      
      // *---------------- Set up fit form, ROOT format data to fit against
      
      // Store the best parameters in the originally provided funtional form
      //   { A, V_0, sigma, h }
      double bestparams_baseform[4] = { 0 };
      TF1* func_form = new TF1("DebrecenGaussFitForm",
                               "[0] * (2 + [3] * (x - [1])/([2]**2) ) * TMath::Exp(-(x - [1])**2/(2*[2]**2))",
                               array_V[0], array_V[n_points - 1]);
      func_form->SetParNames("A","V0","#sigma","h");
      
      
      // fit lower and upper range limits
      double fitrange[2] = {array_V[10], array_V[85]};
      if (_debug) std::cout << "Fitting in range [" << fitrange[0] << ',' << fitrange[1] << ']' << std::endl;
      
      
      // Definition of the data in TGraph fomat
      TGraphErrors* graph3der = new TGraphErrors(n_points - 2*SG_window_oneside,        // Number of sample points
                                                 array_V + SG_window_oneside,           // Voltage data
                                                 sg_3der_result + SG_window_oneside,    // Current data
                                                 error_V + SG_window_oneside,           // Voltage error
                                                 error_I + SG_window_oneside);          // Current error
      
      
      //  -------------------------------------------------------------------------------------------------------- Fitmode 1 (default)
      // Attempt 1 ::
      //   With no smoothing, direct implementation of fitting form
#ifdef fit1
      
      // Initial guesses with reasonable values for the fitting
      double Ainit = 9e-13 / diff_cubed;
      double hinit = 0.7;
      double sinit = 0.11;
      double Vinit = 38.1;
      
      double Arange[2] = {1e-13 / diff_cubed, 8e-12 / diff_cubed};
      double Vrange[2] = {Vinit - 0.5, Vinit + 0.5};
      double srange[2] = {sinit - 0.05, sinit + 0.05};
      double hrange[2] = {hinit - 1, hinit + 1};
      
      //  func_form->SetParLimits(0, Arange[0], Arange[1]);
      //  func_form->SetParLimits(1, Vrange[0], Vrange[1]);
      //  func_form->SetParLimits(2, srange[0], srange[1]);
      func_form->SetParLimits(3, hrange[0], hrange[1]);
      
      
      // Generate arrays of points for fit scanning
      unsigned int npoints = 5;
      std::vector<double> bins_A;
      std::vector<double> bins_h;
      std::vector<double> bins_s;
      std::vector<double> bins_V;
      for (int i = 1; i <= npoints; ++i) {
        bins_A.push_back(Arange[0] + i * (Arange[1] - Arange[0]) / npoints);
        bins_h.push_back(hrange[0] + i * (hrange[1] - hrange[0]) / npoints);
        bins_s.push_back(srange[0] + i * (srange[1] - srange[0]) / npoints);
        bins_V.push_back(Vrange[0] + i * (Vrange[1] - Vrange[0]) / npoints);
      }
      
      
      
      
      double best_chi2 = 1e10;
      double best_params[4];
      for (int iA = 0; iA < npoints; ++iA)
        for (int ih = 0; ih < npoints; ++ih)
          for (int is = 0; is < npoints; ++is)
            for (int iV = 0; iV < npoints; ++iV) {
              func_form->SetParameters(bins_A[iA], bins_V[iV], bins_s[is], bins_h[ih]);
              
              // Notes on options:
              //   - S :: Return fit result pointer
              //   -
              // Notes on outputs
              //   - {S, SMF} works but always hits the barrier on h.
              //   - SMG fails
              //
              TFitResultPtr fit_result = graph3der->Fit(func_form, "SMQN", "",fitrange[0], fitrange[1]);
              //          TFitResultPtr fit_result = graph3der->Fit(func_form, "SMGFQ", "",fitrange[0], fitrange[1]).Get();
              if (fit_result->Chi2() <= 0) continue;
              if (std::fabs(func_form->GetParameter(0)) > 1) continue;
              
              
              if (fit_result->Chi2() < best_chi2) {
                best_chi2 = fit_result->Chi2();
                std::cout << "New best chi2 found :: " << best_chi2 << std::endl;
                for (int iPar = 0; iPar < 4; ++iPar) {
                  best_params[iPar] = func_form->GetParameter(iPar);
                }
              }
            }// End of chi2 scan fitting
      
      
      func_form->SetParameters(best_params[0], best_params[1], best_params[2], best_params[3]);
      bestparams_baseform[0] = best_params[0];
      bestparams_baseform[1] = best_params[1];
      bestparams_baseform[2] = best_params[2];
      bestparams_baseform[3] = best_params[3];
      
      //  -------------------------------------------------------------------------------------------------------- Fitmode 2 (fixed peak position solver)
#elifdef fit2
      
      // Determine the fit region for the first peak as:
      //  -> The largest point below the midpoint of the data sample,
      //     so that numerical noise in the large region does not snatch the peak away
      int fit_baseline[2] = {0};
      for (int i = SG_window_oneside; i < n_points/2 + SG_window_oneside; ++i)
        if (*(sg_3der_result+i) > *(sg_3der_result+fit_baseline[0])) fit_baseline[0] = i;
      if (_debug) {
        std::cout << "Positive peak fit baseline at point (" << *(array_V + fit_baseline[0]);
        std::cout << ',' << *(sg_3der_result+fit_baseline[0]) << ")." << std::endl;
      }
      
      // The baseline for the second peak is given as
      //  -> The smallest point within +0.5V region of the first baseline
      for (int i = fit_baseline[0]; i < fit_baseline[0] + 20; ++i) {
        if (i >= n_points) {
          std::cout << "\033[31mWarning\033[39m :: Flagged max is within 0.5V of upper limit in range!! Check fits!" << std::endl;
          return;
          // TODO wrap goto that skips to next SiPM.
        }
        if (*(sg_3der_result+i) < *(sg_3der_result+fit_baseline[1])) fit_baseline[1] = i;
      }
      if (_debug) {
        std::cout << "Negative peak fit baseline at point (" << *(array_V + fit_baseline[1]);
        std::cout << ',' << *(sg_3der_result+fit_baseline[1]) << ")." << std::endl;
      }
      
      
      // Perform fits to extract the largest peak
      TF1* peak_fit_gauss[2];
      peak_fit_gauss[0] = new TF1("gauss_fit_pos",
                                  "[0] * TMath::Exp(-(x - [1])**2/(2*[2]**2))",
                                  array_V[fit_baseline[0] - nbin_fit_range_gauss],
                                  array_V[fit_baseline[0] + nbin_fit_range_gauss]);
      peak_fit_gauss[0]->SetParameters(*(sg_3der_result+fit_baseline[0]), array_V[fit_baseline[0]], 0.1);
      graph3der->Fit(peak_fit_gauss[0], "MQN", "",
                     array_V[fit_baseline[0] - nbin_fit_range_gauss],
                     array_V[fit_baseline[0] + nbin_fit_range_gauss]);
      
      // Perform fits to extract the negative valley
      peak_fit_gauss[1] = new TF1("gauss_fit_neg",
                                  "[0] * TMath::Exp(-(x - [1])**2/(2*[2]**2))",
                                  array_V[fit_baseline[1] - nbin_fit_range_gauss],
                                  array_V[fit_baseline[1] + nbin_fit_range_gauss]);
      peak_fit_gauss[1]->SetParameters(*(sg_3der_result+fit_baseline[1]), array_V[fit_baseline[1]], 0.1);
      graph3der->Fit(peak_fit_gauss[1], "MQN", "",
                     array_V[fit_baseline[1] - nbin_fit_range_gauss],
                     array_V[fit_baseline[1] + nbin_fit_range_gauss]);
      
      // Gather and report on the peak positions
      double peak_pos[2][2] = {
        {peak_fit_gauss[0]->GetParameter(1), peak_fit_gauss[0]->GetParameter(0)},
        {peak_fit_gauss[1]->GetParameter(1), peak_fit_gauss[1]->GetParameter(0)},
      };
      if (_print_fit_info) {
        std::cout << "Extracted peak positions :: +(\033[31m";
        std::cout << peak_pos[0][0] << "\033[39m,\033[33m" << peak_pos[0][1] << "\033[39m), -(\033[31m";
        std::cout << peak_pos[1][0] << "\033[39m,\033[33m" << peak_pos[1][1] << "\033[39m)." << std::endl;
      }
      
      // Extract the ratio z = sigma / h from the ratio of peak heights function
      TF1* func_peak_ratio = new TF1("func_peak_ratio",
                                     "-(1 + 2*(x**2) * (1 + TMath::Sqrt(1 + 1/x**2) ) ) * TMath::Exp(2*(x**2) * TMath::Sqrt(1 + 1/x**2) )",
                                     0, 100);
//      TF1* func_peak_ratio = new TF1("func_peak_ratio",
//                                     "-(1 + 2/(x**2) * (1 + TMath::Sqrt(1 + x**2) ) ) * TMath::Exp(2/(x**2) * TMath::Sqrt(1 + x**2) )",
//                                     0.2, 1000);
      
      double hratio = peak_pos[0][1] / peak_pos[1][1]; // pos peak / neg peak
      if (hratio > -1.0) {// Failed fit -- outside the real domain of the inverse
        parm_A[i_SiPM] = -999;
        parm_V[i_SiPM] = -999;
        parm_s[i_SiPM] = -999;
        parm_h[i_SiPM] = -999;
        hist_hratio_failed[is_robot]->Fill(hratio);
        corrdata_3derV.push_back(-999);
        corrdata_3derH.push_back(-999);
        corr_global_3derV[is_robot].push_back(-999);
        corr_global_3derH[is_robot].push_back(-999);
//        std::cout << "3derV push back " << i_SiPM << " :: -999" << std::endl;
        continue;
      }
      
      double z_root = func_peak_ratio->GetX(hratio);
//      double z_root = 1./func_peak_ratio->GetX(hratio);
      if (_print_fit_info) {
        std::cout << "Height ratio from fit :: h+/h- = \033[31m" << hratio << "\033[39m -> ";
        std::cout << "sigma / h = \033[32m" << z_root << "\033[39m"<< std::endl;
      }
      
      // From this, all parameters of the fit form are uniquely constrained to pass through the desired points.
      //   { A, V_0, sigma, h }
      bestparams_baseform[0] = z_root * TMath::Exp(z_root * z_root) * TMath::Sqrt(- TMath::E() * peak_pos[0][1] * peak_pos[1][1]);
      bestparams_baseform[2] = (peak_pos[1][0] - peak_pos[0][0]) / (2 * TMath::Sqrt(1 + z_root * z_root));
      bestparams_baseform[3] = -bestparams_baseform[2] / z_root;
      bestparams_baseform[1] = 0.5*(peak_pos[1][0] + peak_pos[0][0]) - z_root * bestparams_baseform[2];
      func_form->SetParameters(bestparams_baseform[0], bestparams_baseform[1], bestparams_baseform[2], bestparams_baseform[3]);
      
      // record for later tray plotting
      ++ntotal_goodfits[is_robot];
      parm_A[i_SiPM] = bestparams_baseform[0];
      parm_V[i_SiPM] = bestparams_baseform[1];
      parm_s[i_SiPM] = bestparams_baseform[2];
      parm_h[i_SiPM] = -bestparams_baseform[3];
      
      // Histograms of parameters and results
      for (int p = 0; p < 3; ++p) hist_params[is_robot][p]->Fill(bestparams_baseform[p]);
      hist_params[is_robot][3]->Fill(-bestparams_baseform[3]);
      hist_hratio[is_robot]->Fill(hratio);
      
      // Correlation plots
      corrdata_3derV.push_back(bestparams_baseform[1]);
      corrdata_3derH.push_back(bestparams_baseform[1] + bestparams_baseform[3]);
      corr_global_3derV[is_robot].push_back(bestparams_baseform[1]);
      corr_global_3derH[is_robot].push_back(bestparams_baseform[1] + bestparams_baseform[3]);
      
      if (_print_fit_info) {
        std::cout << "Best parameters :: {";
        std::cout << "A = \033[33m" << bestparams_baseform[0] << "\033[39m, ";
        std::cout << "mu = \033[33m" << bestparams_baseform[1] << "\033[39m, ";
        std::cout << "sigma = \033[33m" << bestparams_baseform[2] << "\033[39m, ";
        std::cout << "h = \033[32m" << bestparams_baseform[3] << "\033[39m}." << std::endl;
      }
#endif
      // *---------------- End of fitting
      
      
      if (_make_solo_plots) {
        
        
        // ----------------------------------------------------------------- Plotting (Individual SiPM level)
        
        // Make a root plot of the IV, third derivative measurements
        gCanvas_double->cd(1);
        gPad->SetLogy();
        TGraph* graphIVdata = new TGraph(n_points, array_V, array_I);
        graphIVdata->SetName(Form("graphIV_%s_%s_%i_%i",trayno,voltage_type_str[use_DMM_over_SMU],row_index,col_index));
        graphIVdata->SetTitle(Form(";%s Voltage [V];Current [A]",voltage_type_str[use_DMM_over_SMU]));
        graphIVdata->SetLineColor(kBlue);
        graphIVdata->SetMarkerColor(kBlack);
        graphIVdata->SetMarkerStyle(6);
        graphIVdata->Draw("apc");
        obj_list->AddLast(graphIVdata);
        
        // Draw the third derivative estimate and corresponding fit
        gCanvas_double->cd(2);
        double rangemult = 2 - is_robot;
        graph3der->SetName(Form("graph3der_%s_%s_%i_%i",trayno,voltage_type_str[use_DMM_over_SMU],row_index,col_index));
        graph3der->SetTitle(Form(";%s Voltage [V];3^{rd} Derivative d^{3}I/dV^{3} [A / V^{3}]",voltage_type_str[use_DMM_over_SMU]));
        graph3der->SetLineColor(kBlack);
        graph3der->SetMarkerColor(kBlack);
        graph3der->SetMarkerStyle(6);
        graph3der->GetYaxis()->SetRangeUser(-60e-12 / diff_cubed * rangemult, 9e-11 / diff_cubed * rangemult);
        graph3der->Draw("ap");
        func_form->Draw("same");
        obj_list->AddLast(graph3der);
        
        
        
        // Draw elements specific to each fit mode
#ifdef fit1
        
        // Add lines to denote the fitting region, resulting derivatives, etc.
        TLine* range_line = new TLine();
        range_line->SetLineColor(kGray+2);
        range_line->SetLineWidth(3);
        //  range_line->SetLineStyle(7);
        gPad->Update();
        range_line->DrawLine(fitrange[0], 0.2 * gPad->GetUymin(), fitrange[0], 0.3 * gPad->GetUymax());
        range_line->DrawLine(fitrange[1], 0.2 * gPad->GetUymin(), fitrange[1], 0.3 * gPad->GetUymax());
#elifdef fit2
        TLine* peak_line = new TLine();
        peak_line->SetLineColor(kGray+2);
        peak_line->SetLineStyle(7);
        
        // Draw gaussian peak fits for each peak
        for (int i = 0; i < 2; ++i) {
          peak_fit_gauss[i]->SetLineColorAlpha(kAzure+2, 0.5);
          peak_fit_gauss[i]->SetLineStyle(1);
          peak_fit_gauss[i]->SetLineWidth(2);
          peak_fit_gauss[i]->Draw("same");
          
          peak_line->DrawLine(peak_fit_gauss[i]->GetParameter(1), 0,
                              peak_fit_gauss[i]->GetParameter(1), 1.3*peak_fit_gauss[i]->GetParameter(0));
        }// End of gauss peak drawing
#endif
        
        // Draw some text giving info on the setup
        gCanvas_double->cd();
        double pad = 0.005;
        if (!drawn_text) {
          drawText("#bf{ePIC} Test Stand", pad + gPad->GetLeftMargin(), 0.955, false, kBlack, 0.035);
          drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", pad + gPad->GetLeftMargin(), 0.915, false, kBlack, 0.035);
#ifdef fit1
          drawText("Fitmode #it{default}", 1 - pad - gPad->GetRightMargin(), 0.915, true, kBlack, 0.035);
#elifdef fit2
          drawText("Fitmode #it{fixpeak}", 1 - pad - gPad->GetRightMargin(), 0.915, true, kBlack, 0.035);
#endif
          
          
          drawn_text = true;
        }// End of single use draw
        
        // Draw parameter text, should be cleared on each iteration
        obj_list->AddLast(drawText(Form("Hamamatsu #bf{%s} [#color[2]{%s}]",
                                        Hamamatsu_SiPM_Code,
                                        string_robotmode[is_robot]),
                                   1 - pad - gPad->GetRightMargin(), 0.955, true, kBlack, 0.035));
        
        // SiPM labels
        gCanvas_double->cd(1);
        obj_list->AddLast(drawText(Form("Tray #%s",trayno), 0.05 + gPad->GetLeftMargin(), 0.92 - gPad->GetTopMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("SiPM (%i,%i)",row_index,col_index), 0.05 + gPad->GetLeftMargin(), 0.88 - gPad->GetTopMargin(), false, kBlack, 0.04));
        
        // SG filter settings
        gCanvas_double->cd(2);
        //  drawText(Form("Tray #%s",trayno), 0.95 - gPad->GetRightMargin(), 0.82 - gPad->GetTopMargin(), true, kBlack, 0.04);
        //  drawText(Form("SiPM (%i,%i)",row_index,col_index), 0.95 - gPad->GetRightMargin(), 0.78 - gPad->GetTopMargin(), true, kBlack, 0.04);
        obj_list->AddLast(drawText("SG Settings ::", 0.05 + gPad->GetLeftMargin(), 0.20 + gPad->GetBottomMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("Poly. order = %i",SG_poly_order), 0.07 + gPad->GetLeftMargin(), 0.15 + gPad->GetBottomMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("Conv. window = %i",2*SG_window_oneside+1), 0.07 + gPad->GetLeftMargin(), 0.10 + gPad->GetBottomMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("# Smooths = %i",nsmooths), 0.07 + gPad->GetLeftMargin(), 0.05 + gPad->GetBottomMargin(), false, kBlack, 0.04));
        
        // Fit results
        char latex_fit_form[200] = "#frac{d^{3}I}{dV^{3}} = A #left(2 #minus #frac{h}{#sigma^{2}} (V #minus V_{0}) #right) Exp#left[#frac{#minus(V #minus V_{0})^{2}}{2#sigma^{2}} #right]";
        obj_list->AddLast(drawText(latex_fit_form, 0.07 + gPad->GetLeftMargin(), 0.90 - gPad->GetTopMargin(), false, kBlack, 0.035));
        obj_list->AddLast(drawText(Form("A = %.4fE-6",bestparams_baseform[0]/1e-6), 0.05 + gPad->GetLeftMargin(), 0.83 - gPad->GetTopMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("h = %.4f",-bestparams_baseform[3]), 0.05 + gPad->GetLeftMargin(), 0.78 - gPad->GetTopMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("#sigma = %.4f",bestparams_baseform[2]), 0.05 + gPad->GetLeftMargin(), 0.73 - gPad->GetTopMargin(), false, kBlack, 0.04));
        obj_list->AddLast(drawText(Form("V_{0} = %.4f",bestparams_baseform[1]), 0.05 + gPad->GetLeftMargin(), 0.67 - gPad->GetTopMargin(), false, kBlack, 0.04));
#ifdef fit2
        obj_list->AddLast(drawText(Form("h_{+}/h_{-} = %.8f",hratio), 0.90 - gPad->GetRightMargin(), 0.81 - gPad->GetTopMargin(), true, kBlack, 0.04));
#endif
        
        
        // Save the canvas
        gCanvas_double->SaveAs(Form("%s/%s_%ismooth/IV_%s_3rdder_%s_%i_%i_%s.pdf",
                                    outdir,
                                    string_robotmode[is_robot],
                                    nsmooths,
                                    voltage_type_str[use_DMM_over_SMU],
                                    trayno,
                                    row_index,
                                    col_index,
                                    string_tempcorr_short[flag_run_at_25_celcius]));
      }// End of plotting
      
      // Delete dynamically initialzied objects in this loop
      delete reader;
      TObjArrayIter TObj_iter(obj_list);
      while (TNamed* data_obj = (TNamed*)TObj_iter.Next()) delete data_obj;
      obj_list->Clear();
      
//      if (i_SiPM > 20) break; // debug one sipm
    }// End of SiPM loop for this tray
    
    
    
    
    // ----------------------------------------------------------------- Compare against extracted Debrecen results
    
    
    // Testing for indexing bugs
    std::string checktray = "";
    int index_shuffle = 0;
    if (gReader->GetTrayStrings()->at(i_tray).compare(checktray) == 0) {
      std::cout << "Testing indexing for tray " << checktray << std::endl;
      index_shuffle = 1;
    }
    
    
    // Set up canvas
    gCanvas_solo->Clear();
    gCanvas_solo->SetCanvasSize(1500, 1500);
    gCanvas_solo->cd();
    gPad->SetTicks(1,1);
    gPad->SetRightMargin(0.02);
    gPad->SetLeftMargin(0.09);
    gPad->SetTopMargin(0.11);
    
      
//    int IV_size = gReader->GetIV()->at(i_tray)->IV_Vpeak->size();
//    int SPS_size = gReader->GetSPS()->at(i_tray)->SPS_Vbd->size();
    int IV_size = 460;
    int SPS_size = 460;
//    if (index_shuffle != 0) {
//      IV_size = 250;
//      SPS_size = 250;
//    }
    if (IV_size != SPS_size) {
      std::cout << "Warning in <sipm_batch_summary_sheet::makeIndexSeries>: SPS and IV arrays for tray ";
      std::cout << gReader->GetTrayStrings()->at(i_tray) << " are of unequal size!" << std::endl;
      std::cout << "All data will be plotted and indices will be assumed regularly correlated, take caution that this is handled correctly." << std::endl;
    }
    
    // Make histograms
    TH1F* hist_indexed_Vpeak = new TH1F("hist_indexed_Vpeak",
                                        ";SiPM index [flattened];V_{br} [V]",
                                        IV_size, 0, IV_size);
    TH1F* hist_indexed_3derV = new TH1F("hist_indexed_3derV",
                                        ";SiPM index [flattened];V_{br} [V]",
                                        IV_size, 0, IV_size);
    TH1F* hist_indexed_3derH = new TH1F("hist_indexed_3derH",
                                        ";SiPM index [flattened];V_{br} [V]",
                                        IV_size, 0, IV_size);
    TH1F* hist_indexed_Vbreakdown = new TH1F("hist_indexed_Vbreakdown",
                                             ";SiPM index [flattened];V_{br} [V]",
                                             SPS_size, 0, SPS_size);
    
    
    TH1F* hist_indexed_diff = new TH1F("hist_indexed_diff",
                                       ";SiPM index [flattened];V_{br} [V]",
                                       SPS_size, 0, SPS_size);
    
    
    // Append all data
    if (flag_run_at_25_celcius) {  // *-------------------------------------- Temperature correction
      for (int i_IV = 1; i_IV <= IV_size; ++i_IV) {
        double tcorr_add = tempcoeff * (25 - gReader->GetIV()->at(i_tray)->avg_temp->at(i_IV - 1));
        
        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak_25C->at(i_IV - 1));
        hist_indexed_3derV->SetBinContent(i_IV, parm_V[i_IV - 1] + tcorr_add);
        hist_indexed_3derH->SetBinContent(i_IV, parm_V[i_IV - 1] + tcorr_add - parm_h[i_IV - 1]);
        
        corrdata_IV.push_back(gReader->GetIV()->at(i_tray)->IV_Vpeak_25C->at(i_IV - 1));
      }// End of IV tabulating
      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS) {
        double tcorr_add = tempcoeff * (25 - gReader->GetSPS()->at(i_tray)->avg_temp->at(i_SPS - 1));
        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        
        corrdata_SPS.push_back(gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        corr_global_SPS[is_robot].push_back(gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        
        hist_indexed_diff->SetBinContent(i_SPS, parm_V[i_SPS - 1] + tcorr_add - gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        
        // Fill residual histograms
        if (parm_V[i_SPS - 1] < 0) continue;
        
        hist_residuals_3derV[is_robot]->Fill(parm_V[i_SPS - 1] + tcorr_add - gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        hist_residuals_NullV[is_robot]->Fill(39.0 - gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        
        
        if (parm_V[i_SPS - 1] - parm_h[i_SPS - 1] < 0 ||
            gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_SPS - 1) < 0) continue;
        
        hist_residuals_3derH[is_robot]->Fill(parm_V[i_SPS - 1] + tcorr_add - parm_h[i_SPS - 1] - gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
        hist_residuals_NullH[is_robot]->Fill(30.0 - gReader->GetSPS()->at(i_tray)->SPS_Vbd_25C->at(i_SPS - 1));
      }
    } else { // *-------------------------------------- No temperature correction
      for (int i_IV = 1; i_IV <= IV_size; ++i_IV) {
        hist_indexed_Vpeak->SetBinContent(i_IV, gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV - 1));
        hist_indexed_3derV->SetBinContent(i_IV, parm_V[i_IV - 1]);
        hist_indexed_3derH->SetBinContent(i_IV, parm_V[i_IV - 1] - parm_h[i_IV - 1]);
        
        corrdata_IV.push_back(gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_IV - 1));
        
      }// End of IV tabulating
      for (int i_SPS = 1; i_SPS <= SPS_size; ++i_SPS) {
        hist_indexed_Vbreakdown->SetBinContent(i_SPS, gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        
        corrdata_SPS.push_back(gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        corr_global_SPS[is_robot].push_back(gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        
        hist_indexed_diff->SetBinContent(i_SPS, parm_V[i_SPS - 1] - gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        
        // Fill residual histograms
        if (parm_V[i_SPS - 1] < 0) continue;
        
        hist_residuals_3derV[is_robot]->Fill(parm_V[i_SPS - 1] - gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        hist_residuals_NullV[is_robot]->Fill(39.0 - gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        
        
        if (parm_V[i_SPS - 1] - parm_h[i_SPS - 1] < 0 ||
            gReader->GetIV()->at(i_tray)->IV_Vpeak->at(i_SPS - 1) < 0) continue;
        
        hist_residuals_3derH[is_robot]->Fill(parm_V[i_SPS - 1] - parm_h[i_SPS - 1] - gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
        hist_residuals_NullH[is_robot]->Fill(30.0 - gReader->GetSPS()->at(i_tray)->SPS_Vbd->at(i_SPS - 1));
      }
    }
    
    // Gather average V_bd for the tray
    double avg_voltages[2];
    if (flag_use_all_trays_for_averages) {
      avg_voltages[0] = getAvgVpeakAllTrays(flag_run_at_25_celcius); //IV
      avg_voltages[1] = getAvgVbreakdownAllTrays(flag_run_at_25_celcius); //SPS
    } else {
      avg_voltages[0] = getAvgVpeak(i_tray, flag_run_at_25_celcius); //IV
      avg_voltages[1] = getAvgVbreakdown(i_tray, flag_run_at_25_celcius); //SPS
    }
    
    // Set plot range--dynamically based on the results of the testing
    double aspect_separation = (avg_voltages[0] - avg_voltages[1])/0.55;
    double voltplot_limits[2] = {
      avg_voltages[1] - 0.2 *aspect_separation,
      avg_voltages[0] + 0.25*aspect_separation
    };
    
    //Debug
    //    std::cout << "Average Vbr :: IV = " << avg_voltages[0] << ", SPS = " << avg_voltages[1] << "]." << std::endl;
    //    std::cout << "Voltplot limits :: [" << voltplot_limits[0] << ',' << voltplot_limits[1] << "]." << std::endl;
    
    if (true) {
//    if (voltplot_limits[1] - voltplot_limits[0] < 0.25) {
      voltplot_limits[0] = voltplot_limits_static[0];
      voltplot_limits[1] = voltplot_limits_static[1] - 10 * (1-is_robot);
    }
    
    // *-- plot histograms to represent the indexed SiPM test results
    
    // Set up the canvas/draw established hists
    gCanvas_solo->cd();
    hist_indexed_Vpeak->GetYaxis()->SetRangeUser(voltplot_limits[0], voltplot_limits[1]); // This line to comment out when swapping to temperature
    hist_indexed_Vpeak->GetYaxis()->SetTitleOffset(0.85);
    hist_indexed_Vpeak->SetMarkerColor(plot_colors[0]);
    hist_indexed_Vpeak->SetMarkerStyle(20);
    hist_indexed_Vpeak->Draw("hist p");
    
    hist_indexed_Vbreakdown->SetMarkerColor(plot_colors[1]);
    hist_indexed_Vbreakdown->SetMarkerStyle(21);
    
    hist_indexed_3derV->SetMarkerColor(kRed + 2);
    hist_indexed_3derV->SetMarkerStyle(24);
    
    hist_indexed_3derH->SetMarkerColor(kBlue + 2);
    hist_indexed_3derH->SetMarkerStyle(25);
    
    // Draw reference averaged +/- 50 MV lines
    TLine* avg_line = new TLine();
    
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
    
    // assure points sit on top of lines
    hist_indexed_Vpeak->Draw("hist p same");
    hist_indexed_Vbreakdown->Draw("hist p same");
    hist_indexed_3derV->Draw("hist p same");
    hist_indexed_3derH->Draw("hist p same");
    
    // Legend for labeling the two V_breakdown measurement types
    float leg_extra_space = 0;
    if (flag_run_at_25_celcius) leg_extra_space = 0.04;
    TLegend* vbd_legend = new TLegend(0.635, 0.16 + leg_extra_space, 0.90, 0.31 + leg_extra_space);
    vbd_legend->SetLineWidth(0);
    vbd_legend->AddEntry(hist_indexed_Vpeak,       Form("IV V_{bd} #kern[0.3]{(#color[2]{%i} outliers)}",
                                                        countOutliersVpeak(i_tray, flag_run_at_25_celcius)), "p");
    vbd_legend->AddEntry(hist_indexed_Vbreakdown,  Form("SPS V_{bd} #kern[0.1]{(#color[2]{%i} outliers)}",
                                                        countOutliersVbreakdown(i_tray, flag_run_at_25_celcius)), "p");
    vbd_legend->AddEntry(hist_indexed_3derV, "3^{rd} Der. V_{01}", "p");
    vbd_legend->AddEntry(hist_indexed_3derH, "3^{rd} Der. V_{10}", "p");
//    vbd_legend->SetFillColorAlpha(kWhite, 0.0);
    vbd_legend->Draw();
    
    // Legend for the lines marking tray average, test sets
//    TLegend* line_legend = new TLegend(0.15, 0.315 + leg_extra_space, 0.45, 0.55 + leg_extra_space);
//    line_legend->SetLineWidth(0);
//    hist_indexed_Vpeak->SetLineColor(kBlack);
//    if (flag_use_all_trays_for_averages) line_legend->AddEntry(hist_indexed_Vpeak, "Average over all trays", "l");
//    else                                 line_legend->AddEntry(hist_indexed_Vpeak, "Average over tray", "l");
//    line_legend->AddEntry(avg_line, "Average #pm 50mV", "l");
//    line_legend->AddEntry(cassette_line, "Test Runs (32 SiPM per test)", "l");
//    line_legend->Draw();
    
    
    // Draw some text giving info on the setup
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.06, 0.91, false, kBlack, 0.027);
    drawText("#bf{ePIC} Test Stand", 0.06, 0.955, false, kBlack, 0.027);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 0.98, 0.95, true, kBlack, 0.032);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
    
    // Zoom in to check for indexing errors
    if (index_shuffle != 0) {
      hist_indexed_Vpeak->GetYaxis()->SetRangeUser(38, 39);
//      hist_indexed_Vpeak->GetXaxis()->SetLimits(0, 20);
    }
    
    //save histograms
    gCanvas_solo->SaveAs(Form("%s/indexed_%s_%ismooth_%s.pdf",
                              outdir,
                              string_robotmode[is_robot],
                              nsmooths,
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    
    hist_indexed_diff->SetMarkerColor(kRed+2);
    hist_indexed_diff->SetMarkerStyle(8);
    hist_indexed_diff->GetYaxis()->SetRangeUser(-0.5, 1.0);
    hist_indexed_diff->Draw("hist p");
    
    // Draw some text giving info on the setup
    drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", 0.06, 0.91, false, kBlack, 0.027);
    drawText("#bf{ePIC} Test Stand", 0.06, 0.955, false, kBlack, 0.027);
    drawText(Form("Hamamatsu #bf{%s} Tray #%s", Hamamatsu_SiPM_Code, gReader->GetTrayStrings()->at(i_tray).c_str()), 0.98, 0.95, true, kBlack, 0.032);
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
    
    gCanvas_solo->SaveAs(Form("%s/diff_%s_%ismooth_%s.pdf",
                              outdir,
                              string_robotmode[is_robot],
                              nsmooths,
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    
    // Correlation plots
    gCanvas_solo->Clear();
    gCanvas_solo->SetLeftMargin(0.15);
    TGraph* graph_corr_3derV = new TGraph(corrdata_3derV.size(),
                                          corrdata_3derV.data(),
                                          corrdata_SPS.data());
    graph_corr_3derV->SetName("graph_corr_3derV");
    graph_corr_3derV->SetTitle(";3^{rd} Der. V_{01} [V];SPS V_{bd} [V]");
    graph_corr_3derV->SetLineColor(kRed + 2);
    graph_corr_3derV->SetMarkerColor(kRed + 2);
    graph_corr_3derV->SetMarkerStyle(24);
    
    
    TGraph* graph_corr_nominal = new TGraph(corrdata_IV.size(),
                                            corrdata_IV.data(),
                                            corrdata_SPS.data());
    graph_corr_nominal->SetName("graph_corr_3derV");
    graph_corr_nominal->SetTitle(";3^{rd} Der. V_{01} [V];SPS V_{bd} [V]");
    graph_corr_nominal->SetLineColorAlpha(kGray + 2, 0.5);
    graph_corr_nominal->SetMarkerColorAlpha(kGray + 2, 0.5);
    graph_corr_nominal->SetMarkerStyle(8);
    
    TMultiGraph* corr_graph = new TMultiGraph();
    corr_graph->Add(graph_corr_3derV);
    corr_graph->Add(graph_corr_nominal);
    
    corr_graph->GetXaxis()->SetLimits(37.5, 39.5);
    corr_graph->GetYaxis()->SetRangeUser(37, 39);
    corr_graph->Draw("ap");
    
    gCanvas_solo->SaveAs(Form("%s/correlationV_%s_%ismooth_%s.pdf",
                              outdir,
                              string_robotmode[is_robot],
                              nsmooths,
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    gCanvas_solo->Clear();
    TGraph* graph_corr_3derH = new TGraph(corrdata_3derH.size(),
                                          corrdata_3derH.data(),
                                          corrdata_SPS.data());
    graph_corr_3derH->SetName("graph_corr_3derH");
    graph_corr_3derH->SetTitle(";3^{rd} Der. V_{10} [V];SPS V_{bd} [V]");
    graph_corr_3derH->SetLineColor(kBlue + 2);
    graph_corr_3derH->SetMarkerColor(kBlue + 2);
    graph_corr_3derH->SetMarkerStyle(25);
    
    graph_corr_3derH->GetXaxis()->SetLimits(10, 40);
    graph_corr_3derH->GetYaxis()->SetRangeUser(37, 39);
    graph_corr_3derH->Draw("ap");
    
    gCanvas_solo->SaveAs(Form("%s/correlationH_%s_%ismooth_%s.pdf",
                              outdir,
                              string_robotmode[is_robot],
                              nsmooths,
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    
    delete hist_indexed_Vpeak;
    delete hist_indexed_Vbreakdown;
    delete hist_indexed_3derV;
    delete hist_indexed_3derH;
    delete hist_indexed_diff;
    delete graph_corr_3derV;
    delete graph_corr_3derH;
    // debug by running only a few trays
//    if (i_tray >= 0) break;
  }// End of tray loop
  
  
  
  
  // Plot final histogram of coefficients
  TCanvas* canvas_quad = new TCanvas("canvas_quad","canvas_quad",1000,1000);
  canvas_quad->Divide(2,2);
  for (int p = 0; p < 4; ++p) {
    canvas_quad->cd(p + 1);
    gPad->SetLeftMargin(0.15);
    gPad->SetTicks(1,1);
    
    hist_params[0][p]->Scale(1./ntotal_goodfits[0], "width");
    hist_params[1][p]->Scale(1./ntotal_goodfits[1], "width");
    
    hist_params[0][p]->GetYaxis()->SetRangeUser(0, 1.15*TMath::Max(hist_params[0][p]->GetMaximum(), hist_params[1][p]->GetMaximum()));
    hist_params[0][p]->Draw("hist");
    hist_params[1][p]->Draw("hist same");
    
  }canvas_quad->cd(4);
  gPad->SetLogx();
  
  canvas_quad->cd(1);
  TLegend* leg = new TLegend(0.60, 0.6, 0.85, 0.85);
  leg->SetLineWidth(0);
  leg->AddEntry(hist_params[0][0], "Cassette", "l");
  leg->AddEntry(hist_params[1][0], "Robot", "l");
  leg->Draw();
  
  canvas_quad->cd();
  drawText(Form("Total successful fits: #color[%i]{%i/%i}, #color[%i]{%i/%i}",
                color_robotmode[0],ntotal_goodfits[0], ntotal_SiPMs[0],
                color_robotmode[1],ntotal_goodfits[1], ntotal_SiPMs[1]), 0.05, 0.97, false, kBlack, 0.02);
  
  canvas_quad->SaveAs(Form("../plots/3rdder/3rdder_param_hist_%s.pdf",
                           string_tempcorr_short[flag_run_at_25_celcius]));
  
  
  // Resiudal histograms -- turn off voltage
  gCanvas_solo->cd();
  gPad->SetLeftMargin(0.13);
  hist_residuals_3derH[0]->GetYaxis()->SetRangeUser(0, 1.15*TMath::Max(hist_residuals_3derH[0]->GetMaximum(),
                                                                       hist_residuals_3derH[1]->GetMaximum()));
  hist_residuals_3derH[0]->Draw("hist");
  hist_residuals_3derH[1]->Draw("hist same");
  hist_residuals_NullH[0]->Draw("hist same");
  hist_residuals_NullH[1]->Draw("hist same");
  TLegend* leg_alt = new TLegend(0.40, 0.6, 0.65, 0.85);
  leg_alt->SetLineWidth(0);
  leg_alt->AddEntry(hist_params[0][0], Form("Cassette (#sigma = %.2fmV)", 1000*hist_residuals_3derH[0]->GetStdDev()), "l");
  leg_alt->AddEntry(hist_params[1][0], Form("Robot (#sigma = %.2fmV)", 1000*hist_residuals_3derH[1]->GetStdDev()), "l");
  leg_alt->AddEntry(hist_residuals_NullV[0], "Cassette Null", "f");
  leg_alt->AddEntry(hist_residuals_NullV[1], "Robot Null", "f");
  leg_alt->Draw();
  drawText("Turn-off Voltage V_{10} = V_{01} - h", gPad->GetLeftMargin(), 1.04 - gPad->GetTopMargin());
  drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
  
  gCanvas_solo->SaveAs(Form("../plots/3rdder/3rdder_turnoff_residuals_%s.pdf",
                            string_tempcorr_short[flag_run_at_25_celcius]));
  
  // Resiudal histograms -- turn on voltage
  gCanvas_solo->cd();
  hist_residuals_3derV[0]->GetYaxis()->SetRangeUser(0, 1.15*TMath::Max(hist_residuals_3derV[0]->GetMaximum(),
                                                                       hist_residuals_3derV[1]->GetMaximum()));
  hist_residuals_3derV[0]->Draw("hist");
  hist_residuals_3derV[1]->Draw("hist same");
  hist_residuals_NullV[0]->Draw("hist same");
  hist_residuals_NullV[1]->Draw("hist same");
  TLegend* leg_alt2 = new TLegend(0.40, 0.6, 0.65, 0.85);
  leg_alt2->SetLineWidth(0);
  leg_alt2->AddEntry(hist_params[0][0], Form("Cassette (#sigma = %.2fmV)", 1000*hist_residuals_3derV[0]->GetStdDev()), "l");
  leg_alt2->AddEntry(hist_params[1][0], Form("Robot (#sigma = %.2fmV)", 1000*hist_residuals_3derV[1]->GetStdDev()), "l");
  leg_alt2->AddEntry(hist_residuals_NullV[0], "Cassette Null", "f");
  leg_alt2->AddEntry(hist_residuals_NullV[1], "Robot Null", "f");
  leg_alt2->Draw();
  
  drawText("Turn-on Voltage V_{01}", gPad->GetLeftMargin(), 1.04 - gPad->GetTopMargin());
  drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
  
  gCanvas_solo->SaveAs(Form("../plots/3rdder/3rdder_turnon_residuals_%s.pdf",
                            string_tempcorr_short[flag_run_at_25_celcius]));
  
  std::cout << "Skewness of main dist :: \033[31m" << hist_residuals_3derV[1]->GetSkewness() << "\033[39m" << std::endl;
  
  // h+/h- ratio histograms
  gCanvas_solo->cd();
  gPad->SetLogy();
  hist_hratio[0]->Draw("hist");
  hist_hratio[1]->Draw("hist same");
  hist_hratio_failed[0]->Draw("hist same");
  hist_hratio_failed[1]->Draw("hist same");
  
  leg->AddEntry(hist_hratio_failed[0], "Cassette (Failed)", "f");
  leg->AddEntry(hist_hratio_failed[1], "Robot (Failed)", "f");
  leg->Draw();
  
  gCanvas_solo->SaveAs(Form("../plots/3rdder/3rdder_hist_hratio_%s_%s.pdf",
                            string_tempcorr_short[flag_run_at_25_celcius],
                            string_tempcorr_short[flag_run_at_25_celcius]));
  
  delete gCanvas_solo;
  gCanvas_solo = new TCanvas();
  gCanvas_solo->SetCanvasSize(1000, 1000);
  gCanvas_solo->SetLeftMargin(0.13);
  gCanvas_solo->SetRightMargin(0.03);
  gPad->SetTicks(1,1);
  // Correlations among all data
  for (int i_robot = 0; i_robot < 2; ++i_robot) {
    if (corr_global_3derH[i_robot].size() == 0) continue;
    
    gCanvas_solo->Clear();
    TGraph* corr_breakdown_3derV = new TGraph(corr_global_3derV[i_robot].size(),
                                              corr_global_3derV[i_robot].data(),
                                              corr_global_SPS[i_robot].data());
    corr_breakdown_3derV->SetName(Form("graph_corr_3derV_all_%s",string_robotmode[i_robot]));
    corr_breakdown_3derV->SetTitle(";3^{rd} Der. V_{01} [V];SPS V_{bd} [V]");
    corr_breakdown_3derV->SetLineColor(kRed + 2);
    corr_breakdown_3derV->SetMarkerColor(kRed + 2);
    corr_breakdown_3derV->SetMarkerStyle(24);
    
    corr_breakdown_3derV->GetXaxis()->SetLimits(37.5, 39.5);
    corr_breakdown_3derV->GetYaxis()->SetRangeUser(37, 39);
    corr_breakdown_3derV->Draw("ap");
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
    
    // Count good SiPMs (which don't fail 3rd derivative or SPS)
    // and also write this data to a text file for later plotting
    std::ofstream outfile(Form("../plots/3rdder/3rdder_alldata_%s_%s.txt",
                               string_robotmode[i_robot],
                               string_tempcorr_short[flag_run_at_25_celcius]));
    int total_good_SiPMs = 0;
    outfile << "3rd derivative method data\nFITMODE :: ";
#ifdef fit1
    outfile << "chi2 fitting.\n\n";
#elifdef fit2
    outfile << "peak fixing.\n\n";
#endif
    outfile << string_tempcorr[flag_run_at_25_celcius] << std::endl;
    outfile << "SPS\t3D_V01\t3D_V10" << std::endl;
    for (int i = 0; i < corr_global_3derV[i_robot].size(); ++i) {
      if (corr_global_3derV[i_robot][i] < 0) continue;
      if (corr_global_SPS[i_robot][i] < 0) continue;
      ++total_good_SiPMs;
      outfile << corr_global_SPS[i_robot][i] << '\t';
      outfile << corr_global_3derV[i_robot][i] << '\t';
      outfile << corr_global_3derH[i_robot][i] << std::endl;
    }outfile.close();
    
    drawText("good/has binary/all SiPMs:", gPad->GetLeftMargin(), 0.95);
    drawText(Form("%i/%i/%i",
                  total_good_SiPMs,
                  ntotal_SiPMs[i_robot],
                  ntotal_possible[i_robot]),
             gPad->GetLeftMargin(), 1.01 - gPad->GetTopMargin());
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
    
    gCanvas_solo->SaveAs(Form("../plots/3rdder/3rdder_correlationV_%s_%s.pdf",
                              string_robotmode[i_robot],
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    gCanvas_solo->Clear();
    TGraph* corr_breakdown_3derH = new TGraph(corr_global_3derH[i_robot].size(),
                                              corr_global_3derH[i_robot].data(),
                                              corr_global_SPS[i_robot].data());
    corr_breakdown_3derH->SetName("graph_corr_3derH_all");
    corr_breakdown_3derH->SetTitle(";3^{rd} Der. V_{10} [V];SPS V_{bd} [V]");
    corr_breakdown_3derH->SetLineColor(kBlue + 2);
    corr_breakdown_3derH->SetMarkerColor(kBlue + 2);
    corr_breakdown_3derH->SetMarkerStyle(25);
    
    corr_breakdown_3derH->GetXaxis()->SetLimits(10, 40);
    corr_breakdown_3derH->GetYaxis()->SetRangeUser(37, 39);
    corr_breakdown_3derH->Draw("ap");
    
    drawText("good/has binary/all SiPMs:", gPad->GetLeftMargin(), 0.95);
    drawText(Form("%i/%i/%i",
                  total_good_SiPMs,
                  ntotal_SiPMs[i_robot],
                  ntotal_possible[i_robot]),
             gPad->GetLeftMargin(), 1.01 - gPad->GetTopMargin());
    drawText(Form("%s", string_tempcorr[flag_run_at_25_celcius]), 0.98, 0.905, true, kBlack, 0.022);
    
    gCanvas_solo->SaveAs(Form("../plots/3rdder/3rdder_correlationH_%s_%s.pdf",
                              string_robotmode[i_robot],
                              string_tempcorr_short[flag_run_at_25_celcius]));
    
    
    delete corr_breakdown_3derV;
    delete corr_breakdown_3derH;
  }// End of robot/cassette global correlation plots
  
  
  
  // Final memory clear and return
  delete finalReader;
  delete obj_list;
  delete gCanvas_solo;
  delete gCanvas_double;
  std::free(sg_coefs);
  return;
}// End of analyze_thrid_derivative::main






// ----------------------------------------------------------------- Helper Methods




// Get the Savitzky Golay coefficients for the relevant
// polynomial order, interpolation window and
double* getSavitzkyGolayCoefs(int poly_order, 
                              int interp_window,
                              int nderivs) {
  double* coefs = static_cast<double*>(std::malloc(sizeof(double)*(2*interp_window + 1)));
  
  // Get data from file, where coefficients were computed in Mathematica
  char file_SG_coefs[100];
  snprintf(file_SG_coefs, 100, "sg_coefs/%ideriv.txt",nderivs);
  std::ifstream SGfile(file_SG_coefs);
  
  std::string line;
  while (std::getline(SGfile, line)) {
    std::stringstream linestream(line);
    std::string entry;
    int r = line[2] - '0';
    int k = line[7] - '0';
    if (k == poly_order && r == interp_window) {
      
      // Report on the found data
      std::getline(linestream, entry, '{');
      std::getline(linestream, entry, '}');
      std::string coefarray_string = entry;
      std::getline(linestream, entry, ' ');
      std::getline(linestream, entry);
      std::cout << "SG Coefs for poly \033[31m" << poly_order << "\033[39m";
      std::cout << ", window size \033[31m" << interp_window << "\033[39m";
      std::cout << ", nderivs = \033[31m" << nderivs << "\033[39m :: {";
      std::cout << coefarray_string << "} / " << entry << std::endl;
      
      // Assign coefficients and return
      std::stringstream coefstream(coefarray_string);
      double norm = std::stod(entry);
      int i = 0;
      while (getline(coefstream, entry, ',')) {
        coefs[i] = std::stod(entry) / norm;
        ++i;
      }return coefs;
    }// End of found coefficients
  }// End of data loop
  
  
  
  // default case: zeroes
  std::cout << "\033[33mWarning\033[39m in <getSavitzkyGolayCoefs>: bad inputs! ";
  std::cout << "An array of zero coefficients will be returned instead." << std::endl;
  for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] = 0;
  return coefs;
}// End of analyze_thrid_derivative::getSavitzkyGolayCoefs3der



// Convolve two lists, assuming listA is longer than listB
double* performListConvolution(double* listA, int sizeA,
                               double* listB, int sizeB) {
  if (sizeA < sizeB) return nullptr;
  double* convolved = static_cast<double*>(std::malloc(sizeof(double)*sizeA));
  for (int i = 0; i < sizeA; ++i) convolved[i] = 0;
  
  int wid = (sizeB) / 2;
  for (int a = wid; a < sizeA - wid; ++a) {
    for (int b = 0; b < sizeB; ++b) {
      convolved[a] += listA[a - wid + b] * listB[b];
    }
  }return convolved;
}// End of analyze_third_derivative::performListConvolution
