/**
 * File: mr-random.h
 * -----------------
 * Presents the implementation of those functions exported by
 * the mr-random.h module
 */

#include "mr-random.h"
#include <cstdlib>  // for srand, random
#include <ctime>    // for time
#include <unistd.h> // for sleep
#include <stdlib.h>
#include <random>
using namespace std;

std::knuth_b rand_engine; // replace knuth_b with one of the engines listed below
std::uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);
void sleepRandomAmount(size_t low, size_t high)
{
  int sleepAmount = rand() % high + low;
  sleep(sleepAmount);
}

bool randomChance(double probability)
{
  return uniform_zero_to_one(rand_engine) >= probability;
}
