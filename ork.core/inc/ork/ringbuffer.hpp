///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// SPSC thread safe optionally lock-free ring buffer
//  TODO : cross-process 'views'
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <atomic>
#include "platutils.hpp"

#if 1 // relaxed mem consistency model (~50% faster than MSC)
#define MemFullFence std::memory_order_seq_cst
#define MemRelaxed std::memory_order_relaxed
#define MemAcquire std::memory_order_acquire
#define MemRelease std::memory_order_release
#else // memory sequential consistent 
#define MemFullFence std::memory_order_seq_cst
#define MemRelaxed std::memory_order_seq_cst
#define MemAcquire std::memory_order_seq_cst
#define MemRelease std::memory_order_seq_cst
#endif

namespace ork {

///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size> 
class SpScRingBuf {
public:
	enum { Capacity = Size+1 };

	SpScRingBuf();
	~SpScRingBuf() {}

	void push(const Element& item); 
	bool try_push(const Element& item);
	bool try_pop(Element& item);

private:

	size_t increment(size_t idx) const; 
	std::atomic<size_t> mTailIndex;  
	Element    			_array[Capacity]; // separate head<>tail cachelines
	std::atomic<size_t> mHeadIndex; 
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size>
SpScRingBuf<Element,Size>::SpScRingBuf()
{
	mHeadIndex.store(0,MemFullFence);
	mTailIndex.store(0,MemFullFence);
}

///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size>
bool SpScRingBuf<Element, Size>::try_push(const Element& item)
{       
	const auto current_tail = mTailIndex.load(MemRelaxed); 
	const auto next_tail = increment(current_tail); 
	if(next_tail != mHeadIndex.load(MemAcquire) )
	{     
		_array[current_tail] = item;
		mTailIndex.store(next_tail,MemRelease);
		return true;
	}
  
  return false; // full queue

}

///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size>
void SpScRingBuf<Element, Size>::push(const Element& item)
{
	bool bpushed = try_push(item);
	while(false==bpushed)
	{
		usleep(1000);
		bpushed = try_push(item);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Pop by Consumer can only update the head (load with relaxed, store with release)
//     the tail must be accessed with at least aquire
///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size>
bool SpScRingBuf<Element, Size>::try_pop(Element& item)
{
	const auto current_head = mHeadIndex.load(MemRelaxed);  
	if(current_head == mTailIndex.load(MemAcquire) ) 
		return false; // empty queue

	item = _array[current_head]; 
	mHeadIndex.store(increment(current_head),MemRelease);

	return true;

}

///////////////////////////////////////////////////////////////////////////////

template<typename Element, size_t Size>
size_t SpScRingBuf<Element, Size>::increment(size_t idx) const
{
	return (idx + 1) % Capacity;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MPMC Ring Buffer
//
//   modified from 1024cores.net
//
//  original license
//
// Copyright (c) 2010-2011 Dmitry Vyukov. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T,size_t max_items> struct MpMcRingBuf
{

	typedef T value_type;

	MpMcRingBuf(); // buffer_size must be power of two
	~MpMcRingBuf();

	MpMcRingBuf(const MpMcRingBuf&oth);

	void push(const T& data);
	bool try_push(const T& data);
	bool try_pop(T& data);

private:

	typedef char cacheline_pad_t [cacheline_size];

	struct cell_t
	{
		std::atomic<size_t>   mSequence;
		T                     mData;
	};

	cacheline_pad_t         mPAD0;
	cell_t  		        mCellBuffer[max_items];
	const size_t            kBufferMask;
	cacheline_pad_t         mPAD1;
	std::atomic<size_t>     mEnqueuePos;
	cacheline_pad_t         mPAD2;
	std::atomic<size_t>     mDequeuePos;
	cacheline_pad_t         mPAD3;

}; 

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


template<typename T,size_t max_items>
MpMcRingBuf<T,max_items>::MpMcRingBuf()
	: kBufferMask(max_items - 1)
{

	const bool is_size_power_of_two = (max_items >= 2) && ((max_items & (max_items - 1)) == 0);
	static_assert(is_size_power_of_two,"max_items must be a power of two");
	static_assert(sizeof(std::atomic<size_t>)==sizeof(size_t),"yo");
	//static_assert(1==2,"one must be equal to one");
	for (size_t i=0; i<max_items; i++ )
	{
		cell_t& the_cell = mCellBuffer[i];
		the_cell.mSequence.template store(i,MemRelaxed);
	}

	mEnqueuePos.store(0,MemRelaxed);
	mDequeuePos.store(0,MemRelaxed);
}

template<typename T,size_t max_items>
MpMcRingBuf<T,max_items>::MpMcRingBuf(const MpMcRingBuf&oth)
	: kBufferMask(oth.kBufferMask)
{
	size_t bufsize = kBufferMask+1;

	for (size_t i=0; i<bufsize; i++ )
	{
		const cell_t& src_cell = oth.mCellBuffer[i];
		cell_t& dst_cell = oth.mCellBuffer[i];
		dst_cell = src_cell;
	}
	mEnqueuePos = oth.mEnqueuePos;
	mDequeuePos = oth.mDequeuePos;
}

///////////////////////////////////////////////////////////////////////////////

template<typename T,size_t max_items>
MpMcRingBuf<T,max_items>::~MpMcRingBuf()
{
}

///////////////////////////////////////////////////////////////////////////////

template<typename T,size_t max_items>
void MpMcRingBuf<T,max_items>::push(const T& item)
{
	bool bpushed = try_push(item);
	while(false==bpushed)
	{
		usleep(250);
		bpushed = try_push(item);
	}
}

///////////////////////////////////////////////////////////////////////////////

template<typename T,size_t max_items>
bool MpMcRingBuf<T,max_items>::try_push(const T& data)
{
	cell_t* cell = nullptr;
	size_t pos = mEnqueuePos.load(MemRelaxed);
	for (;;)
	{
		cell = mCellBuffer + (pos & kBufferMask);
		//////////////////////////////////////
		size_t seq = cell->mSequence.template load(MemAcquire);
		intptr_t dif = intptr_t(seq) - intptr_t(pos);
		//////////////////////////////////////
		if (dif == 0)
		{	
			size_t oldpos = pos;
			size_t newpos = pos+1;
			bool bchg = mEnqueuePos.compare_exchange_weak(oldpos,newpos,MemRelaxed);
			if( bchg )
				break;
		}
		//////////////////////////////////////
		else if (dif < 0) // Full ?
			return false;
		//////////////////////////////////////
		else
			pos = mEnqueuePos.load(MemRelaxed);
	}

	cell->mData = data;
	cell->mSequence.template store(pos + 1,MemRelease);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

template<typename T,size_t max_items>
bool MpMcRingBuf<T,max_items>::try_pop(T& data)
{
	cell_t* cell;

	size_t pos = mDequeuePos.load(MemRelaxed);

	for (;;)
	{	cell = mCellBuffer + (pos & kBufferMask);
		//////////////////////////////////////
		size_t seq = cell->mSequence.template load(MemAcquire);
		intptr_t dif = intptr_t(seq) - intptr_t(pos + 1);
		//////////////////////////////////////
		if (dif == 0)
		{
			size_t oldpos = pos;
			size_t newpos = pos+1;
			bool bchg = mDequeuePos.compare_exchange_weak(oldpos,newpos,MemRelaxed);
			if( bchg )
				break;
		}
		//////////////////////////////////////
		else if (dif < 0) // Empty ?
		{
			return false;
		}
		//////////////////////////////////////
		else
			pos = mDequeuePos.load(MemRelaxed);
	}
	//////////////////////
	// Read From Cell 
	//////////////////////
	data = cell->mData;
	cell->mSequence.template store(1+pos+kBufferMask,MemRelease);
	return true;

}

} // namespace ork