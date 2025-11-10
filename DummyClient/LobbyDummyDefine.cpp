#include "pch.h"
#include "LobbyDummyDefine.h"

alignas(64) LONG DummyData::aliveDummyCount = 0;

int GetRandValue(int div, int extra) { return  rand() % div + extra; }

int GetRandTimeForDummy() { return  GetRandValue(2000, 1500); }
