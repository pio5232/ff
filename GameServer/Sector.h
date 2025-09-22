#pragma once

namespace jh_content
{
	struct Sector
	{
		int m_iX;
		int m_iZ;

		Sector& operator= (const Sector& other)
		{
			m_iX = other.m_iX;
			m_iZ = other.m_iZ;

			return *this;
		}

		bool operator ==(const Sector& other)
		{
			return (m_iX == other.m_iX && m_iZ == other.m_iZ);
		}

		bool operator !=(const Sector& other)
		{
			return !(*this == other);
		}

		bool operator< (const Sector& other) const
		{
			if (m_iZ != other.m_iZ)
				return m_iZ < other.m_iZ;

			return m_iX < other.m_iX;
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