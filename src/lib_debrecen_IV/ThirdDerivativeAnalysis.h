#ifndef __ThirdDerivativeAnalysis_H__
#define __ThirdDerivativeAnalysis_H__

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
 *  @class  __ThirdDerivativeAnalysis
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

class ThirdDerivativeAnalysis {

 public:

   //constructor with default parameters
   ThirdDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_)
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

//nthirdDerSmooths  number of runs of SG filter after sec derivative(), if no filter is required set to 0
 
//thirdDerSmoothsWidth  SG filter width 5,7 or 9


//----------------------------------------------------------------------------------
*/



    //constructor 
	ThirdDerivativeAnalysis(IVBinaryFile *IV_file_, Temp *temp_, uint32_t nPreSmooths_, uint32_t preSmoothsWidth_, uint32_t nlnSmooths_, uint32_t lnSmoothsWidth_, uint32_t nderSmooths_, uint32_t derSmoothsWidth_, uint32_t nsecDerSmooths_, uint32_t secDerSmoothsWidth_, uint32_t nthirdDerSmooths_, uint32_t thirdDerSmoothsWidth_)
	{
		IV_file = IV_file_;
		temp = temp_;
		
		nPreSmooths = nPreSmooths_;
        preSmoothsWidth = preSmoothsWidth_;
		nderSmooths = nderSmooths_;
		derSmoothsWidth = derSmoothsWidth_;
		nsecDerSmooths = nsecDerSmooths_;
		secDerSmoothsWidth = secDerSmoothsWidth_;
		nthirdDerSmooths = nthirdDerSmooths_;
		thirdDerSmoothsWidth = thirdDerSmoothsWidth_;
		
		init_function();
	}
  


void SaveFinalPlot(const char* path)
{
	if(!error && thirdDerPlot != NULL)
	{
		std::string RAW_Vbr_text = "RAW Vbr:" + std::to_string(VBR_RAW) +"V";
		std::string comp_Vbr_text = "Comp Vbr:" +  std::to_string(VBR_COMP) +"V";
		std::string tempAvgLabelText = "avg temp: " + std::to_string(temp->GetAvgAllFourNearest()) +"C";
		std::string tempStdDevLabelText = "temp StdDev: " + std::to_string(temp->GetStdDevAllFourNearest());
		
		std::string fitV01LabelText = "fit V01: " + std::to_string(fit_V01);
		std::string fithLabelText = "fit h: " + std::to_string(fit_h);
	
		thirdDerPlot->addLabel(RAW_Vbr_text.c_str(), 0.92,0.95);
		thirdDerPlot->addLabel(comp_Vbr_text.c_str(), 0.92,0.98);
		thirdDerPlot->addLabel(tempAvgLabelText.c_str(), 0.7,0.98);
		thirdDerPlot->addLabel(tempStdDevLabelText.c_str(), 0.7,0.95);
		
		thirdDerPlot->addLabel(fitV01LabelText.c_str(), 0.2,0.95);
		thirdDerPlot->addLabel(fithLabelText.c_str(), 0.2,0.98);
		
		std::string filename = std::to_string(position) +  "_third_der" ;
		
		thirdDerPlot->save(path, filename.c_str());
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

		Plot first_der_plot(volatgeArray, derivativeArray, nmeasurements, first_der__plot_cut, "first der curve");
		first_der_plot.addSmoothedGraph(volatgeArray, derivativeArray_sm);
		filename = std::to_string(position) +  "_first_der" ;
		first_der_plot.save(path, filename.c_str());

		Plot sec_der_plot(volatgeArray, secDerivativeArray, nmeasurements, sec_der__plot_cut, "sec der curve");
		sec_der_plot.addSmoothedGraph(volatgeArray, secDerivativeArray_sm);
		filename = std::to_string(position) +  "_sec_der" ;
		sec_der_plot.save(path, filename.c_str());
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

double GetFith()
{
	return fit_h;
}

double GetFitV01()
{
	return fit_V01;
}


  // destructor
  virtual ~ThirdDerivativeAnalysis()
  {
	//printf("~ThirdDerivativeAnalysis()\n");
	
	delete volatgeArray;
	delete currentArray;
	delete fineCurr_sm;
	delete derivativeArray;
	delete derivativeArray_sm;
	delete secDerivativeArray;
	delete secDerivativeArray_sm;
	delete thirdDerivativeArray;
	delete thirdDerivativeArray_sm;
	delete thirdDerPlot;
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
	    first_der__plot_cut = iv_plot_cut + nderSmooths * ((derSmoothsWidth - 1)/2);
	    sec_der__plot_cut = first_der__plot_cut + nsecDerSmooths * ((secDerSmoothsWidth - 1)/2);
		third_der__plot_cut = sec_der__plot_cut + nthirdDerSmooths * ((thirdDerSmoothsWidth - 1)/2);
		
		//apply SavitzkyGolay filter
		fineCurr_sm =  runSGMultiple(nmeasurements, currentArray, preSmoothsWidth,  nPreSmooths);
  
		//calculate first derivative
		Derivate Derivate_(nmeasurements, fineCurr_sm, volatgeArray);
		derivativeArray = Derivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after first derivative
		derivativeArray_sm = runSGMultiple(nmeasurements, derivativeArray, derSmoothsWidth,  nderSmooths); 
		
		//calculate sec derivative
		Derivate secDerivate_(nmeasurements, derivativeArray, volatgeArray);
		secDerivativeArray = secDerivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after sec derivative
		secDerivativeArray_sm = runSGMultiple(nmeasurements, secDerivativeArray, secDerSmoothsWidth,  nsecDerSmooths); 
		
		//calculate third derivative
		Derivate thirdDerivate_(nmeasurements, secDerivativeArray_sm, volatgeArray);
		thirdDerivativeArray = thirdDerivate_.GetDerivative();
  
		//apply SavitzkyGolay filter after third derivative
		thirdDerivativeArray_sm = runSGMultiple(nmeasurements, thirdDerivativeArray, secDerSmoothsWidth,  nsecDerSmooths); 

		thirdDerPlot = new Plot(volatgeArray, thirdDerivativeArray, nmeasurements, third_der__plot_cut, "third derivative");
		if(thirdDerPlot == NULL)
			return true;
		
		thirdDerPlot->addSmoothedGraph(volatgeArray, thirdDerivativeArray_sm);
		
		TGraph *thirdDerGraphSm =  thirdDerPlot->GetSmoothedTgraph();
		if(thirdDerGraphSm == NULL)
			return true;
		
		max_xpos = GetGraphYmaxXpos(thirdDerGraphSm);
		
		//printf("max XPos:%lf\n", max_xpos);
		
		double fit_start = volatgeArray[third_der__plot_cut  + 1];
		double fit_end   = volatgeArray[nmeasurements - third_der__plot_cut - 2];

	    TF1 *f1 = new TF1("f1","[0] * (2-([1]*(x-[3]))/([2]^2)) * exp(-1*(((x-[3])*(x-[3]))/(2*([2]^2))))", fit_start, fit_end);  //  |0 A| |1 h| |2 sigma| |3 V01|  65-68,3 fit width
		f1->SetParameters(0.0001,0.8,0.17,max_xpos);  //initial parameters
		thirdDerGraphSm->Fit("f1","RM");
		fit_h   = thirdDerGraphSm->GetFunction("f1")->GetParameter(1);
		fit_V01 = thirdDerGraphSm->GetFunction("f1")->GetParameter(3);
		
		VBR_RAW = fit_V01 - fit_h;
		
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
  double *derivativeArray =  NULL;
  double *derivativeArray_sm =  NULL;
  double* secDerivativeArray = NULL;
  double* secDerivativeArray_sm = NULL;
  double* thirdDerivativeArray = NULL;
  double* thirdDerivativeArray_sm = NULL;
  
  double max_xpos = 0;
  
  Plot *thirdDerPlot = NULL;
  
  
  TGraph *thirdDerGraphSm = NULL;
  
  TF1 *f1 = NULL;
  
  double fit_h = 0;
  double fit_V01 = 0;
  double VBR_RAW = 0;
  double VBR_COMP = 0;
 
 int iv_plot_cut = 0;
 int first_der__plot_cut = 0;
 int sec_der__plot_cut = 0;
 int third_der__plot_cut = 0;
 
 
 
 
//---------------------------------------------------------------------------------- 
//default settings, modifiable through constructor
//number of runs of SG filter on original IV curve before ln(), if no filter is required set to 0
int nPreSmooths = 1;
//SG filter width 5,7 or 9
int   preSmoothsWidth = 9; 


//number of runs of SG filter after first derivative(), if no filter is required set to 0
int nderSmooths = 0; 
//SG filter width 5,7 or 9
int derSmoothsWidth = 9;

//number of runs of SG filter after sec derivative(), if no filter is required set to 0
int nsecDerSmooths = 2; 
//SG filter width 5,7 or 9
int secDerSmoothsWidth = 9;

//number of runs of SG filter after third der if no filter is required set to 0
int nthirdDerSmooths = 3; 
//SG filter width 5,7 or 9
int thirdDerSmoothsWidth = 9;
//----------------------------------------------------------------------------------
  
};

#endif /* __ThirdDerivativeAnalysis_H__ */
