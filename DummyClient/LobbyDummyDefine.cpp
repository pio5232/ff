#include "pch.h"

int GetRandValue(int div, int extra) { return  rand() % div + extra; }

int GetRandTimeForDummy() { return  GetRandValue(3000, 2000); }
