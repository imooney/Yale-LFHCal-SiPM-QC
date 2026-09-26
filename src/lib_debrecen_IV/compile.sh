g++ -o ReadSPSData `root-config --cflags` ReadSPSData.cc `root-config --glibs`
g++ -o ProcessSPSData `root-config --cflags` ProcessSPSData.cc `root-config --glibs` -lSpectrum
