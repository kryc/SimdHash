//
//  shacommon.h
//  SimdHash
//
//  Created by Gareth Evans on 20/01/2021.
//  Copyright © 2021 Gareth Evans. All rights reserved.
//

#ifndef shacommon_h
#define shacommon_h

size_t
SimdShaUpdateBuffer(
	SimdShaContext* Context,
	const size_t Offset,
	const size_t Length,
	const uint8_t* Buffers[]);

#endif /* shacommon_h */
