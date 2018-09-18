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

struct msgq_image
{
	ork::mpmc_bounded_queue<IpcPacket_t,ipcq_msgcnt> 	mMsgQ;
	ork::mpmc_bounded_queue<IpcPacket_t,ipcq_msgcnt/8> 	mDbgQ;

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

struct IpcMsgQSender
{
	IpcMsgQSender();
	~IpcMsgQSender();

	void Create( const std::string& nam );
	void Connect( const std::string& nam );
	void SendSyncStart();
	void send( const IpcPacket_t& msg );
	void send_debug( const IpcPacket_t& msg );
	void SetName(const std::string& nam);
	void SetSenderState(msgq_ep_state est);
	msgq_ep_state GetRecieverState() const;

	IpcMsgQProfileFrame profile();

	std::string mName;
	std::string mPath;
	//bool mbShutdown;

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
	bool try_recv( IpcPacket_t& msg_out );
	bool try_recv_debug( IpcPacket_t& msg_out );
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
	ork::atomic<fut_id_t> mFutureCtr;
};

} // namespace ork
