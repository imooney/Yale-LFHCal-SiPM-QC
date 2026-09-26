DEPS = Derivate.h Header.h bithelper.h IVBinaryFile.h CenteredMovingAvgFilter.h SavitzkyGolayFilter.h SavitzkyGolay3Der.h Plot.h Temp.h ThirdDerivativeAnalysis.h SecDerivativeAnalysis.h relDerivativeAnalysis.h invRelDerivativeAnalysis.h

all: IV_analysis

IV_analysis : IV_analysis.cc $(DEPS)
	g++ -o IV_analysis `root-config --cflags` IV_analysis.cc `root-config --glibs`
	

clean: 
	rm IV_analysis