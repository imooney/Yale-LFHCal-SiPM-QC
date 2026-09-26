#ifndef __SecDerivativeAnalysis_H__
#define __SecDerivativeAnalysis_H__

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
 *  @class  __SecDerivativeAnalysis
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

class SecDerivativeAnalysis {

 public:

   //constructor with default parameters
   SecDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_)
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

//lnSmoothsWidth SG filter width 5,7 or 9

//nderSmooths   number of runs of SG filter after first  derivative(), if no filter is required set to 0
 
//derSmoothsWidth   SG filter width 5,7 or 9

//nsecDerSmooths  number of runs of SG filter after sec derivative(), if no filter is required set to 0
 
//secDerSmoothsWidth  SG filter width 5,7 or 9


//----------------------------------------------------------------------------------
*/



    //constructor 
	SecDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_, uint32_t nPreSmooths_, uint32_t preSmoothsWidth_, uint32_t nlnSmooths_, uint32_t lnSmoothsWidth_, uint32_t nderSmooths_, uint32_t derSmoothsWidth_, uint32_t nsecDerSmooths_, uint32_t secDerSmoothsWidth_  )
	{
		IV_file = IV_file_;
		temp = temp_;
		
		nPreSmooths = nPreSmooths_;
        preSmoothsWidth = preSmoothsWidth_;
		nlnSmooths = nlnSmooths_;
		lnSmoothsWidth = lnSmoothsWidth_;
		nderSmooths = nderSmooths_;
		derSmoothsWidth = derSmoothsWidth_;
		nsecDerSmooths = nsecDerSmooths_;
		secDerSmoothsWidth = secDerSmoothsWidth_;
		
		init_function();
	}
  


void SaveFinalPlot(const char* path)
{
	if(!error && secDerPlot != NULL)
	{
		std::string RAW_Vbr_text = "RAW Vbr:" + std::to_string(VBR_RAW) +"V";
		std::string comp_Vbr_text = "Comp Vbr:" +  std::to_string(VBR_COMP) +"V";
		std::string tempAvgLabelText = "avg temp: " + std::to_string(temp->GetAvgAllFourNearest()) +"C";
		std::string tempStdDevLabelText = "temp StdDev: " + std::to_string(temp->GetStdDevAllFourNearest());
	
		secDerPlot->addLabel(RAW_Vbr_text.c_str(), 0.92,0.95);
		secDerPlot->addLabel(comp_Vbr_text.c_str(), 0.92,0.98);
		secDerPlot->addLabel(tempAvgLabelText.c_str(), 0.7,0.98);
		secDerPlot->addLabel(tempStdDevLabelText.c_str(), 0.7,0.95);
		
		std::string filename = std::to_string(position) +  "_sec_der" ;
		
		secDerPlot->save(path, filename.c_str());
	}
}

void SaveAllPlot(const char* path)
{
	if(!error )
	{
		SaveFinalPlot(path);
		
		Plot IV_plot(volatgeArray, currentArray, nmeasurements, iv_plot_cut, "IV curve");
		IV_plot.addSmoothedGraph(volatgeArray, fineCurr_sm);
		std::string filename = std::to_string(position) +  "_IV" ;
		IV_plot.save(path, filename.c_str());
		
		Plot ln_IV_plot(volatgeArray, lnArray, nmeasurements, ln_iv_plot_cut, "ln IV curve");
		ln_IV_plot.addSmoothedGraph(volatgeArray, lnArray_sm);
		filename = std::to_string(position) +  "_ln_IV" ;
		ln_IV_plot.save(path, filename.c_str());

		Plot first_der_plot(volatgeArray, derivativeArray, nmeasurements, first_der__plot_cut, "first der curve");
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
  virtual ~SecDerivativeAnalysis()
  {
	//printf("~SecDerivativeAnalysis()\n");
	
	delete volatgeArray;
	delete currentArray;
	delete fineCurr_sm;
	delete lnArray_sm;
	delete derivativeArray;
	delete derivativeArray_sm;
	delete secDerivativeArray;
	delete secDerivativeArray_sm;
	delete secDerPlot;
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
				if(lnArray != NULL)
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

bool RunCalculations()
{
	if(!error)
	{
		//calculate plot display cuts
		iv_plot_cut = nPreSmooths * ((preSmoothsWidth - 1)/2);
		ln_iv_plot_cut = iv_plot_cut + nlnSmooths * ((lnSmoothsWidth - 1)/2);
	    first_der__plot_cut = ln_iv_plot_cut + nderSmooths * ((derSmoothsWidth - 1)/2);
	    sec_der__plot_cut = first_der__plot_cut + nsecDerSmooths * ((secDerSmoothsWidth - 1)/2);
		
		//apply SavitzkyGolay filter
		fineCurr_sm =  runSGMultiple(nmeasurements, currentArray, preSmoothsWidth,  nPreSmooths);
  
		//calculate ln()
		for(int i=0;i<nmeasurements;i++)
		{
			lnArray[i] = TMath::Log(fineCurr_sm[i]);
		}
  
		//apply SavitzkyGolay filter after ln()
		lnArray_sm = runSGMultiple(nmeasurements, lnArray, lnSmoothsWidth,  nlnSmooths);
  
		//calculate first derivative
		Derivate Derivate_(nmeasurements, lnArray_sm, volatgeArray);
		derivativeArray = Derivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after first derivative
		derivativeArray_sm = runSGMultiple(nmeasurements, derivativeArray, derSmoothsWidth,  nderSmooths); 
		
		//calculate sec derivative
		Derivate secDerivate_(nmeasurements, derivativeArray_sm, volatgeArray);
		secDerivativeArray = secDerivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after sec derivative
		secDerivativeArray_sm = runSGMultiple(nmeasurements, secDerivativeArray, secDerSmoothsWidth,  nsecDerSmooths); 

		secDerPlot = new Plot(volatgeArray, secDerivativeArray, nmeasurements, sec_der__plot_cut, "second derivative");
		if(secDerPlot == NULL)
			return true;
		
		secDerPlot->addSmoothedGraph(volatgeArray, secDerivativeArray_sm);
		
		TGraph *secDerGraphSm =  secDerPlot->GetSmoothedTgraph();
		if(secDerGraphSm == NULL)
			return true;
		
		max_xpos = GetGraphYmaxXpos(secDerGraphSm);
		
		VBR_RAW = max_xpos;
		
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
  double* secDerivativeArray = NULL;
  double* secDerivativeArray_sm = NULL;
  
  double max_xpos = 0;
  
  Plot *secDerPlot = NULL;
  
  
  TGraph *secDerGraphSm = NULL;
  
  TF1 *f1 = NULL;
  
  double VBR_RAW = 0;
  double VBR_COMP = 0;
 
 int iv_plot_cut = 0;
 int ln_iv_plot_cut = 0;
 int first_der__plot_cut = 0;
 int sec_der__plot_cut = 0;
 
 
 
 
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
int nderSmooths = 0; 
//SG filter width 5,7 or 9
int derSmoothsWidth = 9;

//number of runs of SG filter after sec derivative(), if no filter is required set to 0
int nsecDerSmooths = 1; 
//SG filter width 5,7 or 9
int secDerSmoothsWidth = 9;
//----------------------------------------------------------------------------------
  
};

#endif /* __SecDerivativeAnalysis_H__ */
