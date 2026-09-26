#ifndef __invRelDerivativeAnalysis_H__
#define __invRelDerivativeAnalysis_H__

#include "IVBinaryFile.h"
#include "Header.h"
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
#include "TGraph.h"  
#include "Header.h"

/**
 *
 *  @class  __invRelDerivativeAnalysis
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

class InvRelDerivativeAnalysis {

 public:

   //constructor with default parameters
   InvRelDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_)
  {
		IV_file = IV_file_;
		temp = temp_;
		
		init_function();
  }
  
/*
//---------------------------------------------------------------------------------- 
// nPreSmooths   number of runs of SG filter on original IV curve before ln(), if no filter is required set to 0

//preSmoothsWidth  SG filter width 5,7 or 9

//nlnSmooths  number of runs of SG filter after ln(), if no filter is required set to 0

//lnSmoothsWidthSG filter width 5,7 or 9

//nderSmooths   number of runs of SG filter after derivative(), if no filter is required set to 0
 
//derSmoothsWidth   SG filter width 5,7 or 9

//fit_length  fited line length 

//fit_start fited line start position from the curve minimum point


//----------------------------------------------------------------------------------
*/
    //constructor 
	InvRelDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_, uint32_t nPreSmooths_, uint32_t preSmoothsWidth_, uint32_t nlnSmooths_, uint32_t lnSmoothsWidth_, uint32_t nderSmooths_, uint32_t derSmoothsWidth_, double fit_length_, double fit_start_   )
	{
		IV_file = IV_file_;
		temp = temp_;
		
		nPreSmooths = nPreSmooths_;
        preSmoothsWidth = preSmoothsWidth_;
		nlnSmooths = nlnSmooths_;
		lnSmoothsWidth = lnSmoothsWidth_;
		nderSmooths = nderSmooths_;
		derSmoothsWidth = derSmoothsWidth_;
		fit_length = fit_length_;
		fit_start = fit_start_;
		
		init_function();
	}
  


void SaveFitPlot(const char* path)
{
	if(!error && invRelDerPlot != NULL)
	{
		std::string RAW_Vbr_text = "RAW Vbr:" + std::to_string(VBR_RAW) +"V";
		std::string comp_Vbr_text = "Comp Vbr:" +  std::to_string(VBR_COMP) +"V";
		std::string tempAvgLabelText = "avg temp: " + std::to_string(temp->GetAvgAllFourNearest()) +"C";
		std::string tempStdDevLabelText = "temp StdDev: " + std::to_string(temp->GetStdDevAllFourNearest());
	
		invRelDerPlot->addLabel(RAW_Vbr_text.c_str(), 0.92,0.95);
		invRelDerPlot->addLabel(comp_Vbr_text.c_str(), 0.92,0.98);
		invRelDerPlot->addLabel(tempAvgLabelText.c_str(), 0.72,0.98);
		invRelDerPlot->addLabel(tempStdDevLabelText.c_str(), 0.72,0.95);
		
		std::string filename = std::to_string(position) +  "_inv_rel_der_fit" ;
		
		invRelDerPlot->save(path, filename.c_str());
	}
}

void SaveAllPlot(const char* path)
{
	if(!error )
	{
		SaveFitPlot(path);
		
		Plot IV_plot(volatgeArray, currentArray, nmeasurements, iv_plot_cut, "IV curve");
		IV_plot.addSmoothedGraph(volatgeArray, fineCurr_sm);
		std::string filename = std::to_string(position) +  "_IV" ;
		IV_plot.save(path, filename.c_str());
		
		Plot ln_IV_plot(volatgeArray, lnArray, nmeasurements, ln_iv_plot_cut, "ln IV curve");
		ln_IV_plot.addSmoothedGraph(volatgeArray, lnArray_sm);
		filename = std::to_string(position) +  "_ln_IV" ;
		ln_IV_plot.save(path, filename.c_str());

		Plot first_der_plot(volatgeArray, derivativeArray, nmeasurements, der_plot_cut, "first der curve");
		first_der_plot.addSmoothedGraph(volatgeArray, derivativeArray_sm);
		filename = std::to_string(position) +  "_first_der" ;
		first_der_plot.save(path, filename.c_str());	
	}
}
  
  
double GetRawVbr()
{
	return VBR_RAW;
}

double GetCompVbr()
{
	return VBR_COMP;
}


  // destructor
  virtual ~InvRelDerivativeAnalysis()
  {
	//printf("~InvRelDerivativeAnalysis()\n");
	
	delete volatgeArray;
	delete currentArray;
	delete fineCurr_sm;
	delete lnArray_sm;
	delete derivativeArray;
	delete derivativeArray_sm;
	delete invRelDerPlot;
	delete invDerivativeArray;
	delete f1;
  }

  

 private:
 
//------------------class private functions---------------------------------
void init_function()
{
	if(IV_file != NULL && temp != NULL)
		{
			header = IV_file->GetHeader();
			nmeasurements = IV_file->NFineMeasurements();
			volatgeArray = IV_file->FineVoltage();
			currentArray = IV_file->FineCurrent();
			position = header->Position();
			
			if(nmeasurements != 0 && volatgeArray != NULL && currentArray != NULL && header != NULL)
			{
				lnArray = new (std::nothrow) double[nmeasurements];
				invDerivativeArray = new (std::nothrow) double[nmeasurements];
				if(lnArray != NULL && invDerivativeArray != NULL)
				{
					//temp = new Temp(IV_file);
					if(RunCalculations())
					{
						error = true;
					}
				}
				else
				{
					error = true;
				}
			}
			else
			{
				error = true;
			}
		
		}
		else
		{
			error = true;
		}
}

//run savitzky-Golay filter 'nruns' time on the inputArray 
// filterWidth it must be 5,7 or 9
// if nruns 0 this function return with the original array
double *runSGMultiple(int inArraySize, double* inArray,int filterWidth,  int nruns)
{
  double *ret = inArray;
  for(int i=0;i<nruns;i++)
  {
	SavitzkyGolayFilter smooth(inArraySize, ret, filterWidth );  //filter width 5,7, or 9
	if(i!=0){
	delete ret;
	}
	ret = smooth.GetSmoothed();
  }
  return ret;
}

double GetGraphYmaxXpos(TGraph *gr){
	return gr->GetX()[ TMath::LocMax(gr->GetN(),gr->GetY()) ];
}

double GetGraphYminXpos(TGraph *gr){
	return gr->GetX()[ TMath::LocMin(gr->GetN(),gr->GetY()) ];
}

bool RunCalculations()
{
	if(!error)
	{
		//calculate plot display cuts
		iv_plot_cut = nPreSmooths * ((preSmoothsWidth - 1)/2);
		ln_iv_plot_cut = iv_plot_cut + nlnSmooths * ((lnSmoothsWidth - 1)/2);
	    der_plot_cut = ln_iv_plot_cut + nderSmooths * ((derSmoothsWidth - 1)/2) +1;

		//apply SavitzkyGolay filter
		fineCurr_sm =  runSGMultiple(nmeasurements, currentArray, preSmoothsWidth,  nPreSmooths);
  
		//calculate ln()
		for(int i=0;i<nmeasurements;i++)
		{
			lnArray[i] = TMath::Log(fineCurr_sm[i]);
		}
  
		//apply SavitzkyGolay filter after ln()
		lnArray_sm = runSGMultiple(nmeasurements, lnArray, lnSmoothsWidth,  nlnSmooths);
  
		//calculate derivative
		Derivate Derivate_(nmeasurements, lnArray_sm, volatgeArray);
		derivativeArray = Derivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after derivative
		derivativeArray_sm = runSGMultiple(nmeasurements, derivativeArray, derSmoothsWidth,  nderSmooths); 
		
		//calculate inverse derivative
		for(int i=0;i<nmeasurements;i++)
		{
			invDerivativeArray[i] = 1.0/derivativeArray_sm[i];
		}
  
		invRelDerPlot = new Plot(volatgeArray, invDerivativeArray, nmeasurements, der_plot_cut , "inv relative derivative");
		if(invRelDerPlot == NULL)
			return true;
		
		
		TGraph *invRelDerGraph =  invRelDerPlot->GetNomralTgraph();
		if(invRelDerGraph == NULL)
			return true;
		
		min_xpos = GetGraphYminXpos(invRelDerGraph);
		
		//printf("min XPos:%lf\n", min_xpos);

		f1 = new TF1("f1","pol1",min_xpos+fit_start, min_xpos+fit_start+fit_length);
		
		invRelDerGraph->Fit("f1","R");
	
		VBR_RAW = (invRelDerGraph->GetFunction("f1")->GetParameter(0) / invRelDerGraph->GetFunction("f1")->GetParameter(1)) *-1.0;
		
		VBR_COMP = temp->calcBreakdownTo25C_fourNearest(VBR_RAW);
	}	
}
 
 
 //------------------class private variables---------------------------------
 bool error = false;
 
 IVBinaryFile *IV_file = NULL;
 Header *header = NULL;
 Temp *temp = NULL;
 
  uint8_t position = 0;
  uint32_t nmeasurements = 0;
  double* volatgeArray = NULL;
  double* currentArray = NULL;
  
  double *fineCurr_sm =  NULL;
  double *lnArray = NULL;
  double *lnArray_sm =  NULL;
  double *derivativeArray =  NULL;
  double *derivativeArray_sm =  NULL;
  double *invDerivativeArray = NULL;
  
  double min_xpos = 0;
  
  Plot *invRelDerPlot = NULL;
  
  
  TGraph *invRelDerGraph = NULL;
  
  TF1 *f1 = NULL;
  
  double VBR_RAW = 0;
  double VBR_COMP = 0;
  
 int iv_plot_cut = 0;
 int ln_iv_plot_cut = 0;
 int der_plot_cut = 0;
 
 
 
//---------------------------------------------------------------------------------- 
//default settings, modifiable through constructor
//number of runs of SG filter on original IV curve before ln(), if no filter is required set to 0
int nPreSmooths = 1;
//SG filter width 5,7 or 9
int   preSmoothsWidth = 9; 

//number of runs of SG filter after ln(), if no filter is required set to 0
int nlnSmooths = 0; 
//SG filter width 5,7 or 9
int lnSmoothsWidth = 9;

//number of runs of SG filter after derivative(), if no filter is required set to 0
int nderSmooths = 1; 
//SG filter width 5,7 or 9
int derSmoothsWidth = 5;
//fited line length 
double fit_length = 1;
//fited line start position from the curve minimum point
double fit_start = 0.3;

//----------------------------------------------------------------------------------
  
};

#endif /* __invRelDerivativeAnalysis_H__ */
