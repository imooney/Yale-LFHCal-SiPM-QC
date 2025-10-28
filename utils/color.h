// A header file with some tools for terminal printing in color
// or otherwise modifying terminal output to be stylized.
//
// Utils for recreational computuing and math
// Created by Ryan Hamilton on Dec. 5th, 2025


#include <ostream>

#ifndef color_h
#define color_h

namespace Color 
{

enum ColorVal {
  TEXT_BLACK      = 30,
  TEXT_RED        = 31,
  TEXT_GREEN      = 32,
  TEXT_YELLOW     = 33,
  TEXT_BLUE       = 34,
  TEXT_MAGENTA    = 35,
  TEXT_CYAN       = 36,
  TEXT_WHITE      = 37,
  TEXT_DEFAULT    = 39,
  BACK_BLACK      = 40,
  BACK_RED        = 41,
  BACK_GREEN      = 42,
  BACK_YELLOW     = 43,
  BACK_BLUE       = 44,
  BACK_MAGENTA    = 45,
  BACK_CYAN       = 46,
  BACK_WHITE      = 47,
  BACK_DEFAULT    = 49
};

class TextModifier {
  
  ColorVal color_list;
  
public:
  
  TextModifier(ColorVal color_input) : color_list(color_input) {}
  
  friend std::ostream&
  operator<<(std::ostream& os, const TextModifier& mod) {
    return os << "\033[" << mod.color_list << "m";
  }
};

}


#endif /* color_h */
