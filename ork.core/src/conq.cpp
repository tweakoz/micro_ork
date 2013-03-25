#include <ork/conq.h>
#include <assert.h>
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011, Dmitry Vyukov 
// 
//   www.1024cores.net
//
/////////////////////////////////////////////////////////////////////////////////
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License version 3 as 
//  published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
/////////////////////////////////////////////////////////////////////////////////

 spsq_priv::spsq_priv(	size_t item_size,
                		size_t items_per_bucket,
                		size_t max_bucket_count
                 	)
    : kItemSize(item_size)
    , kItemsPerBucket(items_per_bucket)
    , kBucketSize (kItemSize*kItemsPerBucket)
    , kMaxBuckets (max_bucket_count)
{
	assert(kBucketSize>=(kItemSize+2*sizeof(void*)));

    printf("itmsiz<%d> buksiz<%d> maxbuk<%d>\n",
    			int(kItemSize),
    			int(kBucketSize),
    			int(max_bucket_count) );

    mNumBuckets = 0;
    bucket_t* bucket = AllocBucket();
    mHeadPos = bucket->mData;
    mTailPos = 0;
    mTailEnd = kBucketSize;
    mTailNext = 0;
    mTailBucket = bucket;
    mLastBucket = bucket;
    *(void**)mHeadPos = (void*)eof;
    mPADD[0] = 0;

}

/////////////////////////////////////////////////////////////////////////////////

spsq_priv::~spsq_priv ()
{
    bucket_t* bucket = mLastBucket;
    while (bucket != nullptr)
    {
        bucket_t* next_bucket = bucket->mNext;
        aligned_free(bucket);
        bucket = next_bucket;
    }
}

/////////////////////////////////////////////////////////////////////////////////

void* spsq_priv::BeginQueueItem()
{
	size_t ptrsiz = sizeof(void*);
    // round-up message size for proper alignment
    size_t msg_size = ((uintptr_t)(kItemSize + ptrsiz - 1) & ~(ptrsiz - 1)) + ptrsiz;
    size_t req_size = (msg_size + ptrsiz);

    size_t rem_size = (mTailEnd - mTailPos);

    // check as to whether there is enough space in the current bucket or not
    if (req_size > rem_size )
    {	// not enough, get another bucket
    	GetBucket();
    }
    rem_size = (mTailEnd - mTailPos);
    assert(rem_size >= req_size);
    // if yes, remember where next message starts
    mTailNext = mTailPos + msg_size;
    // and return a pointer into the bucket
    // for a user to fill with his data
    return mTailBucket->mData+(mTailPos + ptrsiz);

}

/////////////////////////////////////////////////////////////////////////////////

void spsq_priv::EndQueueItem ()
{
    //////////////////////
    // prepare next cell
    //////////////////////
    char* tail_next_data = mTailBucket->mData+mTailNext;
    *(char* volatile*)tail_next_data = (char*)eof;
    //////////////////////
    // update current cell
    // (after this point the message becomes consumable)
    //////////////////////
    //mTailPos[0]=tail_next
    char* tail_pos_data = mTailBucket->mData+mTailPos;
    atomic_addr_store_release((void* volatile*)tail_pos_data, tail_next_data);
    //////////////////////
    mTailPos = mTailNext;
}

/////////////////////////////////////////////////////////////////////////////////

const void* spsq_priv::BeginDeQueueItem()
{
    // load value in the current cell
    void* next = atomic_addr_load_acquire((void* volatile*)mHeadPos);
    // if EOF flag is not set,
    // then there is a consumable message
    if (((uintptr_t)next & eof) == 0)
    {
        char* msg = mHeadPos + sizeof(void*);
        return msg;
    }
    // otherwise there is just nothing or ...
    else if (((uintptr_t)next & ~eof) == 0)
    {
        return nullptr;
    }
    // ... a tranfer to a next bucket
    else
    {
        // in this case we just follow the pointer and retry
        atomic_addr_store_release((void* volatile*)&mHeadPos,
            (char*)((uintptr_t)next & ~eof));
        return BeginDeQueueItem();
    }
}

/////////////////////////////////////////////////////////////////////////////////

void spsq_priv::EndDeQueueItem ()
{
    // follow the pointer to the next cell
    char* next = *(char* volatile*)mHeadPos;
    assert(next != nullptr);
    atomic_addr_store_release((void* volatile*)&mHeadPos, next);
}

/////////////////////////////////////////////////////////////////////////////////

spsq_priv::bucket_t* spsq_priv::AllocBucket()
{
    bucket_t* bucket = (bucket_t*)aligned_malloc(sizeof(bucket_t) + kBucketSize);
    assert(bucket!=nullptr);
    bucket->mNext = nullptr;
    mNumBuckets++;
    return bucket;
}

/////////////////////////////////////////////////////////////////////////////////
// get a bucket
//  if one is not available for use
//   allocate a new one
//   if the max buckets limit has been reached
//    wait for one to become available
/////////////////////////////////////////////////////////////////////////////////

spsq_priv::bucket_t* spsq_priv::GetBucketImpl()
{
    bucket_t* bucket = nullptr;
    
    const char* head_pos = (const char*)atomic_addr_load_acquire((void* volatile*)&mHeadPos);

    //////////////////////////////////////////
    // wait for a bucket loop
    //////////////////////////////////////////

    while( IsPointerOutsideBucketBoundary(mLastBucket,head_pos) )
    {
        bucket = mLastBucket;
        mLastBucket = bucket->mNext;
        assert(mLastBucket != nullptr);
        bucket->mNext = nullptr;

        bool out_of_buckets = (mNumBuckets > kMaxBuckets);

        if( out_of_buckets && IsPointerOutsideBucketBoundary(mLastBucket,head_pos) )
        {
        	//static int ictr = 0;
        	//printf( "freeing bucket<%d>\n", ictr );
            aligned_free(bucket);
            bucket = nullptr;
            //ictr++;
            continue;
        }
        break;
    }

    //////////////////////////////////////////

    if (bucket == nullptr)
        bucket = AllocBucket();

    //////////////////////////////////////////
    // we have a bucket!
    //////////////////////////////////////////

    return bucket;
}

/////////////////////////////////////////////////////////////////////////////////

void spsq_priv::GetBucket()
{
    bucket_t* bucket = GetBucketImpl();
    *(void* volatile*)bucket->mData = (void*)eof;
    /////////////////////////////
    char* tail_pos_data = mTailBucket->mData+mTailPos;
    atomic_addr_store_release((void* volatile*)tail_pos_data, (void*)((uintptr_t)bucket->mData | eof));
    /////////////////////////////
    mTailPos = 0;
    mTailEnd = kBucketSize;
    mTailBucket->mNext = bucket;
    mTailBucket = bucket;
}
