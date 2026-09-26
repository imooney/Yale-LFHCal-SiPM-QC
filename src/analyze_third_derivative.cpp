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
//  Changelog ::
//    - 09/14/2026    : Created by Ryan Hamilton
//  *--

#include "global_vars.hpp"
#include "lib_debrecen_IV/IVBinaryFile.h"


#define fit1

//========================================================================== Global Variables

// Base relative location of IV binary files
char bin_base_directory[100] = "../data/IV_bin/production/robot_production";

// Switch to use DMM or SMU voltage measurement
bool use_DMM_over_SMU = true;
char voltage_type_str[2][5] = {"SMU","DMM"};

// Controls for the Savitzky-Golay filter
int SG_poly_order = 3;
int SG_window_oneside = 4;

// Debug flag
bool _debug = false;


// Root global pointers
TCanvas* gCanvas_solo;
TCanvas* gCanvas_double;
std::vector<std::vector<TPad*> > cpads;

//========================================================================== Forward declarations

double* getSavitzkyGolayCoefs3der(int poly_order, int interp_window);

double* performListConvolution(double* listA, int sizeA,
                               double* listB, int sizeB);


//========================================================================== Macro Main
void analyze_third_derivative() {
  
  // Initialize canvases
  gStyle->SetOptStat(0);
  
  TObjArray* obj_list = new TObjArray();
  gCanvas_solo = new TCanvas();
  gPad->SetTicks(1,1);
  obj_list->AddLast(gCanvas_solo);
  
  gCanvas_double = new TCanvas();
  gCanvas_double->SetCanvasSize(1000, 500);
  gCanvas_double->Divide(2, 1);
  obj_list->AddLast(gCanvas_double);
  
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
  
  
  
  // ----------------------------------------------------------------- Read IV data from binary file
  
  // Initialize the IV reader class with a sample output file
  char trayno[15] = "250821-1305";
  int row_index = 0;
  int col_index = 0;
  char fulldir[200];
  snprintf(fulldir, 200, "%s/%s/binaries/IV_%s_%i_%i.bin", bin_base_directory, trayno, trayno, row_index, col_index);
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
  for (int i = 0; i < n_points; ++i) *(array_I+i) *= 1e-3;
  
  // ----------------------------------------------------------------- Computing and fitting the 3rd derivative
  
  
  // Get the convolution kernel for the desired SG filter
  int ntotal_sg_coefs = 2*SG_window_oneside + 1;
  double* sg_coefs = getSavitzkyGolayCoefs3der(SG_poly_order, SG_window_oneside);
  
  std::cout << "SG convolution kernel : { ";
  for (int c = 0; c < ntotal_sg_coefs; ++c) {
    std::cout << sg_coefs[c] << ' ';
  }std::cout << " }." << std::endl;
  
  // Perform the convolution to estimate the third derivative
  double* sg_3der_result = performListConvolution(array_I, n_points, sg_coefs, ntotal_sg_coefs);
  
  
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
  for (int i = 0; i < n_points; ++i) error_I[i] = coeff_error_I * array_I[i];
  
  // *---------------- Perform fit
  
  
  // Definition of the data in TGraph fomat
  TGraphErrors* graph3der = new TGraphErrors(n_points - 2*SG_window_oneside,        // Number of sample points
                                             array_V + SG_window_oneside,           // Voltage data
                                             sg_3der_result + SG_window_oneside,    // Current data
                                             error_V + SG_window_oneside,           // Voltage error
                                             error_I + SG_window_oneside);          // Current error
#ifdef fit1
  // Functional form
//  TF1* func_form = new TF1("DebrecenGaussFitForm",
//                           "[0] * (2 - [3] * (x - [1])/([2]**2) ) * TMath::Exp(-(x - [1])**2/(2*[2]**2))",
//                           array_V[0], array_V[n_points - 1]);
  TF1* func_form = new TF1("DebrecenGaussFitForm",
                           "gaus(0) + [3] * (x - [1])/(2 * [2]**2) * gaus(0)",
                           array_V[0], array_V[n_points - 1]);
  func_form->SetParNames("A","V0","#sigma","h");
  
  // fit lower and upper range limits
  double fitrange[2] = {array_V[10], array_V[100]};
  std::cout << "Fitting in range [" << fitrange[0] << ',' << fitrange[1] << ']' << std::endl;
  
  // Initial guesses with reasonable values for the fitting
  double Ainit = 9e-13;
  double hinit = 0.7;
  double sinit = 0.11;
  double Vinit = 38.1;
  
  double Arange[2] = {1e-13, 8e-12};
  double Vrange[2] = {Vinit - 0.5, Vinit + 0.5};
  double srange[2] = {sinit - 0.05, sinit + 0.05};
  double hrange[2] = {hinit - 0.1, hinit + 0.1};
  
  func_form->SetParLimits(0, Arange[0], Arange[1]);
  func_form->SetParLimits(1, Vrange[0], Vrange[1]);
  func_form->SetParLimits(2, srange[0], srange[1]);
  //  func_form->SetParLimits(3, hrange[0], hrange[1]);
  
  
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
          if (fit_result->Chi2() < best_chi2) {
            best_chi2 = fit_result->Chi2();
            std::cout << "New best chi2 found :: " << best_chi2 << std::endl;
            for (int iPar = 0; iPar < 4; ++iPar) {
              best_params[iPar] = func_form->GetParameter(iPar);
            }
          }
  }// End of chi2 scan fitting
  
  
  func_form->SetParameters(best_params[0], best_params[1], best_params[2], best_params[3]);
 
  
  
  
#else
  // Functional form
  
  // Another approach?
  
  
#endif
  
  
  // TODO SiPM data reader to access debrecen results
  
  
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
  graph3der->SetName(Form("graph3der_%s_%s_%i_%i",trayno,voltage_type_str[use_DMM_over_SMU],row_index,col_index));
  graph3der->SetTitle(Form(";%s Voltage [V];3^{rd} Derivative d^{3}I/dV^{3} [A / V^{3}]",voltage_type_str[use_DMM_over_SMU]));
  graph3der->SetLineColor(kBlack);
  graph3der->SetMarkerColor(kBlack);
  graph3der->SetMarkerStyle(6);
  graph3der->GetYaxis()->SetRangeUser(-60e-12, 9e-11);
  graph3der->Draw("ap");
  func_form->Draw("same");
  obj_list->AddLast(graph3der);
  
  
  // Draw some text giving info on the setup
  gCanvas_double->cd();
  double pad = 0.005;
  drawText("#bf{ePIC} Test Stand", pad + gPad->GetLeftMargin(), 0.955, false, kBlack, 0.035);
  drawText("#bf{Debrecen} SiPM Test Setup @ #bf{Yale}", pad + gPad->GetLeftMargin(), 0.915, false, kBlack, 0.035);
  drawText(Form("Hamamatsu #bf{%s}", Hamamatsu_SiPM_Code), 1 - pad - gPad->GetRightMargin(), 0.955, true, kBlack, 0.035);
  
  // SiPM labels
  gCanvas_double->cd(1);
  drawText(Form("Tray #%s",trayno), 0.05 + gPad->GetLeftMargin(), 0.92 - gPad->GetTopMargin(), false, kBlack, 0.04);
  drawText(Form("SiPM (%i,%i)",row_index,col_index), 0.05 + gPad->GetLeftMargin(), 0.88 - gPad->GetTopMargin(), false, kBlack, 0.04);
  
  // SG filter settings
  gCanvas_double->cd(2);
//  drawText(Form("Tray #%s",trayno), 0.95 - gPad->GetRightMargin(), 0.82 - gPad->GetTopMargin(), true, kBlack, 0.04);
//  drawText(Form("SiPM (%i,%i)",row_index,col_index), 0.95 - gPad->GetRightMargin(), 0.78 - gPad->GetTopMargin(), true, kBlack, 0.04);
  drawText("SG Settings ::", 0.05 + gPad->GetLeftMargin(), 0.20 + gPad->GetBottomMargin(), false, kBlack, 0.04);
  drawText(Form("Poly. order = %i",SG_poly_order), 0.05 + gPad->GetLeftMargin(), 0.15 + gPad->GetBottomMargin(), false, kBlack, 0.04);
  drawText(Form("Conv. window = %i",2*SG_window_oneside+1), 0.05 + gPad->GetLeftMargin(), 0.10 + gPad->GetBottomMargin(), false, kBlack, 0.04);
  
  // Fit results
  char latex_fit_form[200] = "#frac{d^{3}I}{dV^{3}} = A #left(2 #minus #frac{h}{#sigma^{2}} (V #minus V_{0}) #right) Exp#left[#frac{#minus(V #minus V_{0})^{2}}{2#sigma^{2}} #right]";
  drawText(latex_fit_form, 0.07 + gPad->GetLeftMargin(), 0.90 - gPad->GetTopMargin(), false, kBlack, 0.035);
  drawText(Form("A = %.4fE-12",func_form->GetParameter(0)*1e12), 0.05 + gPad->GetLeftMargin(), 0.83 - gPad->GetTopMargin(), false, kBlack, 0.04);
  drawText(Form("h = %.4f",func_form->GetParameter(3)), 0.05 + gPad->GetLeftMargin(), 0.78 - gPad->GetTopMargin(), false, kBlack, 0.04);
  drawText(Form("#sigma = %.4f",func_form->GetParameter(2)), 0.05 + gPad->GetLeftMargin(), 0.73 - gPad->GetTopMargin(), false, kBlack, 0.04);
  drawText(Form("V_{0} = %.4f",func_form->GetParameter(1)), 0.05 + gPad->GetLeftMargin(), 0.67 - gPad->GetTopMargin(), false, kBlack, 0.04);
  
  
  
  // Save the canvas
  gCanvas_double->SaveAs(Form("../plots/3rdder/singlesipm/%s/IV_%s_3rdder_%s_%i_%i.pdf",
                              trayno,
                              voltage_type_str[use_DMM_over_SMU],
                              trayno,
                              row_index,
                              col_index));
  
  
  // Delete dynamically initialzied objects and return
  delete reader;
  TObjArrayIter TObj_iter(obj_list);
  while (TNamed* data_obj = (TNamed*)TObj_iter.Next()) delete data_obj;
  delete obj_list;
  std::free(sg_coefs);
  return;
}// End of analyze_thrid_derivative::main



// Get the Savitzky Golay coefficients for the relevant
// polynomial order, interpolation window and
double* getSavitzkyGolayCoefs3der(int poly_order,
                                  int interp_window) {
  double* coefs = static_cast<double*>(std::malloc(sizeof(double)*(2*interp_window + 1)));
  
  switch (poly_order) {
    case 1: case 2: // too small
      break;
    case 3: case 4: // 3rd, 4th
      switch (interp_window) {
        case 2:
          coefs[0] = -1;
          coefs[1] = 2;
          coefs[2] = 0;
          coefs[3] = -2;
          coefs[4] = 1;
          for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] /= 2.;
          return coefs;
        case 3:
          coefs[0] = -1;
          coefs[1] = 1;
          coefs[2] = 1;
          coefs[3] = 0;
          coefs[4] = -1;
          coefs[5] = -1;
          coefs[6] = 1;
          for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] /= 6.;
          return coefs;
        case 4:
          coefs[0] = -14;
          coefs[1] = 7;
          coefs[2] = 13;
          coefs[3] = 9;
          coefs[4] = 0;
          coefs[5] = -9;
          coefs[6] = -13;
          coefs[7] = -7;
          coefs[8] = 14;
          for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] /= 198.;
          return coefs;
        default:
          break;
      }break;
    case 5: case 6: // 5th, 6th
      switch (interp_window) {
        case 3:
          coefs[0] = 1;
          coefs[1] = -8;
          coefs[2] = 13;
          coefs[3] = 0;
          coefs[4] = -13;
          coefs[5] = 8;
          coefs[6] = -1;
          for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] /= 8.;
          return coefs;
        case 4:
          coefs[0] = 100;
          coefs[1] = -457;
          coefs[2] = 256;
          coefs[3] = 459;
          coefs[4] = 0;
          coefs[5] = -459;
          coefs[6] = -256;
          coefs[7] = 457;
          coefs[8] = -100;
          for (int i = 0; i < 2*interp_window + 1; ++i) coefs[i] /= 1144.;
          return coefs;
        default:
          break;
      }break;
    default:
      break;
  }// End of poly order switch
  
  // default case: zeroes
  std::cout << "\033[33mWarning\033[39m in <getSavitzkyGolayCoefs3der>: bad inputs!";
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
