#pragma once

#include <stdio.h>

#if ! defined(IRIX) // TBB atomics

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

struct MemFullFence
{
	static std::memory_order GetOrder() { return std::memory_order_seq_cst; }
};
struct MemRelaxed
{
	static std::memory_order GetOrder() { return std::memory_order_relaxed; }
};
struct MemAcquire
{
	static std::memory_order GetOrder() { return std::memory_order_acquire; }
};
struct MemRelease
{
	static std::memory_order GetOrder() { return std::memory_order_release; }
};

namespace ork {


template <typename T> struct atomic //: public std::atomic<T>
{
    template <typename A=MemRelaxed> void store( const T& source )
    {
    	mData.store(source,A::GetOrder());
    }

    template <typename A=MemRelaxed> T load() const
    {
    	return mData.load(A::GetOrder());
    }


    template <typename A=MemRelaxed> T compare_and_swap( T new_val, T ref_val )
    {
    	T old_val = ref_val;
    	bool changed = mData.compare_exchange_strong(old_val,new_val,A::GetOrder());
    	printf( "CAS ref<%d> new<%d> old<%d> changed<%d>\n", ref_val, new_val, old_val, int(changed));
    	return changed ? new_val : old_val;
    }

    template <typename A=MemRelaxed> T fetch_and_store( T new_val )
    {
    	return mData.exchange(new_val,A::GetOrder());
    }

    template <typename A=MemRelaxed> T fetch_and_increment()
    {
    	return mData.operator++(A::GetOrder());
    }

    template <typename A=MemRelaxed> T fetch_and_decrement()
    {
    	return mData.operator--(A::GetOrder());
    }

    operator T() const
    {
    	return (T)mData;
    }

    T operator++ (int iv)
    {
    	return mData.operator++(iv);
    }
    T operator-- (int iv)
    {
    	return mData.operator--(iv);
    }
    atomic<T>& operator = (const atomic<T>& inp)
    {
    	mData = inp.mData;
    	return *this;
    }
    atomic<T>& operator = (const T& inp)
    {
    	this->store<MemRelaxed>(inp);
    	return *this;
    }


    operator bool() const { return bool(load<MemFullFence>()); }

    std::atomic<T> mData;

};


}

///////////////////////////////////////////////

#endif