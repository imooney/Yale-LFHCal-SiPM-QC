#ifndef __Plot_H__
#define __Plot_H__

#include "TMultiGraph.h"
#include "TPaveText.h"
#include "TPaveLabel.h"
#include "TText.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TLine.h"
#include "TMath.h"
#include <string>
#include <new>        

/**
 *
 *  @class  __Plot
 *  @author Balazs Gyongyosi
 *	@date 2019.03 - 2019.05
 *  @brief  
 *
 *  
 *  
 *
 */



#ifdef __CINT__
typedef unsigned int uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
#endif

class Plot {

 public:

	//constructor    
	Plot(double *xArray, double* yArray, uint32_t array_size, uint32_t cut_size, const char* title )
	{
		label_x_pos = new (std::nothrow) double[max_labels]; 
		label_y_pos = new (std::nothrow) double[max_labels]; 
		label_text  =  new (std::nothrow) char*[max_labels]; 
		
		ndisplayed_points =  array_size - cut_size * 2;
		cut_size_ = cut_size;
		if(ndisplayed_points  <= 0 || ndisplayed_points > array_size || xArray == NULL || yArray == NULL || label_x_pos == NULL || label_y_pos == NULL || label_text == NULL)
		{
			error = true;
			printf("plot parameter error!!\n");
		}
		else
		{
			Tgraph_ = new TGraph(ndisplayed_points  ,(xArray + cut_size_)   , (yArray  + cut_size_));
			
			
			Tgraph_->SetLineColor(kBlue);
		
			TmultiGraph = new TMultiGraph();
			TmultiGraph-> Add(Tgraph_);
		
			std::string titleString = std::string(title); //+ " non smoothed(blue) smoothed(red)";
			TmultiGraph->SetTitle(titleString.c_str());
			TmultiGraph->GetXaxis()->SetTitle("V");
		   // TmultiGraph->GetYaxis()->SetTitle("A");
			
		}
		
  }
  
 void addSmoothedGraph(double *xArraySm, double* yArraySm)
  {
	  if(!error && xArraySm != NULL && yArraySm != NULL)
	  {
		TgraphSm_ = new TGraph(ndisplayed_points  ,(xArraySm + cut_size_) , (yArraySm + cut_size_));
	 
		TgraphSm_->SetLineColor(kRed);
	  
		TmultiGraph->Add(TgraphSm_);  
	  }
  }
  
  void save(const char * path, const char* fileName)
  {
	 if(!error && path != NULL && fileName != NULL)
	 { 
 
		std::string out_file_path = std::string(path) + "/" + fileName + ".png";
    
		printf("%s\n", out_file_path.c_str());
	
		TCanvas *canvas = new TCanvas("plot_canvas","plot_canvas",0,0,hsize,vsize);
		
		TmultiGraph->Draw("ACP");  //"AC*" 
		//gPad->SetLogy();
	
		
		for(uint32_t i=0;i<label_counter;i++)
		{
			printLabel(label_x_pos[i], label_y_pos[i], label_text[i]);
		}			
		
		canvas->Update();
		printf("%s\n", out_file_path.c_str());
		canvas->SaveAs(out_file_path.c_str());
	
		delete canvas;
	}
  }
  

  void addLabel(const char * text, double label_x, double label_y)
  {
	  
	 if(!error && label_counter < max_labels && text != NULL)
	 { 
		 label_x_pos[label_counter] = label_x;
		 label_y_pos[label_counter] = label_y;

		 label_text[label_counter] = new (std::nothrow) char [strlen(text)+1]; 
		 if(label_text[label_counter] != NULL)
		 {
			strcpy(label_text[label_counter], text);
			label_counter++;
		 }
		 
	 }
  }
  
 TGraph *GetNomralTgraph() const
 {
    return Tgraph_;
 }
 
  TGraph *GetSmoothedTgraph() const
 {
    return TgraphSm_;
 }


  // destructor
  virtual ~Plot()
  {
	//printf(" ~Plot()\n");
	delete TmultiGraph;
	//delete Tgraph_;
	//delete TgraphSm_;
	
	for(uint32_t i=0;i<label_counter;i++)
	{
		delete label_text[i];
	}
	delete label_text;
	delete label_x_pos;
	delete label_y_pos;
	
	
  }

  /*
  // get the smoothed data array
  double* GetDerivative() const {
    double *ret = new double[_nInputArray];
    for(uint32_t i=0; i<_nInputArray; i++){
      ret[i] = derivativeArray[i];
    }
    return ret;
  }

  //! Get derivativeArray array size
  uint32_t GetDerivativeArraySize() const {
    return _nInputArray;
  }

 

 */


 private:
 
 void printLabel(double x, double y, char *txt)
 {
 TText *xlabel = new TText();
 xlabel-> SetNDC();
 xlabel -> SetTextFont(1);
 xlabel -> SetTextColor(1);
 xlabel -> SetTextSize(0.03);
 xlabel -> SetTextAlign(22);
 xlabel -> SetTextAngle(0);
 xlabel -> DrawText(x, y, txt);
 delete xlabel;
 }


  
 bool error = false;
 uint32_t cut_size_ = 0;
 int32_t ndisplayed_points = 0;
 TGraph *Tgraph_ = NULL;
 TGraph *TgraphSm_ = NULL;
 TMultiGraph *TmultiGraph = NULL;
 
 uint32_t hsize = 1920;
 uint32_t vsize = 1080;
 
 const uint32_t max_labels = 10;
 uint32_t label_counter = 0;
 double *label_x_pos = NULL;
 double *label_y_pos = NULL;
 char **label_text = NULL;
};

#endif /* __Plot_H__ */
