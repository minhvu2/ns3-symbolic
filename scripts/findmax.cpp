#include <iostream>
#include <fstream>

using namespace std;

int main (int argc, char* argv[])
{
  ifstream in (argv[1]);
  int max = 0;
  int invalue = 0;
  while (in >> hex >> invalue) {
	if (max < invalue) {
	  max = invalue;	
	}  
  }
  cout << "Max updated time: " << max << endl;
}
