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
#include <ork/fixedstring.h>
#include <map>
#include <ork/atomic.h>
#include <ork/netpacket.h>

namespace ork {

static const size_t ipcq_msglen = 4096;
static const size_t ipcq_msgcnt = 65536;

///////////////////////////////////////////////////////////////////////////////

typedef MessagePacket<ipcq_msglen> IpcPacket_t;
typedef MessagePacketIterator<ipcq_msglen> IpcPacketIter_t;

enum msgq_ep_state
{
	EMQEPS_INIT = 0,
	EMQEPS_WAIT,
	EMQEPS_RUNNING,
	EMQEPS_TERMINATED,
};

typedef cringbuf::ringbuf<1,ipcq_msgcnt*sizeof(IpcPacket_t)> ringbuf_t;
typedef cringbuf::ringbuf_worker_t worker_t;

struct msgq_image
{

	ringbuf_t _ringbuf;

	ork::atomic<uint64_t> mSenderState;
	ork::atomic<uint64_t> mRecieverState;

	msgq_image()
	{
		mSenderState = EMQEPS_INIT;
		mRecieverState = EMQEPS_INIT;
	}
};
typedef msgq_image msq_impl_t;

///////////////////////////////////////////////////////////////////////////////

struct IpcMsgQProfileFrame
{
	size_t _bytesSent = 0;
	size_t _messagesSent = 0;
};

///////////////////////////////////////////////////////////////////////////////

inline void dump_segment(const char* label, const uint8_t* base_addr, size_t length)
{
    size_t icount = length;
    size_t j = 0;
    while(icount>0)
    {
        uint8_t* paddr = (uint8_t*)(base_addr+j);
        printf( "msg<%s> [%02lx : ", label, j );
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

///////////////////////////////////////////////////////////////////////////////

struct IpcMsgQSender
{
	IpcMsgQSender();
	~IpcMsgQSender();

	void Create( const std::string& nam );
	void Connect( const std::string& nam );
	void SendSyncStart();
	void send_debug( const IpcPacket_t& msg );
	void SetName(const std::string& nam);
	void SetSenderState(msgq_ep_state est);
	msgq_ep_state GetRecieverState() const;

	IpcMsgQProfileFrame profile();

	inline void send(const IpcPacket_t& inp)
	{
		beginSend() = inp;
		endSend();
	}


	inline IpcPacket_t& beginSend()
	{
		assert(_begpkt==nullptr);
		_begpkt = nullptr;

		assert(mOutbox!=nullptr);
		assert(GetRecieverState()!=EMQEPS_TERMINATED);

		bool acquired = false;
		while(false==acquired)
		{
			if( auto pdest = mOutbox->_ringbuf.tryEnqueue(_worker,sizeof(IpcPacket_t)))
			{	_begpkt = (IpcPacket_t*) pdest;
				_begpkt->clear();
				acquired = true;
			}
			else
				usleep(100);
		}


		return *_begpkt;
	}
	inline void endSend()
	{
		assert(_begpkt!=nullptr);

		_bytesSent.fetch_add(_begpkt->GetLength());
		_messagesSent.fetch_add(1);

		mOutbox->_ringbuf.enqueued(_worker);	
		_begpkt = nullptr;
	}

	IpcPacket_t* _begpkt = nullptr;
	std::string mName;
	std::string mPath;
	//bool mbShutdown;
	worker_t* _worker;
	msq_impl_t* mOutbox;
	void* mShmAddr;

	std::atomic<size_t> _bytesSent;
	std::atomic<size_t> _messagesSent;


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
	bool try_recv_debug( IpcPacket_t& msg_out );
	void SetRecieverState(msgq_ep_state est);
	msgq_ep_state GetSenderState() const;

	//////////////////////////////////////////

	inline const IpcPacket_t* beginRecv()
	{
		const IpcPacket_t* rval = nullptr;

		assert(mInbox!=nullptr);

		if( auto try_pkt = mInbox->_ringbuf.tryDequeue() )
		{
			if( try_pkt.len() >= sizeof(IpcPacket_t) )
				return (const IpcPacket_t*) try_pkt._readptr;
			else
				mInbox->_ringbuf.dequeued(0);

		}

		return nullptr;
	}
	inline void endRecv()
	{
		mInbox->_ringbuf.dequeued(sizeof(IpcPacket_t));
	}

	inline void recv( IpcPacket_t& out_msg )
	{
		bool done = false;
		while(false==done)
		{
			if( auto r = beginRecv() )
			{
				out_msg = *r;
				endRecv();
			}
			else			
				usleep(100);
		}
	}

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
	ork::atomic<fut_id_t> mFutureCtr;
};

} // namespace ork
