//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//
// TODO:Student Information
//
const char *studentName = "Shanshan Xiao";
const char *studentID   = "A53302520";
const char *email       = "s3xiao@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//
// Doublely linked list
typedef struct block
{
  uint32_t val;
  struct block* next, *prev;
}block;

typedef struct set
{
  uint32_t size;
  block* head, *tail;
}set;

block* newBlock(uint32_t val, block* prev, block* next) {
  block* b = (block*)malloc(sizeof(block));
  b->val = val;
  b->prev = prev;
  b->next = next;
  return b;
}

set* newCache(uint32_t cacheSet) {
  set* cache = (set*)malloc(sizeof(set) * cacheSet);
  for (int i = 0; i < cacheSet; i++) {
    cache[i].size = 0;
    cache[i].head = NULL;
    cache[i].tail = NULL;
  }
  return cache;
}

void pushFront(set* s, block* b) {
  if (s->size) {
    s->head->prev = b;
    b->next = s->head;
    s->head = b;
  }
  else {
    s->head = b;
    s->tail = b;
  }
  (s->size)++;
}

uint32_t popBack(set* s) {
  block* b = s->tail;
  s->tail = s->tail->prev;
  if (s->tail) s->tail->next = NULL;
  else s->head = NULL;
  uint32_t val = b->val;
  free(b);
  (s->size)--;
  return val;
}
// icache[index] & tag
block* pop(set* s, uint32_t tag) {
  if (s->size == 0) return NULL;
  block* b = s->head;
  for (int i = 0; i < s->size; i++) {
    if ((b->val) == tag) {
      if (b == (s->head)) {
        s->head = s->head->next;
        if (s->head) s->head->prev = NULL;
        else s->tail = NULL;
      }
      else if (b == (s->tail)) {
        s->tail = s->tail->prev;
        s->tail->next = NULL;
      }
      else {
        b->prev->next = b->next;
        b->next->prev = b->prev;
      }
      (s->size)--;
      b->next = NULL;
      b->prev = NULL;
      return b;
    }
    b = b->next;
  }
  // no such block
  return NULL;
}
//
//TODO: Add your Cache data structures here
//

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
set* icache;
set* dcache;
set* l2cache;

uint32_t offsetLen;
uint32_t offsetMask;

uint32_t icacheIndexLen;
uint32_t dcacheIndexLen;
uint32_t l2cacheIndexLen;

uint32_t icacheIndexMask;
uint32_t dcacheIndexMask;
uint32_t l2cacheIndexMask;
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //
  icache = newCache(icacheSets);
  dcache = newCache(dcacheSets);
  l2cache = newCache(l2cacheSets);

  offsetLen = ceil(log2(blocksize));
  offsetMask = (1 << offsetLen) - 1;

  icacheIndexLen = ceil(log2(icacheSets));
  dcacheIndexLen = ceil(log2(dcacheSets));
  l2cacheIndexLen = ceil(log2(l2cacheSets));

  icacheIndexMask = ((1 << icacheIndexLen) - 1) << offsetLen;
  dcacheIndexMask = ((1 << dcacheIndexLen) - 1) << offsetLen;
  l2cacheIndexMask = ((1 << l2cacheIndexLen) - 1) <<offsetLen;

}

//------------------------------------//
//           Inavalidation            //
//------------------------------------//
void icacheInvalid(uint32_t addr) {
  uint32_t index = (addr & icacheIndexMask) >> offsetLen;
  uint32_t tag = addr >> (icacheIndexLen + offsetLen);
  block* b = pop(&icache[index], tag);
  free(b);
}

void dcacheInvalid(uint32_t addr) {
  uint32_t index = (addr & dcacheIndexMask) >> offsetLen;
  uint32_t tag = addr >> (dcacheIndexLen + offsetLen);
  block* b = pop(&dcache[index], tag);
  free(b);
}
// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //
  if (icacheSets == 0) return l2cache_access(addr);

  icacheRefs++;
  uint32_t offset = addr & offsetMask;
  uint32_t index = (addr & icacheIndexMask) >> offsetLen;
  uint32_t tag = addr >> (icacheIndexLen + offsetLen);

  block* b = pop(&icache[index], tag);
  if (b != NULL) {
    pushFront(&icache[index], b);
    return icacheHitTime;
  }

  icacheMisses++;
  uint32_t l2Penalty = l2cache_access(addr);
  icachePenalties += l2Penalty;

  if (icache[index].size == icacheAssoc) {
    popBack(&icache[index]);
  }
  b = newBlock(tag, NULL, NULL);
  pushFront(&icache[index], b);

  return icacheHitTime + l2Penalty;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //TODO: Implement D$
  //
  if (dcacheSets == 0) return l2cache_access(addr);

  dcacheRefs++;
  uint32_t offset = addr & offsetMask;
  uint32_t index = (addr & dcacheIndexMask) >> offsetLen;
  uint32_t tag = addr >> (dcacheIndexLen + offsetLen);

  block* b = pop(&dcache[index], tag);
  if (b != NULL) {
    pushFront(&dcache[index], b);
    return dcacheHitTime;
  }

  dcacheMisses++;
  uint32_t l2Penalty = l2cache_access(addr);
  dcachePenalties += l2Penalty;

  if (dcache[index].size == dcacheAssoc) {
    popBack(&dcache[index]);
  }
  b = newBlock(tag, NULL, NULL);
  pushFront(&dcache[index], b);

  return dcacheHitTime + l2Penalty;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //
  if (l2cacheSets == 0) return memspeed;

  l2cacheRefs++;
  uint32_t offset = addr & offsetMask;
  uint32_t index = (addr & l2cacheIndexMask) >> offsetLen;
  uint32_t tag = addr >> (l2cacheIndexLen + offsetLen);

  block* b = pop(&l2cache[index], tag);
  if (b != NULL) {
    pushFront(&l2cache[index], b);
    return l2cacheHitTime;
  }

  l2cacheMisses++;
  uint32_t l2Penalty = memspeed;
  l2cachePenalties += l2Penalty;

  if (l2cache[index].size == l2cacheAssoc) {
    uint32_t popTag = popBack(&l2cache[index]);
    if (inclusive) {
      uint32_t invalidAddr = (popTag << (l2cacheIndexLen + offsetLen)) + (index << offsetLen) + offset;
      if (icacheSets > 0) icacheInvalid(invalidAddr);
      if (dcacheSets > 0) dcacheInvalid(invalidAddr);
    }
  }
  b = newBlock(tag, NULL, NULL);
  pushFront(&l2cache[index], b);
  return l2cacheHitTime + l2Penalty;
}
