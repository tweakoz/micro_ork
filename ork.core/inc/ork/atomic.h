#pragma once

#if 1 // TBB atomics

#include <tbb/tbb.h>

namespace ork {

///////////////////////////////////////////////
#if 1 // relaxed mem consistency model (~50% faster than MSC)
#define MemFullFence tbb::full_fence
#define MemRelaxed tbb::relaxed
#define MemAcquire tbb::acquire
#define MemRelease tbb::release
#else // memory sequential consistent 
#define MemFullFence tbb::full_fence
#define MemRelaxed tbb::full_fence
#define MemAcquire tbb::full_fence
#define MemRelease tbb::full_fence
#endif
///////////////////////////////////////////////

template<typename T> using atomic = tbb::atomic<T>;

/*template <typename T> struct atomic : public tbb::atomic<T>
{

};*/

}


#else // STD atomics

#include <atomic>

namespace ork {

template <typename T> struct atomic : public std::atomic<T>
{
	the_cell.mSequence.template store<MemRelaxed>(i);
	cell->mSequence.template load<MemAcquire>();
	size_t dq_read = mDequeuePos.compare_and_swap<MemRelaxed>(pos+1,pos);
		destroyer_t pdestr = mDestroyer.exchange(nullptr); == fetch_and_store

	fetch_and_increment = fetch_add(1)
	fetch_and_decrement = fetch_sub(1)
};


}

///////////////////////////////////////////////
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
///////////////////////////////////////////////

#endif