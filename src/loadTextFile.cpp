#include<loadTextFile.h>
#include<fstream>

std::string loadFile(std::string fileName){
  std::ifstream f(fileName.c_str());
  if(!f.is_open()){
    std::cerr<<"file: "<<fileName<<" does not exist!"<<std::endl;
    return 0;
  }
  std::string str((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
  f.close();
  return str;
}

