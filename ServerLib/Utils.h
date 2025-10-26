#pragma once
#include "RingBuffer.h"
#include "SerializationBuffer.h"
#include "CrashDump.h"
#include <stack>
#include <queue>
#include <math.h>
#include <algorithm>
#include <deque>
/*-------RAII--------
	  LockGuard
-------------------*/

class SRWLockGuard
{
public:
	SRWLockGuard(SRWLOCK* lock) : _playerLock(lock) { AcquireSRWLockExclusive(_playerLock); }
	~SRWLockGuard() { ReleaseSRWLockExclusive(_playerLock); }

	SRWLOCK* _playerLock;
};

class SRWSharedLockGuard
{
public:
	SRWSharedLockGuard(SRWLOCK* lock) : _playerLock(lock) { AcquireSRWLockShared(_playerLock); }
	~SRWSharedLockGuard() { ReleaseSRWLockShared(_playerLock); }

private:
	SRWLOCK* _playerLock;

};

namespace jh_utility
{
	struct ProcessTimeInfo
	{
		FILETIME creationTime;
		FILETIME exitTime;
		// GetProcessTime 실행 시 100 나노 초 기준으로 입력됨.
		FILETIME kernnelTime;
		FILETIME userTime;

		// convert용
		SYSTEMTIME systemTime;

		ULONGLONG prevKernelTime;
		ULONGLONG prevUserTime;
	};
	// 기본 자료형 사용
	template <typename T>
	class LockStack
	{
	public:
		LockStack()
		{
			InitializeSRWLock(&_lock);
		}
		void Reserve(int reserveCount)
		{
			SRWLockGuard lockGuard(&_lock);
			_vec.reserve(reserveCount);
		}
		void Push(T data)
		{
			SRWLockGuard lockGuard(&_lock);
			_vec.push_back(data);
		}

		bool TryPop(T& data)
		{
			SRWLockGuard lockGuard(&_lock);
			if (_vec.size() > 0)
			{
				data = _vec.back();

				_vec.pop_back();
				return true;
			}
			else
				return false;
		}
	private:
		SRWLOCK _lock;
		std::vector<T> _vec;
	};

	template <typename T>
	class LockQueue {
	public:
		LockQueue()
		{
			InitializeSRWLock(&m_lock);
		}
		//void Push(T data)
		void Push(T& data)
		{
			SRWLockGuard lockGuard(&m_lock);

			m_queue.push(data);
		}

		T Pop()
		{
			SRWLockGuard lockGuard(&m_lock);
			if (m_queue.size() > 0)
			{
				T ret = m_queue.front();
				m_queue.pop();
				
				return ret;
			}
			
			return T();
		}

		bool TryPop(T& t)
		{
			SRWLockGuard lockGuard(&m_lock);
			if (m_queue.size() > 0)
			{
				t = m_queue.front();
				m_queue.pop_front();

				return true;
			}
			else
				return false;
		}

		int PopAll(OUT std::vector<T>& vec)
		{
			SRWLockGuard lockGuard(&m_lock);

			int popCount = 0;
			while (m_queue.size() > 0)
			{
				T ele = m_queue.front();
				m_queue.pop();

				vec.push_back(ele);

				popCount++;
			}

			return popCount;
		}

		void Swap(std::queue<T>& q)
		{
			SRWLockGuard lockGuard(&m_lock);
			
			std::swap(m_queue, q);
		}
		int GetUseSize()
		{
			SRWLockGuard lockGuard(&m_lock);
			
			return m_queue.size();
		}
		void Clear()
		{
			SRWLockGuard lockGuard(&m_lock);
			
			// move 
			m_queue = std::queue<T>();	
		}
	private:
		SRWLOCK m_lock;
		std::queue<T> m_queue;
	};
}

void ExecuteProcess(const WCHAR* path, const WCHAR* currentDirectory);// , const std::wstring& args);

// Unity Vector System 이용
struct Vector3
{
	Vector3();
	Vector3(float x, float y, float z);
	Vector3(const Vector3& other)
	{

		x = other.x;
		y = other.y;
		z = other.z;
	}

	Vector3& operator= (const Vector3& other)
	{
		if (this != &other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
		}

		return *this;
	}

	Vector3& operator+= (const Vector3& other)
	{
		if (this != &other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
		}

		return *this;
	}

	Vector3 operator- (const Vector3& other)
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 operator- (const Vector3& other) const
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 operator*(float f)
	{
		return Vector3(x * f, y * f, z * f);
	}

	static Vector3 Zero()
	{
		static const Vector3 zero(0, 0, 0);
		return zero;
	}

	static Vector3 Left()
	{
		static const Vector3 left(-1.0f, 0, 0);
		return left;
	}
	static Vector3 Right()
	{
		static const Vector3 right(1.0f, 0, 0);
		return right;
	}
	static Vector3 Forward()
	{
		static const Vector3 forward(0, 0, 1.0f);
		return forward;
	}
	static Vector3 Back()
	{
		static const Vector3 back(0, 0, -1.0f);
		return back;
	}

	// Distance와 Magnitude는 루트계산 -> 느리다. 정확한 값아니면 사용x
	static float Distance(const Vector3& firstVec, const Vector3& secondVec);
	float Magnitude()
	{
		int powCount = 2;

		return static_cast<float>(sqrt(pow(x, powCount) + pow(y, powCount) + pow(z, powCount)));
	}
	float sqrMagnitude() const { return x * x + y * y + z * z; }

	static float Dot(const Vector3& from, const Vector3& to)
	{
		return from.x * to.x + from.y * to.y + from.z * to.z;
	}

	Vector3 Normalized()
	{
		float magnitude = Magnitude();

		if(magnitude > 1E-05f)
			return Vector3(x / magnitude, y / magnitude, z / magnitude);
		
		return Zero();
	}

	// 두 벡터 사이의 각도를 반환한다. 방향상관없이 리턴값은 0~180, 0~360을 위해선 SignedAngle을 사용하라.
	static float Angle(Vector3 from, Vector3 to)
	{
		float num = (float)sqrt(from.sqrMagnitude() * to.sqrMagnitude());
		if (num < 1E-15f)
		{
			return 0;
		}

		float num2 = std::clamp(Dot(from, to) / num, -1.0f, 1.0f);
		return (float)acos(num2) * 57.29578f;
	}


	bool operator== (const Vector3& other) const
	{
		return this->x == other.x && this->y == other.y && this->z == other.z;
	}

	bool operator!= (const Vector3& other) const
	{
		return !((*this) == other);
	}

	float x;
	float y;
	float z;
};

const float deg2Rad = 180.0f / 3.141592f;

double GetRandDouble(double min, double max, int roundPlaceValue = 0);
int GetRand(int min, int max);

bool CheckChance(int percentage);

float NormalizeAngle(float angle);