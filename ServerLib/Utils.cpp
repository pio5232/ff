#include "LibraryPch.h"
#include "Utils.h"
#include <random>
#include <math.h>
//namespace jh_utility
//{
//	//template<typename T, typename... Args>
//	//ManagerPool<T, Args...>::~ManagerPool()
//	//{
//	//	for (T* elementPtr : _elementArr)
//	//	{
//	//		delete elementPtr;
//	//	}
//	//	_elementArr.clear();
//	//}
//}

void ExecuteProcess(const std::wstring& path)//, const std::wstring& args)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::wstring commandLine = path;// +L" " + args;
	bool ret = CreateProcess(NULL, &commandLine[0], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	if (!ret)
	{
		printf("Create Process Failed [%d]\n", GetLastError());
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}

double GetRandDouble(double min, double max, int roundPlaceValue)
{
	static std::random_device randomDevice;

	static std::mt19937 gen(randomDevice());

	std::uniform_real_distribution<double> dist(min, max);

	double d = dist(gen);

	double powValue = pow(10, roundPlaceValue);
	d *= powValue;

	d = round(d);
	d /= powValue;

	return d;
}

int GetRand(int min, int max)
{
	static std::random_device randomDevice;

	static std::mt19937 gen(randomDevice());

	std::uniform_int_distribution<int> dist(min, max);

	return dist(gen);
}

bool CheckChance(int percentage)
{
	return static_cast<int>(GetRandDouble(0.0, 100.0)) < percentage;
}

float NormalizeAngle(float angle)
{
	angle = fmod(angle + 180.0f, 360.0f);
	if (angle < 0)
		angle += 360.0f;
	return angle - 180.0f;
}

Vector3::Vector3() : x(0), y(0), z(0)
{
}

Vector3::Vector3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ)
{
}

float Vector3::Distance(const Vector3& firstVec, const Vector3& secondVec)
{
	float diffX = firstVec.x - secondVec.x;
	float diffY = firstVec.y - secondVec.y;
	float diffZ = firstVec.z - secondVec.z;

	return static_cast<float>(sqrt(diffX * diffX + diffY * diffY + diffZ * diffZ));
}

