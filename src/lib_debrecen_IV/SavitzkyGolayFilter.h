#ifndef __SavitzkyGolayFilter_H__
#define __SavitzkyGolayFilter_H__


/**
 *
 *  @class  __SavitzkyGolayFilter
 *  @author Balazs Gyongyosi
 *	@date 2019.03 - 2019.05
 *  @brief  This class implement a SavitzkyGolayFilter to get third derivative from iv curve
 *
 *
 */



#ifdef __CINT__
typedef unsigned int uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
#endif

class SavitzkyGolayFilter {

 public:

	//constructor    filter size is 5, 7 or 9
	SavitzkyGolayFilter(const unsigned int nInputArray, double *inputArray, const unsigned short filterSize ){
	_nInputArray = nInputArray;
	_inputArray = inputArray;
	_filterSize = filterSize;
	if(_filterSize < 5 || _filterSize > 9) _filterSize = 5;
	
	double *coeffTable = NULL;
	double normValue = 1;
	if(_filterSize == 5) {
		coeffTable = coeffTable5;
		normValue  = normalisation5;
	}
	else if(_filterSize == 7) {
		coeffTable = coeffTable7;
		normValue  = normalisation7;
	}
	else if(_filterSize == 9) {
		coeffTable = coeffTable9;
		normValue  = normalisation9;
	}
	
    
	unsigned int halfPoints = (_filterSize-1)/2;
	unsigned int startIndex = halfPoints;
	unsigned int endIndex   = nInputArray - halfPoints;
	
	nExtendedInputArray = nInputArray + halfPoints*2;
	
	filteredArray = new double[nInputArray];
	ExtendedInputArray = new double[nExtendedInputArray];
	
	for(int i=0;i<nInputArray;i++){
		ExtendedInputArray[startIndex+i] = inputArray[i];
	}
	
	for(int i=0; i<startIndex;i++)  //extend the array start
	{
		ExtendedInputArray[i] = inputArray[startIndex-i];
	}
	
	for(int i=0; i<startIndex;i++)  //extend the array end
	{
		ExtendedInputArray[startIndex+nInputArray+i] = inputArray[nInputArray-1-1-i];
	}
	
	
	for(int i=0; i<nInputArray;i++)
	{
		double accu = 0;
		for(int j=0;j<_filterSize;j++)
		{
			accu += coeffTable[j] * ExtendedInputArray[i+j];
		}
		filteredArray[i] = accu / normValue;
	}
  }


  // destructor
  virtual ~SavitzkyGolayFilter(){
    delete filteredArray;
  }

  // get the smoothed data array
  double* GetSmoothed() const {
    double *ret = new double[_nInputArray];
    for(uint32_t i=0; i<_nInputArray; i++){
      ret[i] = filteredArray[i];
    }
    return ret;
  }

  //! GetSmoothed array size
  uint32_t GetSmoothedArraySize() const {
    return _nInputArray;
  }

 

  //! dump the original and smoothed array to stdout
  void Print() const {
    std::cout<<"Original\tSmoothed"<<std::endl;
    for(uint32_t i=0; i<_nInputArray; i++){
      std::cout<<i+1<<"  "<<_inputArray[i]<<" "<<filteredArray[i]<<std::endl;
    }
  }


 private:
  
  unsigned int _nInputArray;
  unsigned int nExtendedInputArray;
  unsigned short _filterSize;
  double *_inputArray;
  double *ExtendedInputArray;
  double *filteredArray;
  double avgXDiff = 0;
  
  double coeffTable5[5] = {-3,12,17,12,-3};
  double coeffTable7[7] = {-2,3,6,7,6,3,-2};
  double coeffTable9[9] = {-21,14,39,54,59,54,39,14,-21};
  const double normalisation5 = 35;
  const double normalisation7 = 21;
  const double normalisation9 = 231;
};

#endif /* SavitzkyGolayFilter */
