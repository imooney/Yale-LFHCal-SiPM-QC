#ifndef __Derivate_H__
#define __Derivate_H__


/**
 *
 *  @class  __Derivate
 *  @author Balazs Gyongyosi
 *	@date 2019.03 
 *  @brief  This class calculate the 1st derivative of the input array
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

class Derivate {

 public:

	//constructor    
	Derivate(const unsigned int nInputArray, double *YinputArray, double *XinputArray ){
	_nInputArray = nInputArray;
	_YinputArray = YinputArray;
	_XinputArray = XinputArray;
	
    derivativeArray = new double[nInputArray];
	
	
	//calc derivative
	for(int i=1;i<nInputArray-1;i++){
	derivativeArray[i] = (YinputArray[i+1]-YinputArray[i-1])/(XinputArray[i+1]-XinputArray[i-1]);
	}
	derivativeArray[nInputArray-1] = derivativeArray[nInputArray-2];
	derivativeArray[0] = derivativeArray[1];
  }


  // destructor
  virtual ~Derivate(){
    delete derivativeArray;
  }

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

 

  //! dump the original and derivative array to stdout
  void Print() const {
    std::cout<<"Original\tDerivative"<<std::endl;
    for(uint32_t i=0; i<_nInputArray; i++){
      std::cout<<i+1<<"  "<<_YinputArray[i]<<" "<<derivativeArray[i]<<std::endl;
    }
  }


 private:
  
  unsigned int _nInputArray;
  double *_XinputArray;
  double *_YinputArray;
  double *derivativeArray;

};

#endif /* __Derivate_H__ */
