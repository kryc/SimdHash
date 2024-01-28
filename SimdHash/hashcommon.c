//
//  hashcommon.c
//  SimdHash
//
//  Created by Gareth Evans on 21/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#include "simdhash.h"
#include "hashcommon.h"

size_t
SimdHashUpdateLaneBuffer(
	SimdHashContext* Context,
	const size_t Lane,
	const size_t Offset,
	const size_t Length,
	const uint8_t* Buffer,
	const uint8_t BigEndian)
{
	size_t toWrite;
	size_t bufferIndex;
	size_t bufferOffset;
	size_t next;
	
	toWrite = Length - Offset;
	next = Offset;
	
	while (toWrite > 0 && Context->Length < Context->BufferSize)
	{
		bufferIndex = Context->Length / 4;
		
		if (
			(Context->Length & 0x3) == 0 &&
			toWrite >= 4
		)
		// 4-byte aligned
		{
			uint32_t* buffer32 = (uint32_t*) Buffer;
			size_t nextInputIndex = next / 4;
			uint32_t writeU32 = buffer32[nextInputIndex];
			
			if (BigEndian)
			{
				writeU32 = __builtin_bswap32(writeU32);
			}

			Context->Buffer[bufferIndex].epi32_u32[Lane] = writeU32;

			toWrite -= 4;
			next += 4;
		}
		else
		{
			bufferOffset = Context->Length % 4;

			if (BigEndian)
			{
				Context->Buffer[bufferIndex].epi32_u8[Lane][(sizeof(uint32_t) - 1 - bufferOffset)] = Buffer[next];
			}
			else
			{
				Context->Buffer[bufferIndex].epi32_u8[Lane][bufferOffset] = Buffer[next];
			}

			toWrite--;
			next++;
		}
	}

	Context->Length[Lane] = Length - toWrite;
	Context->BitLength[Lane] = (Length - toWrite) * 8;
	
	return toWrite;
}

// size_t
// SimdHashUpdateBuffer(
// 	SimdHashContext* Context,
// 	const size_t Offset,
// 	const size_t Length,
// 	const uint8_t* Buffers[],
// 	const uint8_t BigEndian)
// {
// 	size_t toWrite;
// 	size_t bufferIndex;
// 	size_t bufferOffset;
// 	size_t next;
	
// 	toWrite = Length - Offset;
// 	next = Offset;
	
// 	while (toWrite > 0 && Context->Length < Context->BufferSize)
// 	{
// 		bufferIndex = Context->Length / 4;
		
// 		if ((Context->Length & 0x3) == 0 &&
// 			toWrite >= 4)
// 		// 4-byte aligned
// 		{
// 			uint32_t** buffer32 = (uint32_t**) Buffers;
// 			size_t nextInputIndex = next / 4;
			
// 			for (size_t lane = 0; lane < Context->Lanes; lane++)
// 			{
// 				if (BigEndian)
// 				{
// 					Context->Buffer[bufferIndex].epi32_u32[lane] = __builtin_bswap32(buffer32[lane][nextInputIndex]);
// 				}
// 				else
// 				{
// 					Context->Buffer[bufferIndex].epi32_u32[lane] = buffer32[lane][nextInputIndex];
// 				}
// 			}
// 			toWrite -= 4;
// 			Context->Length += 4;
// 			Context->BitLength += 32;
// 			next += 4;
// 		}
// 		else
// 		{
// 			bufferOffset = Context->Length % 4;
// 			for (size_t lane = 0; lane < Context->Lanes; lane++)
// 			{
// 				if (BigEndian)
// 				{
// 					Context->Buffer[bufferIndex].epi32_u8[lane][(sizeof(uint32_t) - 1 - bufferOffset)] = Buffers[lane][next];
// 				}
// 				else
// 				{
// 					Context->Buffer[bufferIndex].epi32_u8[lane][bufferOffset] = Buffers[lane][next];
// 				}
// 			}
// 			toWrite--;
// 			Context->Length++;
// 			Context->BitLength += 8;
// 			next++;
// 		}
// 	}
	
// 	return toWrite;
// }
