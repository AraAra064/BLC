#include <vector>
#include <unordered_map>
#include <thread>
#include <algorithm>
#include <cmath>

#ifndef __BLCOLOURS__
	#define __BLCOLOURS__

namespace blc
{
	const uint32_t D_CTSIZE = 256;
	const float D_PSTEP = 0.0625f;

	struct ColourData
	{
		uint32_t c;
		uint32_t n;

		ColourData()
		{
			c = 0;
			n = 0;
		}
		ColourData(uint32_t c, uint32_t n)
		{
			this->c = c;
			this->n = n;
		}
	};

	uint32_t distFunc(uint32_t cA, uint32_t cB)
	{
		int16_t d1 = std::abs(int16_t((cA & 0x000000FF) - (cB & 0x000000FF)));
		int16_t d2 = std::abs(int16_t(((cA & 0x0000FF00) >> 8) - ((cB & 0x0000FF00) >> 8)));
		int16_t d3 = std::abs(int16_t(((cA & 0x00FF0000) >> 16) - ((cB & 0x00FF0000) >> 16)));
		int16_t d4 = std::abs(int16_t(((cA & 0xFF000000) >> 24) - ((cB & 0xFF000000) >> 24)));
		return d1 + d2 + d3 + d4;
	}

	//Returns an optimized colour palette from an image
	//Colour format does not matter (RGBA/BGRA) 
	std::vector<uint32_t> GetColourTable(std::vector<uint32_t>& pixels, uint32_t tableSize = D_CTSIZE, float pStep = D_PSTEP)
	{
		std::unordered_map<uint32_t, uint32_t> colourTable;
		std::vector<ColourData> ct;
		//colourTable[0x00000000] = -1;

		for (uint32_t c : pixels)
		{
			//A == 0
			if (((c & 0xFF000000) >> 24) == 0) {
				c = 0;
			}

			if (colourTable.count(c) == 0)
			{
				auto &colourCount = colourTable[c];
				colourCount += (colourCount != 0xFFFFFFFF ? 1 : 0);
				colourTable[c] = colourCount;
			}
		}

		//if (colourTable.size() <= colourTableSize) {
		//	return;
		//}

		ct.reserve(colourTable.size());
		for (auto c : colourTable)
		{
			ct.push_back(ColourData(c.first, c.second));
		}
		std::sort(ct.begin(), ct.end(), [](ColourData& a, ColourData& b)->bool {return a.n < b.n;});

		int iterations = (int)(100.f / pStep);
		//always keep most common colour, maybe change this
		int ctStepSize = (int)round(tableSize * (pStep / 100.f));
		if (ctStepSize == 0)
		{
			ctStepSize++;
		}
		//????
		for (int i = 1, pos = ctStepSize + (tableSize % ctStepSize); i < iterations && pos < ct.size(); i++)
		{
			memcpy(&ct[pos], &ct[(int)(ct.size() * ((float)i / (float)iterations)) - (ctStepSize / 2)], ctStepSize * sizeof(ColourData));
			pos += ctStepSize;
		}

		ct.resize(tableSize);
		std::vector<uint32_t> table(tableSize);
		for (size_t i = 0; i < table.size(); i++)
		{
			table[i] = ct[i].c;
		}

		//merge common colours to increase accuracy?

		return table;
	}

	//Returns image with optimized palette
	std::vector<uint32_t> ReduceColours(std::vector<uint32_t>& pixels, std::vector<uint32_t> colourTable)
	{
		std::vector<uint32_t> optPixels(pixels.size());
		std::unordered_map<uint32_t, uint32_t> dict;

		for (size_t i = 0; i < pixels.size(); i++)
		{
			uint32_t& pixel = pixels[i];

			if (dict.count(pixel) == 0)
			{
				uint32_t index = 0;
				uint32_t _c = pixel;
				bool findClosest = false;

				for (uint32_t it = 0, dLast = 0xFFFFFFFF; it < colourTable.size(); ++it)
				{
					auto& c = colourTable[it];
					uint32_t d = distFunc(c, pixel);
					if (d > dLast){continue;
					}
					if ((findClosest = (d != 0)))
					{
						dLast = d;
						index = it;
					} else break;
				}

				if (findClosest){_c = colourTable[index];
				}

				dict[pixel] = _c;
				optPixels[i] = _c;
			} else optPixels[i] = dict[pixel];
		}

		return optPixels;
	}
};

#endif