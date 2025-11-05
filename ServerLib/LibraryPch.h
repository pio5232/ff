#pragma once

//#define LAN
//#define ECHO
#include <iostream>

#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "ws2_32.lib")

#include "Utils.h"
#include "GlobalInstance.h"
#include "Define.h"
#include "NetworkUtils.h"

#include "MemorySystem.h"
//#include "MemoryPool.h"
#include "ObjectPool.h"
//#include "TLSMemoryPool.h"
//#include "ClearPool.h"
//#include "ClearRefPtr.h"
#include "PacketDefine.h"
#include "Session.h"


#include "Timer.h"

#include "ErrorCode.h"
#include "Logger.h"
#include "Profiler.h"
#include "Parser.h"
#include "Job.h"
