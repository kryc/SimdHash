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
	size_t toWrite;
	size_t next;
	
	toWrite = Length - Offset;
	next = Offset;
	
	while (toWrite > 0 && Context->Offset[Lane] < Context->BufferSize)
	{
		if (
			(Context->Offset[Lane] & 0x7) == 0 &&
			toWrite >= 8
		)
		{
			// Destination buffer is 8-byte aligned
			uint64_t buffer64 = *(uint64_t*)(&Buffer[next]);
			Context->Offset[Lane] = SimdHashWriteBuffer64(Context, Lane, buffer64);
			toWrite -= sizeof(uint64_t);
			next += sizeof(uint64_t);
		}
		else if (
			(Context->Offset[Lane] & 0x3) == 0 &&
			toWrite >= 4
		)
		{
			// Destination buffer is 4-byte aligned
			uint32_t* buffer32 = (uint32_t*)(&Buffer[next]);
			Context->Offset[Lane] = SimdHashWriteBuffer32(Context, Lane, *buffer32);
			toWrite -= sizeof(uint32_t);
			next += sizeof(uint32_t);
		}
		else
		{
			Context->Offset[Lane] = SimdHashWriteBuffer8(Context, Lane, Buffer[next]);
			toWrite--;
			next++;
		}
	}

	size_t bytesWritten = next - Offset;
	Context->BitLength[Lane] += bytesWritten * 8;
	
	return toWrite;
}