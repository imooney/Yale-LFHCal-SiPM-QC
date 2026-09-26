#ifndef __CenteredMovingAvgFilter_H__
#define __CenteredMovingAvgFilter_H__


/**
 *
 *  @class  __CenteredMovingAvgFilter
 *  @author Balazs Gyongyosi
 *	@date 2019.03 - 2019.05
 *  @brief  This class implement a simple centered moving average filter  to smooth IV curves
 *
 *  filter size is an odd number and greater or equal than 3
 *  input array size is greater than filterSizer+1
 *
 */



#ifdef __CINT__
typedef unsigned int uint8_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;
#endif

class CenteredMovingAvgFilter {

 public:

	//constructor    
	CenteredMovingAvgFilter(const unsigned int nInputArray, double *inputArray, const unsigned short filterSize ){
	_nInputArray = nInputArray;
	_inputArray = inputArray;
	_filterSize = filterSize;
	if(_filterSize < 3) _filterSize = 3;
	if(_filterSize%2 == 0) _filterSize++;
	
    smothedArray = new double[nInputArray];
	unsigned int halfPoints = (filterSize-1)/2;
	unsigned int startIndex = halfPoints;
	unsigned int endIndex   = nInputArray - halfPoints;
	for(int i=startIndex; i<endIndex;i++)
	{
		double accu = 0;
		for(int j=i-halfPoints;j<i+halfPoints+1;j++)
		{
			accu +=  inputArray[j];
		}
		smothedArray[i] = accu / filterSize;
	}
	
	for(int i=0; i<startIndex;i++)
	{
		smothedArray[i] = inputArray[i];
	}
	
	for(int i=endIndex; i<nInputArray;i++)
	{
		smothedArray[i] = inputArray[i];
	}
   
  }


  // destructor
  virtual ~CenteredMovingAvgFilter(){
    delete smothedArray;
  }

  // get the smoothed data array
  double* GetSmoothed() const {
    double *ret = new double[_nInputArray];
    for(uint32_t i=0; i<_nInputArray; i++){
      ret[i] = smothedArray[i];
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
      std::cout<<i+1<<"  "<<_inputArray[i]<<" "<<smothedArray[i]<<std::endl;
    }
  }


 private:
  
  unsigned int _nInputArray;
  unsigned short _filterSize;
  double *_inputArray;
  double *smothedArray;

};

#endif /* __CenteredMovingAvgFilter_H__ */
