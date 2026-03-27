#include <iostream>
#include <fstream>
#include <string>
using namespace std;
int main(){
  ifstream f("config.ini");
  string line;
  int i=0;
  while(getline(f,line) && i<5){
    cout<<"line"<<i<<" len="<<line.size()<<" first="<<(int)(unsigned char)line[0]<<" text="<<line<<"\n";
    i++;
  }
}
