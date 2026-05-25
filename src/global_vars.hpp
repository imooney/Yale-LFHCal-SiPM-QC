//  *--
//  global_vars
//
//  Header file with certain global parameters that might be used throughout the analysis
//
//  Created by Ryan Hamilton on 10/19/25.
//  *--

#ifndef global_vars_h
#define global_vars_h

#include <stdio.h>
#include <sys/stat.h>
#include "../utils/color.h"
#include "../utils/root_draw_tools.h"

// Terminal output color modifiers
// Example: std::cout << t_red << "ex." << t_def << std::endl;
Color::TextModifier t_red(Color::TEXT_RED);
Color::TextModifier t_mgn(Color::TEXT_MAGENTA);
Color::TextModifier t_grn(Color::TEXT_GREEN);
Color::TextModifier t_yll(Color::TEXT_YELLOW);
Color::TextModifier t_blu(Color::TEXT_BLUE);
Color::TextModifier t_cyn(Color::TEXT_CYAN);
Color::TextModifier t_def(Color::TEXT_DEFAULT);

// Tags and Identifiers
const char Hamamatsu_SiPM_Code[20] = "S14160-1315PS";

// Variables for allowed Q/A Ranges
const double declare_Vbd_outlier_range = 0.050; // +/- 50mV range around average
const bool use_quadrature_sum_for_syst_error = true;
const double contract_outlier_margin_percent = 5; // 5% outliers allowed by contract
const float Hamamatsu_spec_max_Idark = 20.;

// Fixed array info variables
const int NROW = 20;
const int NCOL = 23;

// Information about SiPM locations in physical space
const float temp_sensor_separation_cm = 0.27;
const float sipm_cassette_separation_cm = 1.0; // TODO measure actual board, these are placeholders from what I roughly remember.
// Temp corr looks weird, seems like factor of 2 could make them align??? I don't think the SiPMs are further than the sensors, that wouldn't make sense....

// Variables for I/O handling
const char batch_data_file[20] = "../batch_data.txt";

// flags to control some options in analysis
bool flag_use_all_trays_for_averages = false;       // Use all available trays' data to compute averages (Recommended ONLY when all trays are similar)

// Variables to control histogram/plot ranges
const int nbin_temp_grad = 19;
//const int nbin_temp_grad = 39;
const double range_temp_grad[2] = {-0.4, 0.4};

// Color pallette for the code
// Colors sampled from/inspired by:
//    - https://www.metmuseum.org/art/collection/search/717587
//    - https://arxiv.org/pdf/2107.02270
Int_t TH2_palette = kBird;
// Colors are ordered as:
//      {main, darker, lighter}
Int_t color_IV[3] = {
  getTColorFromHex("#7817DA"),
  getTColorFromHex("#4E128A"),
  getTColorFromHex("#C6A0ED")
};
Int_t color_SPS[3] = {
  getTColorFromHex("#FD8628"),
  getTColorFromHex("#A54B08"),
  getTColorFromHex("#FFBD8C")
};
Int_t color_cassette[3] = {
  getTColorFromHex("#2C8D46"),
  getTColorFromHex("#06581C"),
  getTColorFromHex("#B5DFC0")
};
Int_t color_robot[3] = {
  getTColorFromHex("#00B5BC"),
  getTColorFromHex("#1A7F83"),
  getTColorFromHex("#BADDDE")
};
Int_t color_accent1[3] = {
  getTColorFromHex("#E23813"),
  getTColorFromHex("#AA3218"),
  getTColorFromHex("#E7B0A5")
};
Int_t color_accent2[3] = {
  getTColorFromHex("#319BD9"),
  getTColorFromHex("#2072A2"),
  getTColorFromHex("#ADCFE2")
};
Int_t color_accent3[3] = {
  getTColorFromHex("#68B442"),
  getTColorFromHex("#377B14"),
  getTColorFromHex("#BED3B3")
};
Int_t color_flatgray[3] = {
  getTColorFromHex("#A2A2A2"),
  getTColorFromHex("#404040"),
  getTColorFromHex("#E3E3E3")
};
Int_t color_warmgray[3] = {
  getTColorFromHex("#BDACA5"),
  getTColorFromHex("#443732"),
  getTColorFromHex("#E3DCD9")
};
Int_t color_coolgray[3] = {
  getTColorFromHex("#9AA5AA"),
  getTColorFromHex("#404A50"),
  getTColorFromHex("#DEE3E6")
};

// When accessing IV/SPS in an array
Int_t plot_colors[3] = {
  color_IV[0],
  color_SPS[0],
  color_accent1[0]
};
Int_t plot_colors_alt[3] = {
  color_IV[1],
  color_SPS[1],
  color_accent1[1]
};


// Plot settings

// Tray display mode for dark current histogram:
//  0 - No tray numbers are displayed
//  1 - All tray numbers are displayed
//  2 - Tray numbers are displayed in condensed notation
const int tray_display_mode = 0;


#endif /* global_vars_h */
