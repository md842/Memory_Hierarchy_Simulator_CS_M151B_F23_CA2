#include "cache.h"

cache::cache(){
  // Initialize all cache blocks to invalid
	for (int i = 0; i < L1_CACHE_SETS; i++)
		L1[i].valid = false;

  for (int i = 0; i < VICTIM_SIZE; i++)
		VC[i].valid = false;

	for (int i = 0; i < L2_CACHE_SETS; i++){
		for (int j = 0; j < L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 
  }

	this->myStat.missL1 = 0;
	this->myStat.missL2 = 0;
  this->myStat.missVic = 0;
	this->myStat.accL1 = 0;
	this->myStat.accL2 = 0;
  this->myStat.accVic = 0;
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem, bool debugMode){
  // Note debugMode parameter has a default value of false.

  if (debugMode){ // Output debug info about the instruction. 
    cout << "Address: " << adr;
    if(MemR)
      cout << "\tAction: Read" << endl;
    else
      cout << "\tAction: Write\tData: " << *data << endl;
  }

  // Address is 12-bit because memory has 4096 lines
  // L1/L2 cache have 16 lines so index is 4 bits
  // Block offset is 2-bit because block size is 4 bytes
  // Therefore we have 6 bits left for tag
  unsigned tag = (adr & 0xFC0) >> 6; // highest 6 bits
  unsigned index = (adr & 0x3C) >> 2; // next lowest 4 bits
  unsigned block_offset = adr & 0x3; // 2 lowest bits

  if (debugMode) // Output more debug info about the instruction. 
    cout << "Tag: " << tag << "\t\tIndex: " << index << "\tBlock offset: " << block_offset << endl;

  if (MemR){ // Read (LW) instruction
    // Step 1. Search L1. If data found in L1, just need to update the stats. 
    // Step 2. Search victim cache. If data found in victim cache, bring data to L1, remove from VC, update LRU and tag. Evicted line from L1 goes to VC.
    // Step 3. Search L2. If data found in L2, bring data to L1, remove from L2, update LRU, data, and tag. Evicted line from L1 goes to VC, evicted line from VC goes to L2.
    // Step 4. Bring data from memory. Install in L1, update tag and data. Evicted line from L1 goes to VC, evicted line from VC goes to L2, evicted line from L2 is removed.

    // Step 1: Search L1
    bool L1_hit = readL1(data, adr, myMem, tag, index, block_offset, debugMode);

    if (!L1_hit){ // Step 2: No L1 hit, search victim cache
      if (debugMode)
        cout << "L1 cache miss!" << endl;
      this->myStat.missL1++; // Increment L1 misses in stats

      bool VC_hit = readVC(data, adr, myMem, tag, index, block_offset, debugMode);

      if (!VC_hit){ // Step 3: No VC hit, search L2
        if (debugMode)
          cout << "VC cache miss!" << endl;
        this->myStat.missVic++; // Increment VC misses in stats

        bool L2_hit = readL2(data, adr, myMem, tag, index, block_offset, debugMode);

        if (!L2_hit){ // Step 4: No L2 hit, bring data from memory
          if (debugMode)
            cout << "L2 cache miss! Bringing the data from memory to L1." << endl;
          this->myStat.missL2++; // Increment L2 misses in stats

          readMem(data, adr, myMem, tag, index, block_offset, debugMode); // Bring data from memory
        }
      }
    }
  }
  else{ // Write (SW) instruction
    // Write-no-allocate write-through strategy:
    // If data is in both cache and memory, both should be updated. If it is not in the cache, only memory should be updated.
    // We are not updating stats in here because the project spec calls for miss rate to only reflect LW instructions.
    // However, SW instructions still update the LRU positions.

    // Step 1: Search L1
    bool L1_hit = writeL1(data, adr, myMem, tag, index, block_offset, debugMode);

    if (!L1_hit){ // Step 2: No L1 hit, search victim cache
      if (debugMode)
        cout << "L1 cache miss!" << endl;

      bool VC_hit = writeVC(data, adr, myMem, tag, index, block_offset, debugMode);

      if (!VC_hit){ // Step 3: No VC hit, search L2
        if (debugMode)
          cout << "VC cache miss!" << endl;

        bool L2_hit = writeL2(data, adr, myMem, tag, index, block_offset, debugMode);

        if (!L2_hit){ // Step 4: No L2 hit, write data to memory only
          if (debugMode)
            cout << "L2 cache miss!" << endl;
        }
      }
    }
    // Write data to memory (this always happens regardless of cache hit or miss)
    myMem[adr] = *data;
  }
}

bool cache::readL1(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  this->myStat.accL1++; // Increment L1 accesses in stats
  bool L1_hit = false;
  if (L1[index].valid){ // Only check valid cache blocks to avoid garbage data matching
    if (L1[index].tag == tag){
      L1_hit = true; // Set L1 hit flag
      if (debugMode)
        cout << "L1 cache hit! Only stats need to be updated." << endl;
    }
  }
  return L1_hit;
}

bool cache::readVC(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  this->myStat.accVic++; // Increment VC accesses in stats
  bool VC_hit = false;
  unsigned VC_tag = (tag << 4) + index; // VC tag is highest 10 bits instead of 6 because fully associative cache doesn't have indices
  for (int VC_index = 0; VC_index < VICTIM_SIZE; VC_index++){ // Search the whole VC since we don't have indices
    if (VC[VC_index].valid){ // Only check valid cache blocks to avoid garbage data matching
      if (VC[VC_index].tag == VC_tag){
        VC_hit = true; // Set VC hit flag
        if (debugMode)
          cout << "VC cache hit! Bringing the data to L1." << endl;
        cacheBlock evicted = L1[index];
        L1[index] = VC[VC_index]; // Swap VC block into L1
        VC[VC_index] = evicted; // Swap evicted L1 block into VC

        for (int j = 0; j < VICTIM_SIZE; j++) // Increment all LRU positions in VC
          VC[j].lru_position++;
        VC[VC_index].lru_position = 0; // Set the newly swapped in data to most recent

        VC[VC_index].tag = (VC[VC_index].tag << 4) + index; // Convert the block in VC to 10-bit tag
        L1[index].tag = (L1[index].tag >> 4); // Convert the block in L1 to 6-bit tag
      }
    }
  }
  return VC_hit;
}

bool cache::readL2(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  this->myStat.accL2++; // Increment L2 accesses in stats
  bool L2_hit = false;
  for (int way = 0; way < L2_CACHE_WAYS; way++){ // Search all ways of the desired index
    if (L2[index][way].valid){ // Only check valid cache blocks to avoid garbage data matching
      if (L2[index][way].tag == tag){
        L2_hit = true; // Set VC hit flag
        if (debugMode)
          cout << "L2 cache hit! Bringing the data to L1." << endl;
        cacheBlock evicted_from_L1 = L1[index]; // L1 is DM, so evict based on index alone
        L1[index] = L2[index][way]; // Swap L2 block into L1

        // Find empty or oldest (because VC uses LRU) block in VC
        int VC_insert_pos = 0;
        bool VC_full = true; // If VC is not full, we do not need to evict something
        for (int VC_pos = 0; VC_pos < VICTIM_SIZE; VC_pos++){
          if (VC[VC_pos].valid){ // Block is occupied, let's check the LRU position
            if (VC[VC_pos].lru_position > VC[VC_insert_pos].lru_position)
              VC_insert_pos = VC_pos;
          }
          else{ // Block is empty, we can place it here
            VC_insert_pos = VC_pos;
            VC_full = false;
            break;
          }
        }

        if (VC_full){ // VC is full, so something is evicted to L2. If VC wasn't full, don't touch L2.
          cacheBlock evicted_from_VC = VC[VC_insert_pos];
          unsigned evicted_tag = (VC[VC_insert_pos].tag >> 4); // Shift VC tag right 4 bits for L2 tag
          unsigned evicted_index = (VC[VC_insert_pos].tag & 0xF); // Lower 4 bits of VC tag is L2 index
          if (debugMode) // Output debug info about the eviction.
            cout << "VC full, evicting tag " << evicted_tag << " to L2 index " << evicted_index << endl;

          // Find empty or oldest (because L2 uses LRU) block in L2's desired index
          int L2_insert_pos = 0;
          // Don't need to track if we actually evicted something from L2 because no lower level to process
          for (int L2_way = 0; L2_way < L2_CACHE_WAYS; L2_way++){
            if (L2[evicted_index][L2_way].valid){ // Block is occupied, let's check the LRU position
              if (L2[evicted_index][L2_way].lru_position > L2[evicted_index][L2_insert_pos].lru_position)
                L2_insert_pos = L2_way;
            }
            else{ // Block is empty, we can place it here
              L2_insert_pos = L2_way;
              break;
            }
          }

          L2[evicted_index][L2_insert_pos] = evicted_from_VC; // Insert evicted VC block into L2
          L2[evicted_index][L2_insert_pos].tag = evicted_tag; // Convert the block in L2 to 6-bit tag
          
          for (int L2_way = 0; L2_way < L2_CACHE_WAYS; L2_way++) // Increment all LRU positions for this L2 index
            L2[evicted_index][L2_way].lru_position++;
          L2[evicted_index][L2_insert_pos].lru_position = 0; // Set the newly swapped in data to most recent
        }

        VC[VC_insert_pos] = evicted_from_L1; // Insert evicted L1 block into VC
        VC[VC_insert_pos].tag = (VC[VC_insert_pos].tag << 4) + index; // Convert the block in VC to 10-bit tag

        for (int VC_pos = 0; VC_pos < VICTIM_SIZE; VC_pos++) // Increment all LRU positions in VC
          VC[VC_pos].lru_position++;
        VC[VC_insert_pos].lru_position = 0; // Set the newly swapped in data to most recent
      }
    }
  }
  return L2_hit;
}

void cache::readMem(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  cacheBlock evicted_from_L1 = L1[index]; // L1 is DM, so evict based on index alone

  cacheBlock new_L1_block; // Create a new cache block to insert into L1
  new_L1_block.data = myMem[adr]; // Read data from memory
  new_L1_block.lru_position = 0; // This doesn't really matter in L1 but initializing it anyways
  new_L1_block.tag = tag; // Set the previously computed tag
  new_L1_block.valid = 1; // Set this block to valid
  L1[index] = new_L1_block; // Insert the new block into L1

  // Find empty or oldest (because VC uses LRU) block in VC
  int VC_insert_pos = 0;
  bool VC_full = true; // If VC is not full, we do not need to evict something
  for (int VC_pos = 0; VC_pos < VICTIM_SIZE; VC_pos++){
    if (VC[VC_pos].valid){ // Block is occupied, let's check the LRU position
      if (VC[VC_pos].lru_position > VC[VC_insert_pos].lru_position)
        VC_insert_pos = VC_pos;
    }
    else{ // Block is empty, we can place it here
      VC_insert_pos = VC_pos;
      VC_full = false;
      break;
    }
  }

  if (VC_full){ // VC is full, so something is evicted to L2. If VC wasn't full, don't touch L2.
    cacheBlock evicted_from_VC = VC[VC_insert_pos];
    unsigned evicted_tag = (VC[VC_insert_pos].tag >> 4); // Shift VC tag right 4 bits for L2 tag
    unsigned evicted_index = (VC[VC_insert_pos].tag & 0xF); // Lower 4 bits of VC tag is L2 index
    if (debugMode){ // Output debug info about the eviction.
      cout << "VC full, evicting tag " << evicted_tag << " to L2 index " << evicted_index << endl;
    }

    // Find empty or oldest (because L2 uses LRU) block in L2's desired index
    int L2_insert_pos = 0;
    // Don't need to track if we actually evicted something from L2 because no lower level to process
    for (int L2_way = 0; L2_way < L2_CACHE_WAYS; L2_way++){
      if (L2[evicted_index][L2_way].valid){ // Block is occupied, let's check the LRU position
        if (L2[evicted_index][L2_way].lru_position > L2[evicted_index][L2_insert_pos].lru_position)
          L2_insert_pos = L2_way;
      }
      else{ // Block is empty, we can place it here
        L2_insert_pos = L2_way;
        break;
      }
    }

    L2[evicted_index][L2_insert_pos] = evicted_from_VC; // Insert evicted VC block into L2
    L2[evicted_index][L2_insert_pos].tag = evicted_tag; // Convert the block in L2 to 6-bit tag
    
    for (int L2_way = 0; L2_way < L2_CACHE_WAYS; L2_way++) // Increment all LRU positions for this L2 index
      L2[evicted_index][L2_way].lru_position++;
    L2[evicted_index][L2_insert_pos].lru_position = 0; // Set the newly swapped in data to most recent
  }

  VC[VC_insert_pos] = evicted_from_L1; // Insert evicted L1 block into VC
  VC[VC_insert_pos].tag = (VC[VC_insert_pos].tag << 4) + index; // Convert the block in VC to 10-bit tag

  for (int VC_pos = 0; VC_pos < VICTIM_SIZE; VC_pos++) // Increment all LRU positions in VC
    VC[VC_pos].lru_position++;
  VC[VC_insert_pos].lru_position = 0; // Set the newly swapped in data to most recent
}

bool cache::writeL1(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  bool L1_hit = false;
  if (L1[index].valid){ // Only check valid cache blocks to avoid garbage data matching
    if (L1[index].tag == tag){
      L1_hit = true; // Set L1 hit flag
      L1[index].data = *data; // Update data in L1 cache
      if (debugMode)
        cout << "L1 cache hit!" << endl;
    }
  }
  return L1_hit;
}

bool cache::writeVC(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  bool VC_hit = false;
  for (int VC_index = 0; VC_index < VICTIM_SIZE; VC_index++){ // Search the whole VC since we don't have indices
    if (VC[VC_index].valid){ // Only check valid cache blocks to avoid garbage data matching
      if (VC[VC_index].tag == tag){
        VC_hit = true; // Set VC hit flag
        VC[VC_index].data = *data; // Update data in VC cache
        if (debugMode)
          cout << "VC cache hit!" << endl;
      }
    }
  }
  return VC_hit;
}

bool cache::writeL2(int* data, int adr, int* myMem, unsigned tag, unsigned index, unsigned block_offset, bool debugMode){
  bool L2_hit = false;
  for (int way = 0; way < L2_CACHE_WAYS; way++){ // Search all ways of the desired index
    if (L2[index][way].valid){ // Only check valid cache blocks to avoid garbage data matching
      if (L2[index][way].tag == tag){
        L2_hit = true; // Set VC hit flag
        L2[index][way].data = *data; // Update data in VC cache
        if (debugMode)
          cout << "L2 cache hit!" << endl;
      }
    }
  }
  return L2_hit;
}

// Simple getters for retrieving cache stats

double cache::getMissL1(){
  return this->myStat.missL1;
}

double cache::getMissL2(){
  return this->myStat.missL2;
}

double cache::getMissVic(){
  return this->myStat.missVic;
}

double cache::getAccL1(){
  return this->myStat.accL1;
}

double cache::getAccL2(){
  return this->myStat.accL2;
}

double cache::getAccVic(){
  return this->myStat.accVic;
}