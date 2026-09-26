#include <string>
#include "IVBinaryFile.h"
#include "Header.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TAxis.h"
#include "TLine.h"
#include "TMath.h"
#include "TApplication.h"
#include "CenteredMovingAvgFilter.h"
#include "SavitzkyGolayFilter.h" 
#include "SavitzkyGolay3Der.h" 
#include "Derivate.h" 
#include "TMultiGraph.h"
#include "TPaveText.h"
#include "TPaveLabel.h"
#include "TText.h"
#include <sys/stat.h>
#include "TH1.h"
#include "Plot.h"
#include "Temp.h"
#include "relDerivativeAnalysis.h"
//#include "SecDerivativeAnalysis.h"
//#include "invRelDerivativeAnalysis.h"
//#include "ThirdDerivativeAnalysis.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>


//./IV_analysis /home/balazs/SiPM/IV/data/10/IV/SiPM10_IV_meas_2021-12-03_14.35.20.bin ./data/10
//./IV_analysis ./data/array0/IV/IV_10.bin 


/**
 *
 *  IV analysis V1.0
 *  @author Balazs Gyongyosi
 *  @date 2019.03 - 2019.05
 *  @brief  
 *
 *  
 *  
 *
 */
 

bool is_file_exist(const char *fileName)
{
	bool ret;
    std::ifstream infile(fileName);
	ret = infile.good();
	infile.close();
    return ret;
}

int get_file_size(std::string filename) // path to file
{
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
	if(p_file == NULL)
	    return 0;
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

//------------------------------------Main start-------------------------------------------//

//bool enable_partial_steps_draw = false;
bool enable_common_resoult_file = false;
std::string common_output_dir_result = "";

int main(int argc, char** argv){
	if(argc < 2 || !is_file_exist(argv[1]))
	{
		printf("IV_analysis IV_binary file [common res_file_path]\n");
		return -1;
	}
		
	
	if(argc > 2)
	{
		enable_common_resoult_file = true;
	    common_output_dir_result = argv[2];
	}
	  printf("ok\n");

  IVBinaryFile file(argv[1]);
  Temp *temp = new Temp(&file);
  
  std::string TrayID = file.TrayID();
  uint64_t timestamp = file.TimeStamp();
  
  //int SIPM_ID = file.SIPM_ID();
  std::string SiPM_ID = file.Sipm_ID_String();
  printf("Tray ID:%s, SIPM ID:%s\n",TrayID.c_str(), SiPM_ID.c_str());
  
  //file.Print();  //uncomment to dump IV data to stdout
  // header->Print(); //uncomment to dump header data to stdout
  //temp->PrintAll(); //uncomment to print temperature data
  
//-----------------------make ouput folders and generate path----------------------------------  
  std::string in_path(argv[1]);
 // std::string out_folder(argv[2]);
  
  std::size_t found = in_path.find_last_of("/\\");
  
  std::string in_file_name = in_path.substr(found+1);
  std::string in_folder = in_path.substr(0, found);
  
  found = in_folder.find_last_of("/\\");
  std::string tray_folder = in_folder.substr(0, found);
  
  std::string output_dir_result = tray_folder + "/result";
  mkdir(output_dir_result.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  std::string output_dir_plots = tray_folder + "/plots";
  mkdir(output_dir_plots.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  if(enable_common_resoult_file)
  {
	  mkdir(common_output_dir_result.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }
/*  
  //rel deriv folders
  std::string output_dir_relDeriv = output_dir_IV_res;
  
  output_dir_relDeriv += "/relDerivative";
  mkdir(output_dir_relDeriv.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  std::string output_dir_relDeriv_detailed = output_dir_relDeriv + "/detailed";
  mkdir(output_dir_relDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  output_dir_relDeriv_detailed += "/" + in_file_name;
  mkdir(output_dir_relDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  */
 /*
  //sec deriv folders
  std::string output_dir_secDeriv = output_dir_IV_res;
  
  output_dir_secDeriv += "/secDerivative";
  mkdir(output_dir_secDeriv.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  std::string output_dir_secDeriv_detailed = output_dir_secDeriv + "/detailed";
  mkdir(output_dir_secDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  output_dir_secDeriv_detailed += "/" + in_file_name;
  mkdir(output_dir_secDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
*/
/*
  //inv rel deriv folders
  std::string output_dir_invRelDeriv = output_dir_IV_res;
  
  output_dir_invRelDeriv += "/invRelDerivative";
  mkdir(output_dir_invRelDeriv.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  std::string output_dir_invRelDeriv_detailed = output_dir_invRelDeriv + "/detailed";
  mkdir(output_dir_invRelDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  output_dir_invRelDeriv_detailed += "/" + in_file_name;
  mkdir(output_dir_invRelDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
 */
 
 /* 
  //third deriv folders
  std::string output_dir_thirdDeriv = output_dir_IV_res;
  
  output_dir_thirdDeriv += "/thirdDerivative";
  mkdir(output_dir_thirdDeriv.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  std::string output_dir_thirdDeriv_detailed = output_dir_thirdDeriv + "/detailed";
  mkdir(output_dir_thirdDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  output_dir_thirdDeriv_detailed += "/" + in_file_name;
  mkdir(output_dir_thirdDeriv_detailed.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
 */ 
  
 //---------------------------------------------------------------------------
 
  RelDerivativeAnalysis *relDerivativeAnalysis = new RelDerivativeAnalysis(&file, temp);
  
  //relDerivativeAnalysis->SaveFitPlot(output_dir_relDeriv.c_str());
  
  relDerivativeAnalysis->SaveAllPlot(output_dir_plots.c_str());

  
  
  /*
  SecDerivativeAnalysis * secDerivativeAnalysis = new SecDerivativeAnalysis(&file, temp);
  
  secDerivativeAnalysis->SaveFinalPlot(output_dir_secDeriv.c_str());
  
 secDerivativeAnalysis->SaveAllPlot(output_dir_secDeriv_detailed.c_str());
  */
  
  /*
  InvRelDerivativeAnalysis *invRelDerivativeAnalysis = new InvRelDerivativeAnalysis(&file, temp);
  
  invRelDerivativeAnalysis->SaveFitPlot(output_dir_invRelDeriv.c_str());
  
  invRelDerivativeAnalysis->SaveAllPlot(output_dir_invRelDeriv_detailed.c_str());
  */
  
 /*
  ThirdDerivativeAnalysis * thirdDerivativeAnalysis = new ThirdDerivativeAnalysis(&file, temp);
  
  thirdDerivativeAnalysis->SaveFinalPlot(output_dir_thirdDeriv.c_str());
  
  thirdDerivativeAnalysis->SaveAllPlot(output_dir_thirdDeriv_detailed.c_str());
  */
  
  
  /*
 //------------------------create detailed result file-----------------------------------------
 std::string res_file_path  = output_dir_result + "/IV_result_detailed.txt";
 
 int fsize = get_file_size(res_file_path);
 
 FILE *resultFile = fopen(res_file_path.c_str() ,"a");
	
 
 	
 if(fsize == 0)
 {
	 fprintf(resultFile, "*********************************Header********************************\n");
	 
	 fprintf(resultFile, "ArrayID: %s\n", ArrayID.c_str());
	 fprintf(resultFile, "timestamp: %lu\n", timestamp); //" PRIu64 "
	 fprintf(resultFile, "NFineMeasurements: %d\n" , file.NFineMeasurements());
	 
	 fprintf(resultFile, "analysis method: %.3lf\n" , 1.0);  //IV analysis method 1 relative derivative
	 
	 //IV analysis parameters
	 fprintf(resultFile, "nPreSmooths: %.3lf\n" , (double)relDerivativeAnalysis->Get_nPreSmooths()); 
	 fprintf(resultFile, "preSmoothsWidth: %.3lf\n" , (double)relDerivativeAnalysis->Get_preSmoothsWidth()); 
	 fprintf(resultFile, "nlnSmooths: %.3lf\n" , (double)relDerivativeAnalysis->Get_nlnSmooths()); 
 	 fprintf(resultFile, "lnSmoothsWidth: %.3lf\n" , (double)relDerivativeAnalysis->Get_lnSmoothsWidth()); 
	 fprintf(resultFile, "nderSmooths: %.3lf\n" , (double)relDerivativeAnalysis->Get_nderSmooths()); 
	 fprintf(resultFile, "derSmoothsWidth: %.3lf\n" , (double)relDerivativeAnalysis->Get_derSmoothsWidth()); 
	 fprintf(resultFile, "fit_width: %.3lf\n" , relDerivativeAnalysis->Get_fit_width()); 	 
	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "reserved_par %.3lf\n" , 0.0);  //reserved parameters	 
	 
	 
	 
 }
 
 fprintf(resultFile, "**************************************************************************\n");
 fprintf(resultFile, "******************************* ArrayID:%s***********************************\n", ArrayID.c_str());
 fprintf(resultFile, "******************************* SIPMID:%d ***********************************\n", SIPM_ID);
 
 
 //fprintf(resultFile, "%s\n",in_file_name.c_str() );
 fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "avg temperature 4 nearest initial:%lfC\n", temp->GetAvgFourNearestInitial() );
 //fprintf(resultFile, "avg temperature 4 nearest final:%lfC\n", temp->GetAvgFourNearestFinal());
 fprintf(resultFile, "avg temperature:%lfC\n", temp->GetSIPMTemp() );
 
 //fprintf(resultFile, "IV_temp:%lfC\n", temp->GetIV() );
 //fprintf(resultFile, "SPS_temp:%lfC\n", temp->GetSPS() );
 //fprintf(resultFile, "metal_temp:%lfC\n", temp->GetMetal() );
 //fprintf(resultFile, "SPS_avg_all:%lfC\n", temp->GetAvgSPSAll() );
 
 //fprintf(resultFile, "temperature 4 nearest StdDev initial:%lf\n", temp->GetStdDevFourNearestInitial() );
 //fprintf(resultFile, "temperature 4 nearest StdDev finall:%lf\n", temp->GetStdDevFourNearestFinal() );
 //fprintf(resultFile, "temperature 4 nearest StdDev:%lf\n", temp->GetStdDevAllFourNearest() );
 //fprintf(resultFile, "temperature SPS StdDev all:%lf\n", temp->GetStdDevSPSAll() );
 
 //fprintf(resultFile, "temperature Avg all:%lf\n", temp->GetAvgAll() );
 //fprintf(resultFile, "temperature StdDev all:%lf\n", temp->GetStdDevAll() );
 fprintf(resultFile, "--------------------------------------------------------------------------\n");
 fprintf(resultFile, "relDerivative RAW Vbr:%lfV\n",relDerivativeAnalysis->GetRawVbr());
 fprintf(resultFile, "relDerivative comp_25C Vbr:%lfV\n",relDerivativeAnalysis->GetCompVbr());
 //fprintf(resultFile, "relDerivative comp_IV Vbr:%lfV\n",relDerivativeAnalysis->GetCompVbrIV());
 //fprintf(resultFile, "relDerivative comp_metal Vbr:%lfV\n",relDerivativeAnalysis->GetCompVbrMetal());
 //fprintf(resultFile, "relDerivative comp_SPS Vbr:%lfV\n",relDerivativeAnalysis-> GetCompVbrSPS());

 
 //fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "secDerivative RAW Vbr:%lfV\n",secDerivativeAnalysis->GetRawVbr());
 //fprintf(resultFile, "secDerivative compensated Vbr:%lfV\n",secDerivativeAnalysis->GetCompVbr());
 //fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "invRelDerivative RAW Vbr:%lfV\n",invRelDerivativeAnalysis->GetRawVbr());
 //fprintf(resultFile, "invRelDerivative compensated Vbr:%lfV\n",invRelDerivativeAnalysis->GetCompVbr());
 
 
 //fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "rel-sec delta Vbr:%lfV\n",secDerivativeAnalysis->GetCompVbr() - relDerivativeAnalysis->GetCompVbr());
 //fprintf(resultFile, "rel-invRel delta Vbr:%lfV\n",invRelDerivativeAnalysis->GetCompVbr() - relDerivativeAnalysis->GetCompVbr());

 fprintf(resultFile, "\n");
  
 fclose(resultFile);
 */
 
 //--------------------------create result file-------------------------------------------
 std::string res_file_path  = output_dir_result + "/" + "IV_result.txt";
 std::string res_file_path_2;
 std::string common_res_file_path;
 int fsize_common = 0;
 int fsize_2 = 0;
 FILE *commonResultFile = NULL;
 FILE *resultFile_2 = NULL;
 if(enable_common_resoult_file)
 {
	char filename[1000];
	snprintf(filename, 1000, "/IV_result_%s.txt", SiPM_ID.c_str());
	res_file_path_2  = common_output_dir_result + filename;
	common_res_file_path  = common_output_dir_result + "/IV_result_common.txt";
 }
 
 int fsize =  get_file_size(res_file_path);
 FILE *resultFile = fopen(res_file_path.c_str() ,"a");
 if(enable_common_resoult_file)
 {
	fsize_common =  get_file_size(common_res_file_path);
	commonResultFile = fopen(common_res_file_path.c_str() ,"a");
	fsize_2 =  get_file_size(res_file_path_2);
	printf("\npath!!!!!!!!!!!!!  %s\n\n",res_file_path_2.c_str());
	resultFile_2 = fopen(res_file_path_2.c_str() ,"a");
	
 }
 
 
 
 
		
 if(fsize == 0)
 {
	 fprintf(resultFile, "%s ", TrayID.c_str());
	 fprintf(resultFile, "%s ", SiPM_ID.c_str());
	 fprintf(resultFile, "%lu ", timestamp); //" PRIu64 "
	 fprintf(resultFile, "%d " , file.NFineMeasurements());
	 
	 fprintf(resultFile, "%.3lf " , 1.0);  //IV analysis method 1 relative derivative
	 
	 //IV analysis parameters
	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_nPreSmooths()); 
	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_preSmoothsWidth()); 
	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_nlnSmooths()); 
 	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_lnSmoothsWidth()); 
	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_nderSmooths()); 
	 fprintf(resultFile, "%.3lf " , (double)relDerivativeAnalysis->Get_derSmoothsWidth()); 
	 fprintf(resultFile, "%.3lf " , relDerivativeAnalysis->Get_fit_width()); 	 
	 
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters	 
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile, "%.3lf " , 0.0);  //reserved parameters
	 
	 
	 fprintf(resultFile, "\n");
 }
 
 if(fsize_common == 0 && enable_common_resoult_file)
 {
	 fprintf(commonResultFile, "barcode SiPM IV_temperature IV_temperature_STD_dev Raw_Vbr Comp_Vbr Id1@vbr+3V[nA] Id2@vbr+4V[nA] DarkCurrent_temperature forwardResistance\n");
 }
 
  if(fsize_2 == 0 && enable_common_resoult_file)
 {
	 fprintf(resultFile_2, "TRAY_ID SiPM_ID IV_temperature IV_temperature_STD_dev Raw_Vbr Comp_Vbr Id1@vbr+3V[nA] Id2@vbr+4V[nA] DarkCurrent_temperature forwardResistance\n");
	 
	 fprintf(resultFile_2, "%s ", TrayID.c_str());
	 fprintf(resultFile_2, "%s ", SiPM_ID.c_str());
	 fprintf(resultFile_2, "%lu ", timestamp); //" PRIu64 "
	 fprintf(resultFile_2, "%d " , file.NFineMeasurements());
	 
	 fprintf(resultFile_2, "%.3lf " , 1.0);  //IV analysis method 1 relative derivative
	 
	 //IV analysis parameters
	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_nPreSmooths()); 
	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_preSmoothsWidth()); 
	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_nlnSmooths()); 
 	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_lnSmoothsWidth()); 
	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_nderSmooths()); 
	 fprintf(resultFile_2, "%.3lf " , (double)relDerivativeAnalysis->Get_derSmoothsWidth()); 
	 fprintf(resultFile_2, "%.3lf " , relDerivativeAnalysis->Get_fit_width()); 	 
	 
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters	 
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters	 
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 fprintf(resultFile_2, "%.3lf " , 0.0);  //reserved parameters
	 
	 
	 fprintf(resultFile_2, "\n");
 }
 
	
 fprintf(resultFile, "%s ", TrayID.c_str()); //tray ID

 fprintf(resultFile, "%s ", SiPM_ID.c_str()); 

 //fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "%2.3lf ", temp->GetAvgFourNearestInitial() ); //avg temperature 4 nearest initial:
 //fprintf(resultFile, "%2.3lf ", temp->GetAvgFourNearestFinal()); //avg temperature 4 nearest final:
 //fprintf(resultFile, "%2.3lf ", temp->GetAvgAllFourNearest() ); //avg temperature all 4 nearest:
 
 fprintf(resultFile, "%2.3lf ", temp->GetSIPMTemp() );

 //fprintf(resultFile, "%2.3lf ", temp->GetSPS() );
 //fprintf(resultFile, "%2.3lf ", temp->GetMetal() );
 //fprintf(resultFile, "%2.3lf ", temp->GetAvgSPSAll() );
 //fprintf(resultFile, "%2.3lf ", temp->GetInitialIV() );
 
// fprintf(resultFile, "%2.3lf ", temp->GetStdDevFourNearestInitial() );//temperature 4 nearest StdDev initial:
// fprintf(resultFile, "%2.3lf ", temp->GetStdDevFourNearestFinal() ); //temperature 4 nearest StdDev finall:
 fprintf(resultFile, "%2.3lf ", temp->GetSIPMStdDev() );  
 
 //fprintf(resultFile, "%2.3lf ", temp->GetAvgAll() ); //temperature Avg all:
// fprintf(resultFile, "%2.3lf ", temp->GetStdDevAll() );//temperature StdDev all:
 //fprintf(resultFile, "%2.3lf ", temp->GetStdDevSPSAll() );
 //fprintf(resultFile, "%2.3lf ", temp->GetStdDevInitialIV() );
 
 
 // fprintf(resultFile, "--------------------------------------------------------------------------\n");
 fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetRawVbr()); //relDerivative RAW Vbr:

 fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetCompVbr()); //relDerivative compensated Vbr:
 
 
 //dark current data
 fprintf(resultFile, "%2.1lf ",file.Idark0()*1e9); //dark current at Vbr-5V  [nA]

 fprintf(resultFile, "%2.1lf ",file.Idark1()*1e9); //dark current at Vbr+3V  [nA]

 fprintf(resultFile, "%2.2lf ",file.IdarkStartTemp()); //dark current measurement temperature [C]
 
 
 //forward resistance
 fprintf(resultFile, "%2.2lf ",file.forward_Resistance()); //forward reisitance [ohm]
 
 
// fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetCompVbrIV());
 //fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetCompVbrMetal());
 //fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis-> GetCompVbrSPS());
 // fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "%2.4lf ",secDerivativeAnalysis->GetRawVbr()); //secDerivative RAW Vbr:
 //fprintf(resultFile, "%2.4lf ",secDerivativeAnalysis->GetCompVbr()); //secDerivative compensated Vbr:
 // fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "%2.4lf ",invRelDerivativeAnalysis->GetRawVbr()); //invrelDerivative RAW Vbr:
 //fprintf(resultFile, "%2.4lf ",invRelDerivativeAnalysis->GetCompVbr()); //invrelDerivative compensated Vbr:
 
 fprintf(resultFile, "\n");
 
 fclose(resultFile);
 
 if(enable_common_resoult_file)
 {
	fprintf(commonResultFile, "%s ", TrayID.c_str()); //tray ID
	fprintf(commonResultFile, "%s ", SiPM_ID.c_str()); 
	fprintf(commonResultFile, "%2.3lf ", temp->GetSIPMTemp() );
	fprintf(commonResultFile, "%2.3lf ", temp->GetSIPMStdDev() ); 
	fprintf(commonResultFile, "%2.4lf ",relDerivativeAnalysis->GetRawVbr()); //relDerivative RAW Vbr:
	fprintf(commonResultFile, "%2.4lf ",relDerivativeAnalysis->GetCompVbr()); //relDerivative compensated Vbr:
	fprintf(commonResultFile, "%2.1lf ",file.Idark0()*1e9); //dark current at Vbr+3v  [nA]
	fprintf(commonResultFile, "%2.1lf ",file.Idark1()*1e9); //dark current at Vbr+4v  [nA]
	fprintf(commonResultFile, "%2.2lf ",file.IdarkStartTemp()); //dark current measurement temperature [C]
	fprintf(commonResultFile, "%2.2lf ",file.forward_Resistance()); //forward reisitance [ohm]
	fprintf(commonResultFile, "\n");
	
	fclose(commonResultFile);
	
	fprintf(resultFile_2, "%s ", TrayID.c_str()); //tray ID
	fprintf(resultFile_2, "%s ", SiPM_ID.c_str()); 
	fprintf(resultFile_2, "%2.3lf ", temp->GetSIPMTemp() );
	fprintf(resultFile_2, "%2.3lf ", temp->GetSIPMStdDev() ); 
	fprintf(resultFile_2, "%2.4lf ",relDerivativeAnalysis->GetRawVbr()); //relDerivative RAW Vbr:
	fprintf(resultFile_2, "%2.4lf ",relDerivativeAnalysis->GetCompVbr()); //relDerivative compensated Vbr:
	fprintf(resultFile_2, "%2.1lf ",file.Idark0()*1e9); //dark current at Vbr+3v  [nA]
	fprintf(resultFile_2, "%2.1lf ",file.Idark1()*1e9); //dark current at Vbr+4v  [nA]
	fprintf(resultFile_2, "%2.2lf ",file.IdarkStartTemp()); //dark current measurement temperature [C]
	fprintf(resultFile_2, "%2.2lf ",file.forward_Resistance()); //forward reisitance [ohm]
	fprintf(resultFile_2, "\n");
	
	fclose(resultFile_2);
 }
 

 
 
 /*
  //--------------------------create simple result file-------------------------------------------
 res_file_path  = tray_folder + "/IV_simple_result.txt";
 resultFile = fopen(res_file_path.c_str() ,"a");
	
 //fprintf(resultFile, "%s ",in_file_name.c_str() );
 fprintf(resultFile, "%d ", SIPM_pos); //position:

 fprintf(resultFile, "%2.3lf ", temp->GetAvgAllFourNearest() ); //avg temperature all 4 nearest:
 
 fprintf(resultFile, "%2.3lf ", temp->GetStdDevAllFourNearest() ); //temperature 4 nearest StdDev:
 
 
 fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetRawVbr()); //relDerivative RAW Vbr:
 fprintf(resultFile, "%2.4lf ",relDerivativeAnalysis->GetCompVbr()); //relDerivative compensated Vbr:
 // fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "%2.4lf ",secDerivativeAnalysis->GetRawVbr()); //secDerivative RAW Vbr:
// fprintf(resultFile, "%2.4lf ",secDerivativeAnalysis->GetCompVbr()); //secDerivative compensated Vbr:
 // fprintf(resultFile, "--------------------------------------------------------------------------\n");
 //fprintf(resultFile, "%2.4lf ",invRelDerivativeAnalysis->GetRawVbr()); //invrelDerivative RAW Vbr:
 //fprintf(resultFile, "%2.4lf ",invRelDerivativeAnalysis->GetCompVbr()); //invrelDerivative compensated Vbr:
 
 fprintf(resultFile, "\n");
	
 fclose(resultFile);
 */

  return 0;
}
