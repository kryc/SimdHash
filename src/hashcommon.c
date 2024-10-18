//
//  hashcommon.c
//  SimdHash
//
//  Created by Gareth Evans on 21/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#include "simdhash.h"
#include "hashcommon.h"
#include "library.h"

size_t
SimdHashUpdateLaneBuffer(
	SimdHashContext* Context,
	const size_t Lane,
	const size_t Length,
	const uint8_t* Buffer)
{
	// Get the current write offset
	size_t bufferOffset = Context->Offset[Lane];
	// The maximum we can write here is either the remaining
	// write buffer length or the input length
	const size_t maxWrite = Context->BufferSize - bufferOffset;
	const size_t maxRead = Length;
	const size_t toCopy = maxWrite > maxRead ? maxRead : maxWrite; // MIN(maxRead, maxWrite)
	const uint8_t* end = &Buffer[toCopy];
	uint8_t* next = (uint8_t*)Buffer;
	
	while (next < end)
	{
		if ((bufferOffset & 0x7) == 0 &&
			end - next >= sizeof(uint64_t))
		{
			// Destination buffer is 8-byte aligned
			bufferOffset = SimdHashWriteBuffer64(Context, bufferOffset, Lane, *(uint64_t*)next);
			next += sizeof(uint64_t);
		}
		else if ((bufferOffset & 0x3) == 0 &&
			end - next >= sizeof(uint32_t))
		{
			// Destination buffer is 4-byte aligned
			bufferOffset = SimdHashWriteBuffer32(Context, bufferOffset, Lane, *(uint32_t*)next);
			next += sizeof(uint32_t);
		}
		else if ((bufferOffset & 0x1) == 0 &&
			end - next >= sizeof(uint16_t))
		{
			// Destination buffer is 2-byte aligned
			bufferOffset = SimdHashWriteBuffer16(Context, bufferOffset, Lane, *(uint16_t*)next);
			next += sizeof(uint16_t);
		}
		else
		{
			bufferOffset = SimdHashWriteBuffer8(Context, bufferOffset, Lane, *next);
			next++;
		}
	}

	const size_t bytesWritten = next - Buffer;
	Context->Offset[Lane] = bufferOffset;
	Context->BitLength[Lane] += bytesWritten * 8;
	
	// Return number of bytes unwritten
	return Length - (next - Buffer);
}