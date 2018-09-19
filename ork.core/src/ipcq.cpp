///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#include <sys/mman.h>
#include <ork/ipcq.h>
#include <ork/path.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <ork/fixedstring.h>
#include <errno.h>

#define __MSGQ_DEBUG__

#include <immintrin.h>
#include <cstdint>

namespace ork {

//static const uin32_t kmapaddrflags = MAP_SHARED|MAP_LOCKED;
static const uint32_t kmapaddrflags = MAP_SHARED;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IpcMsgQSender::IpcMsgQSender()
	// //mbShutdown(false)
	: mOutbox(nullptr)
	, mShmAddr(nullptr)
{
}

///////////////////////////////////////////////////////////////////////////////

IpcMsgQSender::~IpcMsgQSender()
{
	//mbShutdown = true;
	//while( true == mbShutdown )
	//{
	//	usleep(1<<16);
	//}
	if( mShmAddr )
	{
		SetSenderState(EMQEPS_TERMINATED);
		shm_unlink(mPath.c_str());
		munmap(mShmAddr,sizeof(msq_impl_t));
	}
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQSender::SetName(const std::string& nam)
{
	mName = nam;
	ork::uristring_t path;
	path.format("/dev/shm/%s",nam.c_str());
	mPath = path.c_str();
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQSender::Create( const std::string& nam )
{
	SetName(nam);
#ifdef __MSGQ_DEBUG__ 
	printf( "IpcMsgQSender<%p> creating msgQ<%s>\n", this, nam.c_str() );
#endif

	int shm_id = shm_open(mName.c_str(),O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
	assert(shm_id>=0);

	size_t isize = sizeof(msq_impl_t);
	int ift = ftruncate(shm_id, isize);
	mShmAddr =  mmap(0,isize,PROT_READ|PROT_WRITE,kmapaddrflags,shm_id,0);
	close(shm_id);
	mOutbox = (msq_impl_t*) mShmAddr;
	const char* errstr = strerror(errno);
#ifdef __MSGQ_DEBUG__ 
	printf( "shmid<%d> errno<%s> size<%d>\n", shm_id, errstr, int(isize) );
#endif

	assert(mShmAddr!=(void*)0xffffffffffffffff);
	new(mOutbox) msq_impl_t;

	////////////////////////////

	mOutbox->_ringbuf.init();
	_worker = mOutbox->_ringbuf.getWorker(0);

	////////////////////////////

	SendSyncStart();
	SetSenderState(ork::EMQEPS_RUNNING);
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQSender::Connect( const std::string& nam )
{
	SetName(nam);
#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQSender<%p> connecting to msgQ<%s>\n", this, nam.c_str() );
#endif

	while( false==ork::Path(mPath).IsFile() )
	{
		usleep(1<<18);
	}

	size_t isize = sizeof(msq_impl_t);
	int shm_id = shm_open(mName.c_str(),O_RDWR,S_IRUSR|S_IWUSR);
	assert(shm_id>=0);
	mShmAddr =  mmap(0,isize,PROT_READ|PROT_WRITE,kmapaddrflags,shm_id,0);
	mOutbox = (msq_impl_t*) mShmAddr;
	close(shm_id);
#ifdef __MSGQ_DEBUG__ 
	printf( "IpcMsgQSender<%p> connected to msgQ<%s>\n", this, nam.c_str() );
#endif

}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQSender::SendSyncStart()
{
#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQSender<%p> sending start/sync\n", this );
#endif
    ork::IpcPacket_t msg;
    msg.WriteString("start/sync");
	this->send(msg);
}

///////////////////////////////////////////////////////////////////////////////

IpcMsgQProfileFrame IpcMsgQSender::profile()
{
	IpcMsgQProfileFrame rval;
	rval._bytesSent = _bytesSent.exchange(0);
	rval._messagesSent = _messagesSent.exchange(0);
	return rval;
}


///////////////////////////////////////////////////////////////////////////////

void IpcMsgQSender::SetSenderState(msgq_ep_state est)
{
	assert(mOutbox!=nullptr);
	mOutbox->mSenderState = (uint64_t) est;
}
msgq_ep_state IpcMsgQSender::GetRecieverState() const
{
	assert(mOutbox!=nullptr);
	return (msgq_ep_state) mOutbox->mRecieverState.load();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IpcMsgQReciever::IpcMsgQReciever()
	//: mbShutdown(false)
	: mInbox(nullptr)
	, mShmAddr(nullptr)
{
    mFutureCtr = 0;
}

///////////////////////////////////////////////////////////////////////////////

IpcMsgQReciever::~IpcMsgQReciever()
{
	//mbShutdown = true;
	//while( true == mbShutdown )
	//{
	//	usleep(1<<16);
	//}
	if( mShmAddr )
	{	
		SetRecieverState(EMQEPS_TERMINATED);
		shm_unlink(mPath.c_str());
		munmap(mShmAddr,sizeof(msq_impl_t));
	}
}

///////////////////////////////////////////////////////////////////////////////
// allocate a future
//  if one is not available in the pool, a new one will be heap-allocated
// once allocated, a msgq unique future ID will be assigned
//  and the future registered to that ID for later search
// 
// future's are attached/registered to msq recievers
//  because msq-reciever's will have to look a future
//  up when the future's associated message reply 
//  is recieved.
///////////////////////////////////////////////////////////////////////////////

ork::RpcFuture* IpcMsgQReciever::AllocFuture()
{
	ork::RpcFuture* rval = nullptr;
	bool ok = mFuturePool.try_pop(rval);
	if( ok )
	{
		assert(rval!=nullptr);
	}
	else // make a new one
		rval = new ork::RpcFuture;

	rval->Clear();
	rval->SetId(mFutureCtr++);
	mFutureMap.LockForWrite()[rval->GetId()]=rval;
	mFutureMap.Unlock();
	return rval;
}

///////////////////////////////////////////////////////////////////////////////
// find an allocated future by msgq unique future ID
///////////////////////////////////////////////////////////////////////////////

ork::RpcFuture* IpcMsgQReciever::FindFuture(int fid) const
{
	ork::RpcFuture* rval = nullptr;
	const future_map_t& fmap = mFutureMap.LockForRead();
	future_map_t::const_iterator it=fmap.find(fid);
	rval = (it==fmap.end()) ? nullptr : it->second;
	mFutureMap.Unlock();
	return rval;
}

///////////////////////////////////////////////////////////////////////////////
// return future to msq's future pool,
//  de-register it's ID
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void IpcMsgQReciever::ReturnFuture( ork::RpcFuture* pf)
{
	future_map_t& fmap = mFutureMap.LockForWrite();
	future_map_t::iterator it=fmap.find(pf->GetId());
	assert(it!=fmap.end());
	fmap.erase(it);
	mFutureMap.Unlock();
	mFuturePool.push(pf);
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQReciever::SetName(const std::string& nam)
{
	mName = nam;
	ork::uristring_t path;
	path.format("/dev/shm/%s",nam.c_str());
	mPath = path.c_str();
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQReciever::Connect( const std::string& nam )
{
	SetName(nam);
#ifdef __MSGQ_DEBUG__ 
	printf( "IpcMsgQReciever<%p> connecting to msgQ<%s>\n", this, nam.c_str() );
#endif

	size_t isize = sizeof(msq_impl_t);
bool keep_waiting = true;
#if defined(OSX)
        int shm_id = -1;
        while( keep_waiting )
        {
            usleep(1<<18);
            shm_id = shm_open(mName.c_str(),O_RDWR,S_IRUSR|S_IWUSR);
            keep_waiting = (shm_id<0);
        }
#else
	while( false==ork::Path(mPath).IsFile() )
	{
		usleep(1<<18);
	}
	int shm_id = shm_open(mName.c_str(),O_RDWR,S_IRUSR|S_IWUSR);
	assert(shm_id>=0);
#endif
	mShmAddr =  mmap(0,isize,PROT_READ|PROT_WRITE,kmapaddrflags,shm_id,0);
	mInbox = (msq_impl_t*) mShmAddr;
	close(shm_id);
#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQReciever<%p> detected msgQ<%s>\n", this, nam.c_str() );
#endif

	while(GetSenderState()!=ork::EMQEPS_RUNNING)
	{
		usleep(1<<19);
		printf( "IpcMsgQReciever<%p> waiting for sender to go up\n", this );
	}

#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQReciever<%p> detected sender running...\n", this );
#endif


	WaitSyncStart();
	SetRecieverState(EMQEPS_RUNNING);
#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQReciever<%p> connected to msgQ<%s>\n", this, nam.c_str() );
#endif

}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQReciever::Create( const std::string& nam )
{
	assert(false);

	SetName(nam);
#ifdef __MSGQ_DEBUG__ 	
	printf( "IpcMsgQReciever<%p> creating msgQ<%s>\n", this, nam.c_str() );
#endif

	size_t isize = sizeof(msq_impl_t);
	int shm_id = shm_open(mName.c_str(),O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
	assert(shm_id>=0);
	int iftr = ftruncate(shm_id, isize);
	mShmAddr =  mmap(0,isize,PROT_READ|PROT_WRITE,kmapaddrflags,shm_id,0);
	new(mShmAddr) msq_impl_t;
	mInbox = (msq_impl_t*) mShmAddr;
	close(shm_id);
	SetRecieverState(ork::EMQEPS_RUNNING);
#ifdef __MSGQ_DEBUG__ 
    printf( "IpcMsgQReciever<%p> created msgQ<%s>\n", this, nam.c_str() );
#endif
}

///////////////////////////////////////////////////////////////////////////////

void IpcMsgQReciever::WaitSyncStart()
{
	usleep(1<<20);
	SetRecieverState(EMQEPS_WAIT);
    // wait for sender to send start/sync message
    ork::IpcPacket_t msg;

	recv(msg);

    ork::IpcPacketIter_t syncit(msg);
    std::string sync_content = msg.ReadString(syncit);
    printf("sync_content<%s>\n", sync_content.c_str() );
    assert(sync_content=="start/sync");
}

void IpcMsgQReciever::SetRecieverState(msgq_ep_state est)
{
	assert(mInbox!=nullptr);
	mInbox->mRecieverState.store((uint64_t)est);
}
msgq_ep_state IpcMsgQReciever::GetSenderState() const
{
	assert(mInbox!=nullptr);
	return (msgq_ep_state) mInbox->mSenderState.load();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

} // namespace net

