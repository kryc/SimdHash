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
	const size_t Offset,
	const size_t Length,
	const uint8_t* Buffer)
{
	size_t toWrite = Length - Offset;
	size_t next = Offset;
	const size_t bufferSize = Context->BufferSize;
	size_t bufferOffset = Context->Offset[Lane];
	
	while (toWrite > 0 && bufferOffset < bufferSize)
	{
		if (
			(bufferOffset & 0x7) == 0 &&
			toWrite >= 8
		)
		{
			// Destination buffer is 8-byte aligned
			const uint64_t buffer64 = *(uint64_t*)(&Buffer[next]);
			bufferOffset = SimdHashWriteBuffer64(Context, bufferOffset, Lane, buffer64);
			toWrite -= sizeof(uint64_t);
			next += sizeof(uint64_t);
		}
		else if (
			(bufferOffset & 0x3) == 0 &&
			toWrite >= 4
		)
		{
			// Destination buffer is 4-byte aligned
			const uint32_t buffer32 = *(uint32_t*)(&Buffer[next]);
			bufferOffset = SimdHashWriteBuffer32(Context, bufferOffset, Lane, buffer32);
			toWrite -= sizeof(uint32_t);
			next += sizeof(uint32_t);
		}
		else
		{
			bufferOffset = SimdHashWriteBuffer8(Context, bufferOffset, Lane, Buffer[next]);
			toWrite--;
			next++;
		}
	}

	size_t bytesWritten = next - Offset;
	Context->Offset[Lane] = bufferOffset;
	Context->BitLength[Lane] += bytesWritten * 8;
	
	return toWrite;
}