#ifndef __SavitzkyGolay3Der_H__
#define __SavitzkyGolay3Der_H__


/**
 *
 *  @class  __SavitzkyGolay3Der_H__
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

class SavitzkyGolay3Der {

 public:

	//constructor    filter size is 5, 7 or 9
	SavitzkyGolay3Der(const unsigned int nInputArray, double *inputArray, double *XinputArray, const unsigned short filterSize ){
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
	
	avgXDiff = 0;
	for(int i=0;i<nInputArray-1;i++){
		avgXDiff += XinputArray[i+1] - XinputArray[i]; 	
	}
	avgXDiff  /= (nInputArray-1);
	//printf("\navgXDiff:%lf\n\n", avgXDiff);
	
    thirdDerArray = new double[nInputArray];
	unsigned int halfPoints = (filterSize-1)/2;
	unsigned int startIndex = halfPoints;
	unsigned int endIndex   = nInputArray - halfPoints;
	
	for(int i=startIndex; i<endIndex;i++)
	{
		double accu = 0;
		for(int j=0;j<filterSize;j++)
		{
			accu += coeffTable[j] * inputArray[i-halfPoints+j];
		}
		thirdDerArray[i] = accu / (normValue*TMath::Power(avgXDiff, 3));
	}
	
	for(int i=0; i<startIndex;i++)
	{
		thirdDerArray[i] = thirdDerArray[startIndex];
	}
	
	for(int i=endIndex; i<nInputArray;i++)
	{
		thirdDerArray[i] = thirdDerArray[endIndex-1];
	}
   
  }


  // destructor
  virtual ~SavitzkyGolay3Der(){
    delete thirdDerArray;
  }

  // get the smoothed data array
  double* GetSmoothed() const {
    double *ret = new double[_nInputArray];
    for(uint32_t i=0; i<_nInputArray; i++){
      ret[i] = thirdDerArray[i];
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
      std::cout<<i+1<<"  "<<_inputArray[i]<<" "<<thirdDerArray[i]<<std::endl;
    }
  }


 private:
  
  unsigned int _nInputArray;
  unsigned short _filterSize;
  double *_inputArray;
  double *thirdDerArray;
  double avgXDiff = 0;
  
  double coeffTable5[5] = {-1,2,0,-2,1};
  double coeffTable7[7] = {-1,1,1,0,-1,-1,1};
  double coeffTable9[9] = {-14,7,13,9,0,-9,-13,-7,14};
  const double normalisation5 = 2;
  const double normalisation7 = 6;
  const double normalisation9 = 198;
};

#endif /* __SavitzkyGolay3Der_H__ */
