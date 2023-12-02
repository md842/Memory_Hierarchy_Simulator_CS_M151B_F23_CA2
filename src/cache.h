#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock{
	unsigned tag; // Computed based on address, index, and block offset
	int lru_position; // 0 means most recently used, larger means older.
	int data; // The actual data stored in the cache/memory
	bool valid; // Whether or not the block has actually been initalized.
};

struct Stat{
	int missL1;
	int missL2;
  int missVic;
	int accL1;
	int accL2;
	int accVic;
};

class cache{
private:
	cacheBlock L1[L1_CACHE_SETS]; // 16 sets
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // 16 sets, 8 ways per set
	cacheBlock VC[VICTIM_SIZE]; // 4 entries
	
	Stat myStat;
  
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem, bool debugMode = false);

  bool readL1(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);
  bool readVC(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);
  bool readL2(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);
  void readMem(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);

  bool writeL1(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);
  bool writeVC(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);
  bool writeL2(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode = false);

	double getMissL1();
  double getMissL2();
  double getMissVic();
  double getAccL1();
  double getAccL2();
  double getAccVic();
};


