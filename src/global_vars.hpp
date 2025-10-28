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
Color::TextModifier t_grn(Color::TEXT_GREEN);
Color::TextModifier t_blu(Color::TEXT_BLUE);
Color::TextModifier t_def(Color::TEXT_DEFAULT);

// Tags and Identifiers
const char Hamamatsu_SiPM_Code[20] = "S14160-1315PS";

// Variables for allowed Q/A Ranges
const double declare_Vbd_outlier_range = 0.05; // +/- 50mV range around average

// Fixed arraay info variables
const int NROW = 20;
const int NCOL = 23;

// Variables for I/O handling
const char batch_data_file[20] = "../batch_data.txt";


#endif /* global_vars_h */
