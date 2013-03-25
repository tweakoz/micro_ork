///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <sys/types.h>
#include <assert.h>
#include <ork/traits.h>
#include <stdlib.h>
#include <string.h>
#include <ork/concurrent_queue.hpp>
#include <ork/future.hpp>
#include <ork/mutex.h>
#include <map>
#include <atomic>

namespace net {

struct NetworkMessage;

///////////////////////////////////////////////////////////////////////////////
// struct: NetworkMessageIterator
//
//	data read marker/iterator for a NetworkMessage
///////////////////////////////////////////////////////////////////////////////
struct NetworkMessageIterator
{	///////////////////////////////////////////////////
	// Constructor: NetworkMessageIterator
	//
	// construct a Network Message Read Iterator
	//
	// Parameters:
	//
	// msg - Network Message that this iterator will iterate upon
	//
	NetworkMessageIterator( const NetworkMessage& msg );
	///////////////////////////////////////////////////

	////////////////////////////////////////////////////
	// Variable: mMessage
	// Network Message this iterator is iterating inside
	const NetworkMessage&			mMessage;
	////////////////////////////////////////////////////
	// Variable: miReadIndex
	// Read Byte Index into the message
	int								miReadIndex;	
	////////////////////////////////////////////////////
};
///////////////////////////////////////////////////////////////////////////////
//
// struct: NetworkMessage
//
// network message packet (an atomic network message)
//
///////////////////////////////////////////////////////////////////////////////
struct NetworkMessage
{	///////////////////////////////////////////////////////
	// constructor: NetworkMessage
	//
	// construct a network message
	//
	///////////////////////////////////////////////////////
	NetworkMessage() { clear(); }
	///////////////////////////////////////////////////////
	// method: clear
	//
	// clear the message to zero size
	//
	///////////////////////////////////////////////////////
	void clear() { miSize = 0; }
	///////////////////////////////////////////////////////
	// method: Write
	//
	// add data to the content of this message, increment write index by size of T
	//
	// Parameters:
	//
	// inp - the data to add
	//
	///////////////////////////////////////////////////////
	template <typename T> void Write( const T& inp )
	{
		static_assert(ork::is_trivially_copyable<T>::value,"can only write is_trivially_copyable's into a NetworkMessage!");
		size_t ilen = sizeof(T);
		assert( (miSize+ilen)<kmaxsize );
		//T swapped = ork::SwapToNetworkOrder(inp);
		char* pdest = & mBuffer[miSize];
		memcpy( pdest, (const char*) & inp, ilen );
		miSize += ilen;
	}
	void WriteString( const char* pstr )
	{
		size_t ilen = strlen(pstr)+1;
		assert( (miSize+ilen)<kmaxsize );
		Write(ilen);
		for( size_t i=0; i<ilen; i ++ )
			Write( pstr[i] ); // slow method, oh well doesnt matter now...
	}
	void WriteData( const void* pdata, size_t ilen )
	{
		assert( (miSize+ilen)<=kmaxsize );
		Write(ilen);
		const char* pch = (const char*) pdata;
		char* pdest = & mBuffer[miSize];
		memcpy( pdest, (const char*) pdata, ilen );
		miSize += ilen;
	}
	///////////////////////////////////////////////////////
	// method: Read
	//
	// read data from the content of this message at a given position specified by an iterator, then increment read index by size of T
	//
	// Parameters:
	//
	// it - the data's read iterator
	//
	///////////////////////////////////////////////////////
	template <typename T> void Read( T& outp, NetworkMessageIterator& it ) const
	{
		static_assert(ork::is_trivially_copyable<T>::value,"can only read is_trivially_copyable's from a NetworkMessage!");
		int ilen = sizeof(T);
		assert( (it.miReadIndex+ilen)<kmaxsize );
		//T inp;
		const char* psrc = & mBuffer[it.miReadIndex];
		memcpy( (char*) & outp, psrc, ilen );
		it.miReadIndex += ilen;
		//outp = ork::SwapFromNetworkOrder(inp);
	}
	std::string ReadString( NetworkMessageIterator& it ) const
	{
		size_t ilen = 0;
		char buffer[256];
		Read( ilen, it );
		assert( ilen<sizeof(buffer) );
		for( size_t i=0; i<ilen; i++ )
			Read( buffer[i], it );
		return std::string(buffer);
	}
	void ReadData( void* pdest, size_t ilen, NetworkMessageIterator& it ) const
	{
		size_t rrlen = 0;
		Read( rrlen, it );
		assert(rrlen==ilen);
		//char* pchd = (char*) pdest;
		const char* psrc = & mBuffer[it.miReadIndex];
		memcpy( (char*) pdest, psrc, ilen );
		it.miReadIndex += ilen;
	}
	///////////////////////////////////////////////////////
	/*NetworkMessage& operator = ( const NetworkMessage& rhs )
	{
		miSize = rhs.miSize;
		if( miSize )
			memcpy(mBuffer,rhs.mBuffer,miSize);
		return *this;
	}*/
	////////////////////////////////////////////////////
	void dump(const char* label)
	{
		size_t icount = miSize;
		size_t j = 0;
		while(icount>0)
		{
			uint8_t* paddr = (uint8_t*)(mBuffer+j);
			printf( "msg<%p:%s> [%02lx : ", this,label, j );
			size_t thisc = (icount>=16) ? 16 : icount;
			for( size_t i=0; i<thisc; i++ )
			{
				size_t idx = j+i;
				printf( "%02x ", paddr[i] );
			}
			j+=thisc;
			icount -= thisc;

			printf("\n");
		}
	}
	////////////////////////////////////////////////////
	// Constant: kmaxsize
	// maximum size of content of a single atomic network message
	static const size_t	kmaxsize = 4096;
	////////////////////////////////////////////////////
	// Variable: mBuffer
	// statically sized buffer for holding a network message's content
	char				mBuffer[kmaxsize];
	////////////////////////////////////////////////////
	// Variable: miSize
	// the actual size of the content of this message
	size_t					miSize;
	////////////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////////////////

enum msgq_ep_state
{
	EMQEPS_INIT = 0,
	EMQEPS_WAIT,
	EMQEPS_RUNNING,
	EMQEPS_TERMINATED,
};

struct msgq_image
{
	ork::mpmc_bounded_queue<NetworkMessage,8<<10> 	mMsgQ;
	ork::mpmc_bounded_queue<NetworkMessage,1<<10> 	mDbgQ;
	std::atomic<uint64_t> 							mSenderState;
	std::atomic<uint64_t> 							mRecieverState;

	msgq_image()
	{
		mSenderState = EMQEPS_INIT;
		mRecieverState = EMQEPS_INIT;
	}
};
typedef msgq_image msq_impl_t;

///////////////////////////////////////////////////////////////////////////////

struct IpcMsgQSender
{
	IpcMsgQSender();
	~IpcMsgQSender();

	void Create( const std::string& nam );
	void Connect( const std::string& nam );
	void SendSyncStart();
	void send( const NetworkMessage& msg );
	void send_debug( const NetworkMessage& msg );
	void SetName(const std::string& nam);
	void SetSenderState(msgq_ep_state est);
	msgq_ep_state GetRecieverState() const;

	std::string mName;
	std::string mPath;
	//bool mbShutdown;

	msq_impl_t* mOutbox;
	void* mShmAddr;


};

///////////////////////////////////////////////////////////////////////////////

struct IpcMsgQReciever
{
	typedef ork::RpcFuture::rpc_future_id_t fut_id_t;
	typedef std::map<fut_id_t,ork::RpcFuture*> future_map_t;

	IpcMsgQReciever();
	~IpcMsgQReciever();

	void SetName(const std::string& nam);

	void Create( const std::string& nam );
	void Connect( const std::string& nam );
	void WaitSyncStart();
	bool try_recv( NetworkMessage& msg_out );
	bool try_recv_debug( NetworkMessage& msg_out );
	void SetRecieverState(msgq_ep_state est);
	msgq_ep_state GetSenderState() const;

	//////////////////////////////////////////
	// future based RPC support
	//////////////////////////////////////////

	ork::RpcFuture* AllocFuture();
	void ReturnFuture( ork::RpcFuture* );
	ork::RpcFuture* FindFuture(fut_id_t fid) const;

	//////////////////////////////////////////

	msq_impl_t* mInbox;
	void* mShmAddr;

	//////////////////////////////////////////

	std::string mName;
	std::string mPath;
	//bool mbShutdown;

	static const size_t kmaxfutures = 64;
	ork::mpmc_bounded_queue<ork::RpcFuture*,kmaxfutures> mFuturePool;
	ork::LockedResource<future_map_t> mFutureMap;
	std::atomic<fut_id_t> mFutureCtr;
};

} // namespace net
