#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cmath>

using namespace std;

int main (int argc, char* argv[])
{
  ifstream in (argv[1]);
  istringstream ss (argv[2]);
  int numsym;
  ss >> numsym;
  int a, b;
  string line;
  double covered = 0;
  double coveredPerPath = 1;
  int i = 0;
  while (getline (in, line)) {	  
    istringstream iss (line);	
	if (!(iss >> hex >> a >> b)) {
	  break;
	}
	cout << "a = " << a << " b = " << b << endl;
	coveredPerPath *= (b-a)+1;
	i++;
	if (i == numsym) {
	  cout << "Covered values per path: " << coveredPerPath << endl;	
	  covered += coveredPerPath;
	  coveredPerPath = 1;
	  i = 0;
	}	  
  }
 
  cout << "Number of values covered: " << covered << endl;
  cout << "Coverage: " << covered/pow(1025.0,numsym) << endl;
}
