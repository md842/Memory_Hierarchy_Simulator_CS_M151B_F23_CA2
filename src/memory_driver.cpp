#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <iomanip>
#include "cache.h"

using namespace std;

struct trace{
	bool MemR; 
	bool MemW; 
	int adr; 
	int data; 
};

int main (int argc, char* argv[]){ // ./program <filename> <mode>
	string filename = argv[1]; // Input file (i.e., test.txt)

	ifstream fin;

	// Opening file
	fin.open(filename.c_str());
	if (!fin){ // Making sure the file is correctly opened
		cout << "Error opening " << filename << endl;
		exit(1);
	}
	
	// Reading the text file
	string line;
	vector<trace> myTrace;
	int TraceSize = 0;
	string s1, s2, s3, s4;
  while(getline(fin, line)){
    stringstream ss(line);
    getline(ss, s1, ','); 
    getline(ss, s2, ','); 
    getline(ss, s3, ','); 
    getline(ss, s4, ',');
    myTrace.push_back(trace()); 
    myTrace[TraceSize].MemR = stoi(s1);
    myTrace[TraceSize].MemW = stoi(s2);
    myTrace[TraceSize].adr = stoi(s3);
    myTrace[TraceSize].data = stoi(s4);
    TraceSize += 1;
  }

	// Defining cache and stat
  cache myCache;
  int myMem[MEM_SIZE];

	int traceCounter = 0;
	bool cur_MemR; 
	bool cur_MemW; 
	int cur_adr;
	int cur_data;

	// Main loop; reads and parses instructions from the trace, then invokes memory controller
	while(traceCounter < TraceSize){
		cur_MemR = myTrace[traceCounter].MemR;
		cur_MemW = myTrace[traceCounter].MemW;
		cur_data = myTrace[traceCounter].data;
		cur_adr = myTrace[traceCounter].adr;
		traceCounter += 1;
		myCache.controller(cur_MemR, cur_MemW, &cur_data, cur_adr, myMem);
	}
	
	// Compute the stats here:
  double L1_miss_rate = myCache.getMissL1() / myCache.getAccL1();
  double VC_miss_rate = myCache.getMissVic() / myCache.getAccVic();
  double L2_miss_rate = myCache.getMissL2() / myCache.getAccL2();
  int L1_hit_time = 1; // Given by project spec
  int VC_hit_time = 1; // Given by project spec
  int L2_hit_time = 8; // Given by project spec
  double L2_miss_penalty = 100; // Given by project spec
  double VC_miss_penalty = L2_hit_time + (L2_miss_rate * L2_miss_penalty);
  double L1_miss_penalty = VC_hit_time + (VC_miss_rate * VC_miss_penalty);
  double AAT = L1_hit_time + (L1_miss_rate * L1_miss_penalty);

  // Autograder expects precision 10 and specific output format (L1 miss rate, L2 missrate, AAT)
	cout << setprecision(10) << "(" << L1_miss_rate << "," << L2_miss_rate << "," << AAT << ")" << endl;

	fin.close(); // Closing the file

	return 0;
}
