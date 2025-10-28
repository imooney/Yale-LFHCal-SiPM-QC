// A collection of small macros to make certain tasks simpler with root.
// Macros are organized roughly by function

#ifndef root_draw_tools
#define root_draw_tools

#include "TPad.h"
#include "TH1.h"
#include "TGraph.h"
#include "TLatex.h"

// TTreeReader checker
bool CheckValue(ROOT::Internal::TTreeReaderValueBase& value) {
  if (value.GetSetupStatus() < 0) {
    std::cerr << "Error " << value.GetSetupStatus()
    << " setting up reader for " << value.GetBranchName() << std::endl;
    return false;
  }
  return true;
}

//========================================================================== Construction of TObjects

//Draws text as specified by the input parameters
//Returns pointer for further modification if needed
//  e.g. drawText(...)->SetTextAngle(90);
TLatex* drawText(const char *text,
                 float xp,
                 float yp,
                 bool isRightAlign = 0,
                 int textColor=kBlack,
                 double textSize=0.04,
                 int textFont = 42,
                 bool isNDC = true){
  TLatex *tex = new TLatex(xp,yp,text);
  tex->SetTextFont(textFont);
  tex->SetTextSize(textSize);
  tex->SetTextColor(textColor);
  tex->SetLineWidth(1);
  if(isNDC) tex->SetNDC();
  if(isRightAlign) tex->SetTextAlign(31);
  tex->Draw();
  return tex;
}

// Constructs, draws and returns a TPad with the given specifications
TPad* buildPad(const char *padName,
               float x1, float y1,
               float x2, float y2,
               float leftMargin = 0.1,
               float rightMargin = 0.1,
               float bottomMargin = 0.1,
               float topMargin = 0.1,
               bool isTransparent = true) {
  TPad *pad = new TPad(padName, "", x1, y1, x2, y2);
  pad->SetLeftMargin(leftMargin);
  pad->SetRightMargin(rightMargin);
  pad->SetTopMargin(topMargin);
  pad->SetBottomMargin(bottomMargin);
  if (isTransparent) {pad->SetFillStyle(4000); pad->SetFrameFillStyle(4000);}
  pad->Draw();
  return pad;
}

// Divide the parent pad into evenly split flush pads with the provided margins
// Pads are returned in a double vector of TPad pointers.
// Pads are indexed like a matrix: (top -> bottom) x (left -> right)
std::vector<std::vector<TPad*>> divideFlush(TVirtualPad* parent,
                                            const int n_horiz,
                                            const int n_vert,
                                            float leftMargin = 0.1,
                                            float rightMargin = 0.05,
                                            float bottomMargin = 0.1,
                                            float topMargin = 0.05) {
  // Setup and checks
  std::vector<std::vector<TPad*>> pads;
  float length = (1-leftMargin-rightMargin)/n_horiz;
  float height = (1-topMargin-bottomMargin)/n_vert;
  if (length < 0 || height < 0) return pads;
  
  // Construct flush pads in the specified grid
  for (int j = 0; j < n_vert; ++j) {
    pads.push_back(*(new std::vector<TPad*>));
    for (int i = 0; i < n_horiz; ++i) {
      pads.at(j).push_back(buildPad(Form("pad%i", n_horiz*j+i),
                                    leftMargin*(i>0)+length*i,
                                    TMath::Abs(1-(topMargin+bottomMargin*((j+1)==n_vert)+height*(j+1))),
                                    leftMargin+rightMargin*((i+1)==n_horiz)+length*(i+1),
                                    TMath::Abs(1-(topMargin*(j>0)+height*j)),
                                    (i==0)*leftMargin/(length+leftMargin),
                                    (i==n_horiz-1)*rightMargin/(length+rightMargin),
                                    (j==n_vert-1)*bottomMargin/(height+bottomMargin),
                                    (j==0)*topMargin/(height+topMargin)));
    }
  }return pads;
}

//========================================================================== Style Settings

void setStyleLine(TAttLine* line, const char* style) {
  TString str(style);
  str.ToLower();
  Int_t gLineStyle;
  Int_t gLineWidth;
  Int_t gLineColor;
  Float_t gLineAlpha;
  // Line width
  if      (str.Contains("thick"))     gLineWidth = 3;
  else if (str.Contains("thin"))      gLineWidth = 1;
  else                                gLineWidth = 2;
  
  // Line style
  if      (str.Contains("dashed"))    gLineStyle = 7;
  else if (str.Contains("longdash"))  gLineStyle = 9;
  else if (str.Contains("shortdash")) gLineStyle = 2;
  else if (str.Contains("dotted"))    gLineStyle = 3;
  else if (str.Contains("-."))        gLineStyle = 4;
  else if (str.Contains("--."))       gLineStyle = 5;
  else if (str.Contains("---."))      gLineStyle = 9;
  else if (str.Contains("...-"))      gLineStyle = 6;
  else if (str.Contains("..-"))       gLineStyle = 8;
  else                                gLineStyle = 1;
  
  // Line color
  if (str.Contains("gray")
      || str.Contains("grey"))        gLineColor = kGray+1;
  else if (str.Contains("red"))       gLineColor = kRed+2;
  else if (str.Contains("blue"))      gLineColor = kBlue+2;
  else if (str.Contains("orange"))    gLineColor = kOrange+10;
  else if (str.Contains("green"))     gLineColor = kGreen+3;
  else if (str.Contains("sky"))       gLineColor = kAzure+5;
  else if (str.Contains("violet"))    gLineColor = kViolet+7;
  else if (str.Contains("black"))     gLineColor = kBlack;
  
  // Transparency
  if (str.Contains("transparent"))      gLineAlpha = 0.5;
  else if (str.Contains("translucent")) gLineAlpha = 0.75;
  else                                  gLineAlpha = 1.00;
  
  line->SetLineStyle(gLineStyle);
  line->SetLineWidth(gLineWidth);
  line->SetLineColorAlpha(gLineColor, gLineAlpha);
}

void setStyleAxis(TAttAxis* axis,
                  Float_t title_size = 0.04,
                  Float_t title_offset = 1,
                  Float_t label_size = 0.04,
                  Float_t label_offset = 0.01,
                  Float_t tick_length = 0.03,
                  Int_t axis_color = kBlack) {
  if (title_size > 0.2)     std::cout << "Warning in axis format: Large title size." << std::endl;
  if (title_offset < 0.1)   std::cout << "Warning in axis format: Small title offset." << std::endl;
  if (label_size > 0.1)     std::cout << "Warning in axis format: Large label size." << std::endl;
  if (label_offset > 0.1)   std::cout << "Warning in axis format: Large label offset." << std::endl;
  if (tick_length > 0.1)    std::cout << "Warning in axis format: Large tick size." << std::endl;
  
  axis->SetTitleSize(title_size);
  axis->SetTitleOffset(title_offset);
  axis->SetLabelSize(label_size);
  axis->SetLabelOffset(label_offset);
  axis->SetTickSize(tick_length);
  axis->SetAxisColor(axis_color);
}


//========================================================================== Misc. Drawing Tools

// Draw a dashed gray line through unity on the input pad.
void drawUnityLine(TAxis* reference) {
  TLine* unityline = new TLine();
  setStyleLine(unityline, "gray, dashed, thin");
  unityline->DrawLine(reference->GetXmin(), 1, reference->GetXmax(), 1);
  return;
}

// Flip a histogram to its side or upside-down by drawing a TGraph
// Inputs give
//      0, 0 - sideways with origin to the left
//      1, 0 - sideways with origin to the right
//      0, 1 - upside-down (reflected about horizontal)
//      1, 1 - rotated 180 degrees (reflected about both axes)
// Input nodraw can be given to supress drawing, if e.g. graph is needed
// as input for TMultiGraph (many rotated graphs are drawn)
// If drawing is supressed, settings for flipping axes (rx, ry) must be
// manually written into the TGraph options when plotting elsewhere.
TGraph* rotateHist(TH1* hist, bool horiz, bool vert, bool dodraw = true) {
  const int nbin = hist->GetXaxis()->GetNbins();
  double x_edge[2*(nbin+1)+1];
  double y_edge[2*(nbin+1)+1];
  
  // Sweep over data to gather hist edges
  y_edge[0] = hist->GetYaxis()->GetBinLowEdge(1);
  for (int i = 0; i < nbin; ++i) {
    x_edge[2*i] = hist->GetBinLowEdge(i+1);
    x_edge[2*i+1] = x_edge[2*i];
    
    y_edge[2*i+1] = hist->GetBinContent(i+1);
    y_edge[2*(i+1)] = y_edge[2*i+1];
  }
  x_edge[2*nbin] = x_edge[2*nbin-1] + hist->GetBinWidth(nbin);
  x_edge[2*nbin+1] = x_edge[2*nbin];
  x_edge[2*nbin+2] = x_edge[0];
  
  y_edge[2*nbin+1] = y_edge[0];
  y_edge[2*nbin+2] = x_edge[0];
  
  // Construct graph with swaps as necessary to reflect input
  TGraph *reflGraph;
  if (!vert) {
    reflGraph = new TGraph(2*(nbin+1)+1, y_edge, x_edge);
    reflGraph->SetTitle(Form("%s;%s;%s",
                             hist->GetTitle(),
                             hist->GetYaxis()->GetTitle(),
                             hist->GetXaxis()->GetTitle()));
  } else {
    reflGraph = new TGraph(2*(nbin+1)+1, x_edge, y_edge);
    reflGraph->SetTitle(Form("%s;%s;%s",
                             hist->GetTitle(),
                             hist->GetXaxis()->GetTitle(),
                             hist->GetYaxis()->GetTitle()));
  }
  
  // Get Color and other settings from original hist
  reflGraph->SetLineWidth(hist->GetLineWidth());
  reflGraph->SetLineColor(hist->GetLineColor());
  reflGraph->SetLineStyle(hist->GetLineStyle());
  reflGraph->SetFillColor(hist->GetFillColor());
  reflGraph->SetFillStyle(hist->GetFillStyle());
  
  // Draw if not supressed
  if (dodraw) {
    switch(2*horiz+3*vert) {
      case 0: // (0, 0)
        reflGraph->Draw("afl"); break;
      case 2: // (1, 0)
        reflGraph->Draw("afl rx"); break;
      case 3: // (0, 1)
        reflGraph->Draw("afl ry"); break;
      case 5: // (1, 1)
        reflGraph->Draw("afl rx ry"); break;
    }
  }return reflGraph;
}

#endif /* root_draw_tools */
