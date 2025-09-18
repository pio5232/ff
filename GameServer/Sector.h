#pragma once

namespace jh_content
{
	struct Sector
	{
		int x;
		int z;

		Sector& operator= (const Sector& other)
		{
			x = other.x;
			z = other.z;

			return *this;
		}

		bool operator ==(const Sector& other)
		{
			return (x == other.x && z == other.z);
		}

		bool operator !=(const Sector& other)
		{
			return !(*this == other);
		}

		bool operator< (const Sector& other) const
		{
			if (z != other.z)
				return z < other.z;

			return x < other.x;
		}
	};

	struct AroundSector
	{
		int sectorCount{};

		/// [] [] []  
		/// [] [] []  
		/// [] [] [] 
		Sector sectors[9]{};
	};
}