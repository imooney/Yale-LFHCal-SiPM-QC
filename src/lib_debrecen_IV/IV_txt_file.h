#ifndef __IV_txt_file_H__
#define __IV_txt_file_H__



#include <fstream>
#include <iostream>
#include "Header.h"
#include "bithelper.h"
using namespace SiPM;

#ifdef __CINT__
typedef unsigned int uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
#endif

class IV_txt_file {

 public:

  //!constructor -- read in the contents of filename into memory
  IV_txt_file(const char* filename)
  {
	_file = NULL;
	char line_buff[1000];
	sipm_num = -1000;
    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL) 
	{
        printf("Error! opening file");
        exit(1);
    }
	
	std::string in_path(filename);
	std::size_t found = in_path.find_last_of("/\\");
	std::string in_file_name = in_path.substr(found+1);
	printf("fn:%s\n", in_file_name.c_str());
	sscanf(in_file_name.c_str(),"SiPM%d_",&sipm_num);
	
	//count lines
	_nfinemeasurements = 0;
	while(1)
	{
		if(fgets(line_buff, 1000, fp)!=NULL ) 
		{
			if(strstr(line_buff,"Start Temperatures"))
			{
				printf("found %d lines\n", _nfinemeasurements);
				rewind(fp);
				break;
			}
		  _nfinemeasurements ++;
	    }
		else
		{
			printf("found %d lines\n", _nfinemeasurements);
			rewind(fp); 
			break;
		}
	}
	
	_nfinemeasurements -= 1;
	fgets(line_buff, 1000, fp); //skip header line
	
	_finevoltage = new double[_nfinemeasurements];
    _finecurrent = new double[_nfinemeasurements];
	for(int i=0;i<_nfinemeasurements;i++)
	{
		if( fgets(line_buff, 1000, fp)!=NULL ) 
		{
			  double SMU_V,SMU_I,DMM_V,COMP_I;
			  sscanf(line_buff,"%lf;%lf;%lf;%lf;", &SMU_V, &SMU_I, &DMM_V, &COMP_I);
			  //printf("%lf\n",DMM_V);
			  
			  _finevoltage[i] = DMM_V;//SMU_V;
			  
			 
			  _finecurrent[i] = SMU_I;
	    }
		else
		{
			printf("file read error!!!\n");
			exit(1);
		}
	}
	
	float res = 10000000*1.05;//9790031.0f;//10000000*1.15;//_finevoltage[1] / _finecurrent[1]; //
		printf("DMM res:%lf\n", res);
		
		for(int i=0;i<_nfinemeasurements;i++)
		{
			_finecurrent[i] = _finecurrent[i] - (_finevoltage[i]/res);
			if(_finecurrent[i] < 0.0) 
				_finecurrent[i] = 0.00000;
			printf("%d curr:%lf\n", i, _finecurrent[i]);
		}
		
	/*
	const int downsample_ratio = 1;
	
	
	_finevoltage = new double[_nfinemeasurements];
    _finecurrent = new double[_nfinemeasurements];
	for(int i=0;i<_nfinemeasurements;i++)
	{
		if( fgets(line_buff, 1000, fp)!=NULL ) 
		{
		  if(i%downsample_ratio == 0)
		  {
			  double SMU_V,SMU_I,DMM_V,COMP_I;
			  sscanf(line_buff,"%lf;%lf;%lf;%lf;", &SMU_V, &SMU_I, &DMM_V, &COMP_I);
			  //printf("%lf\n",DMM_V);
			  
			  _finevoltage[i/downsample_ratio] = DMM_V;
			  _finecurrent[i/downsample_ratio] = COMP_I;
		  }
	    }
		else
		{
			printf("file read error!!!\n");
			exit(1);
		}
	}
	_nfinemeasurements /= downsample_ratio;
   */

	_ncoarsemeasurements = 0;
	_coarsevoltage = NULL;  
	_coarsecurrent = NULL;  
	_nibreakdownvoltage = 0.0;
	_nibreakdowncurrent = 0.0;
	
	 _ntemp = 1;
    _temps_initial = new double[_ntemp];
    _temps_interim = new double[_ntemp];
    _temps_final = new double[_ntemp];
	
    for(uint32_t i=0; i<_ntemp; i++)
	{
      _temps_initial[i] = 20;
    }
    for(uint32_t i=0; i<_ntemp; i++)
	{
      _temps_interim[i] = 20;
    }
    for(uint32_t i=0; i<_ntemp; i++)
	{
      _temps_final[i] = 20;
    }
	  
    _header = new Header(0, "cern", 0, "0", 0);
    fclose(fp);
  }

  //!constructor for fake data
  IV_txt_file(Header* header, uint32_t ncoarse, double* vcoarse, double *icoarse, uint32_t nfine, double* vfine, double* ifine, uint8_t nimethod, double nivbreak, double niibreak,
    uint32_t ntemp, double* temps_initial, double* temps_interim, double* temps_final){
    _header = new Header(*header);
    _ncoarsemeasurements = ncoarse;
    _coarsevoltage = new double[_ncoarsemeasurements];
    _coarsecurrent = new double[_ncoarsemeasurements];
    for(uint32_t i=0; i<_ncoarsemeasurements; i++){
      _coarsevoltage[i] = vcoarse[i];
      _coarsecurrent[i] = icoarse[i];
    }
    _nfinemeasurements = nfine;
    _finevoltage = new double[_nfinemeasurements];
    _finecurrent = new double[_nfinemeasurements];
    for(uint32_t i=0; i<_nfinemeasurements; i++){
      _finevoltage[i] = vfine[i];
      _finecurrent[i] = ifine[i];
    }
    _nibreakdownmethod = nimethod;
    _nibreakdownvoltage = nivbreak;
    _nibreakdowncurrent = niibreak;
    _ntemp = ntemp;
    _temps_initial = new double[_ntemp];
    _temps_interim = new double[_ntemp];
    _temps_final = new double[_ntemp];
    for(uint32_t i=0; i<_ntemp; i++){
      _temps_initial[i] = temps_initial[i];
    }
    for(uint32_t i=0; i<_ntemp; i++){
      _temps_interim[i] = temps_interim[i];
    }
    for(uint32_t i=0; i<_ntemp; i++){
      _temps_final[i] = temps_final[i];
    }
  }

  //! destructor
  virtual ~IV_txt_file(){
	  
    delete _file;
    delete _header;
    delete _coarsevoltage;
    delete _coarsecurrent;
    delete _finevoltage;
    delete _finecurrent;
    delete _temps_initial;
    delete _temps_interim;
    delete _temps_final;
	
  }

  //! get the pointer to the metadata information
  Header* GetHeader() const {
    return _header;
  }

  //! the number of coarse measurements
  uint32_t NCoarseMeasurements() const {
    return _ncoarsemeasurements;
  }

  //! the array of voltage steps in the coarse measurements
  double* CoarseVoltage() const {
    double *ret = new double[_ncoarsemeasurements];
    for(uint32_t i=0; i<_ncoarsemeasurements; i++){
      ret[i] = _coarsevoltage[i];
    }
    return ret;
  }

  //! the array of currents in the coarse measurements
  double* CoarseCurrent() const {
    double *ret = new double[_ncoarsemeasurements];
    for(uint32_t i=0; i<_ncoarsemeasurements; i++){
      ret[i] = _coarsecurrent[i];
    }
    return ret;
  }

  //! a particular voltage in the coarse measurements
  //! if requested bin out of range, garbage -9999 is returned
  double CoarseVoltage(uint32_t i) const {
    if(i<_ncoarsemeasurements)
      return _coarsevoltage[i];
    return -9999.;
  }

  //! a particular currents in the coarse measurements
  //! if requested bin out of range, garbage -9999 is returned
  double CoarseCurrent(uint32_t i) const {
    if(i<_ncoarsemeasurements)
      return _coarsecurrent[i];
    return -9999.;
  }

  //! the number of fine measurements
  uint32_t NFineMeasurements() const {
    return _nfinemeasurements;
  }

  //! the array of voltage steps in the fine measurements
  double* FineVoltage() const {
    double *ret = new double[_nfinemeasurements];
    for(uint32_t i=0; i<_nfinemeasurements; i++){
      ret[i] = _finevoltage[i];
    }
    return ret;
  }

  //! the array of currents in the fine measurements
  double* FineCurrent() const {
    double *ret = new double[_nfinemeasurements];
    for(uint32_t i=0; i<_nfinemeasurements; i++){
      ret[i] = _finecurrent[i];
    }
    return ret;
  }

  //! a particular voltage in the fine measurements
  //! if requested bin out of range, garbage -9999 is returned
  double FineVoltage(uint32_t i) const {
    if(i<_nfinemeasurements)
      return _finevoltage[i];
    return -9999.;
  }

  //! a particular currents in the fine measurements
  //! if requested bin out of range, garbage -9999 is returned
  double FineCurrent(uint32_t i) const {
    if(i<_nfinemeasurements)
      return _finecurrent[i];
    return -9999.;
  }

  //! get the flag for the method used by NI to calculate the breakdown voltage
  uint8_t NIBreakdownMethod() const {
    return _nibreakdownmethod;
  }

  //! get the the breakdown voltage determined by NI
  double NIBreakdownVoltage() const {
    return _nibreakdownvoltage;
  }

  //! get the the current at breakdown voltage determined by NI
  double NIBreakdownCurrent() const {
    return _nibreakdowncurrent;
  }
  
  //! the number of fine measurements
  uint32_t NTemps() const {
    return _ntemp;
  }
  
   int get_sipm_num() const {
    return sipm_num;
  }
  
  //! get the array of initial temperatures
  double* InitialTemp() const {
    double *ret = new double[_ntemp];
    for(uint32_t i=0; i<_ntemp; i++){
      ret[i] = _temps_initial[i];
    }
    return ret;
  }
  
   //! get the array of interim temperatures
  double* InterimTemp() const {
    double *ret = new double[_ntemp];
    for(uint32_t i=0; i<_ntemp; i++){
      ret[i] = _temps_interim[i];
    }
    return ret;
  }
  
   //! get the array of final temperatures
  double* FinalTemp() const {
    double *ret = new double[_ntemp];
    for(uint32_t i=0; i<_ntemp; i++){
      ret[i] = _temps_final[i];
    }
    return ret;
  }

  //! dump the contents of the file onto stdout
  void Print() const {
    _header->Print();
    std::cout<<"Reading "<<(unsigned int)_ncoarsemeasurements<<" coarse voltage scan"<<std::endl;
    for(uint32_t i=0; i<_ncoarsemeasurements; i++){
      std::cout<<"\t"<<_coarsevoltage[i]<<" "<<_coarsecurrent[i]<<std::endl;
    }
    std::cout<<"Reading "<<(unsigned int)_nfinemeasurements<<" fine voltage scan"<<std::endl;
    for(uint32_t i=0; i<_nfinemeasurements; i++){
      std::cout<<"\t"<<_finevoltage[i]<<" "<<_finecurrent[i]<<std::endl;
    }
    std::cout<<"NI breakdown method: "<<(unsigned int)_nibreakdownmethod<<std::endl;
    std::cout<<"NI breakdown voltage: "<<_nibreakdownvoltage<<std::endl;
    std::cout<<"NI breakdown current: "<<_nibreakdowncurrent<<std::endl;
    for(uint32_t i=0; i<_ntemp; i++){
      std::cout<<"Temperature: "<<_temps_initial[i]<<"  "<<_temps_interim[i]<<"  "<<_temps_final[i]<<std::endl;
    }
  }

  void Write(const char* filename){
    _file = new std::fstream(filename,std::ios::out | std::ios::binary);
    WriteData<uint8_t>(_file,_header->Version());
    WriteData<uint32_t>(_file,_ncoarsemeasurements);
    for(uint32_t i=0; i<_ncoarsemeasurements; i++){
      WriteData<double>(_file,_coarsevoltage[i]);
      WriteData<double>(_file,_coarsecurrent[i]);
    }
    WriteData<uint32_t>(_file,_nfinemeasurements);
    for(uint32_t i=0; i<_nfinemeasurements; i++){
      WriteData<double>(_file,_finevoltage[i]);
      WriteData<double>(_file,_finecurrent[i]);
    }
    WriteData<uint8_t>(_file,_nibreakdownmethod);
    WriteData<double>(_file,_nibreakdownvoltage);
    WriteData<double>(_file,_nibreakdowncurrent);
    _header->Write(_file);
    _file->close();
  }

 private:
  std::fstream *_file;
  Header *_header;
  uint32_t _ncoarsemeasurements;
  double *_coarsevoltage;
  double *_coarsecurrent;
  uint32_t _nfinemeasurements;
  double *_finevoltage;
  double *_finecurrent;
  uint8_t _nibreakdownmethod;
  double _nibreakdownvoltage;
  double _nibreakdowncurrent;
  uint32_t _ntemp;
  double *_temps_initial;
  double *_temps_interim;
  double *_temps_final;
  int sipm_num;

};

#endif /* __IV_txt_file_H__ */
