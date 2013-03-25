///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ork/concurrent_queue.hpp>
#include <ork/svariant.h>
#include <atomic>
#include <semaphore.h>
#include <functional>
#include <string>

#include <set>

typedef ork::svar128_t op_wrap_t;
typedef std::function<void()> void_lambda_t;

struct OpMultiQ;

//////////////////////////////////////////////////////////////////////
namespace ork { struct Future; };
//////////////////////////////////////////////////////////////////////

void SetCurrentThreadName(const char* threadName);

//////////////////////////////////////////////////////////////////////

struct BarrierSyncReq
{
	BarrierSyncReq(ork::Future&f):mFuture(f){}
	ork::Future& mFuture;
};

//////////////////////////////////////////////////////////////////////

struct NOP {};

//////////////////////////////////////////////////////////////////////

struct Op
{
	op_wrap_t mWrapped;	
	std::string mName;

	Op(const Op& oth);
	Op(const BarrierSyncReq& op,const std::string& name="");
	Op(const void_lambda_t& op,const std::string& name="");
	//Op(const void_block_t& op,const std::string& name="");
	Op();
	~Op();

	void SetOp(const op_wrap_t& op);

	void QueueASync(OpMultiQ&q) const;
	void QueueSync(OpMultiQ&q) const;;
};

//////////////////////////////////////////////////////////////////////

struct OpGroup
{

	OpGroup(OpMultiQ*popq, const char* pname);
	void push( const Op& the_op );
	bool try_pop( Op& out_op );
	void drain();
	void MakeSerial() { mLimitMaxOpsInFlight=1; }
	void Disable() { mEnabled=false; }
	int NumOps() const { return int(mOpsPendingCounter); }

	////////////////////////////////

	std::string mGroupName;
	OpMultiQ* mpOpQ;

	////////////////////////////////

	ork::mpmc_bounded_queue<Op,4096> 	mOps;
	std::atomic<int>			 		mOpsInFlightCounter;
	std::atomic<int>			 		mOpSerialIndex;
	std::atomic<int> 					mOpsPendingCounter;

	bool 								mEnabled;

	////////////////////////////////

	int 								mLimitMaxOpsInFlight;
	int 								mLimitMaxOpsQueued;
};

//////////////////////////////////////////////////////////////////////

struct OpMultiQ
{
	OpMultiQ(int inumthreads, const char* name = "DefOpQ");
	~OpMultiQ();

	void push(const Op& the_op);
	void push_sync(const Op& the_op);
	void sync();
	void drain();

	OpGroup* CreateOpGroup(const char* pname);

	static OpMultiQ* GlobalConQ();
	static OpMultiQ* GlobalSerQ();

	void notify_one();
	void BlockingIterate(int thid);

	OpGroup* mDefaultGroup;
	std::atomic<int> mGroupCounter;
	std::set<OpGroup*> mOpGroups;
	bool mbOkToExit;
	std::atomic<int> mThreadsRunning;
	std::string mName;

private:

	void ProcessOne(OpGroup*pgrp,const Op& the_op);
	void notify_all();

	ork::mpmc_bounded_queue<OpGroup*,32> 	mOpGroupRing;

	std::atomic<int> mOpsPendingCounter2;
	//std::condition_variable mOpWaitCV;
	//mtx_t mOpWaitMtx;

};	

