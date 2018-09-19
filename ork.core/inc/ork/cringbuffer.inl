/*-
 * Copyright (c) 2016-2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Atomic multi-producer single-consumer ring buffer, which supports
 * contiguous range operations and which can be conveniently used for
 * message passing.
 *
 * There are three offsets -- think of clock hands:
 * - NEXT: marks the beginning of the available space,
 * - WRITTEN: the point up to which the data is actually written.
 * - Observed READY: point up to which data is ready to be written.
 *
 * Producers
 *
 *	Observe and save the 'next' offset, then request N bytes from
 *	the ring buffer by atomically advancing the 'next' offset.  Once
 *	the data is written into the "reserved" buffer space, the thread
 *	clears the saved value; these observed values are used to compute
 *	the 'ready' offset.
 *
 * Consumer
 *
 *	Writes the data between 'written' and 'ready' offsets and updates
 *	the 'written' value.  The consumer thread scans for the lowest
 *	seen value by the producers.
 *
 * Key invariant
 *
 *	Producers cannot go beyond the 'written' offset; producers are
 *	also not allowed to catch up with the consumer.  Only the consumer
 *	is allowed to catch up with the producer i.e. set the 'written'
 *	offset to be equal to the 'next' offset.
 *
 * Wrap-around
 *
 *	If the producer cannot acquire the requested length due to little
 *	available space at the end of the buffer, then it will wraparound.
 *	WRAP_LOCK_BIT in 'next' offset is used to lock the 'end' offset.
 *
 *	There is an ABA problem if one producer stalls while a pair of
 *	producer and consumer would both successfully wrap-around and set
 *	the 'next' offset to the stale value of the first producer, thus
 *	letting it to perform a successful CAS violating the invariant.
 *	A counter in the 'next' offset (masked by WRAP_COUNTER) is used
 *	to prevent from this problem.  It is incremented on wraparounds.
 *
 *	The same ABA problem could also cause a stale 'ready' offset,
 *	which could be observed by the consumer.  We set WRAP_LOCK_BIT in
 *	the 'seen' value before advancing the 'next' and clear this bit
 *	after the successful advancing; this ensures that only the stable
 *	'ready' observed by the consumer.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <assert.h>
#include <algorithm>

namespace cringbuf {

/*
 * Branch prediction macros.
 */
#ifndef __predict_true
#define	__predict_true(x)	__builtin_expect((x) != 0, 1)
#define	__predict_false(x)	__builtin_expect((x) != 0, 0)
#endif

/*
 * Atomic operations and memory barriers.  If C11 API is not available,
 * then wrap the GCC builtin routines.
 */
#ifndef atomic_compare_exchange_weak
#define	atomic_compare_exchange_weak(ptr, expected, desired) \
    __sync_bool_compare_and_swap(ptr, expected, desired)
#endif

#ifndef atomic_thread_fence
#define	memory_order_acquire	__ATOMIC_ACQUIRE
#define	memory_order_release	__ATOMIC_RELEASE
#define	memory_order_seq_cst	__ATOMIC_SEQ_CST
#define	atomic_thread_fence(m)	__atomic_thread_fence(m)
#endif

/*
 * C11 memory model does not support classic load/store barriers.
 * Emulate it using the using the full memory barrier.
 */
#ifndef memory_order_loads
#define	memory_order_loads	memory_order_seq_cst
#endif
#ifndef memory_order_stores
#define	memory_order_stores	memory_order_seq_cst
#endif

/*
 * Exponential back-off for the spinning paths.
 */
#define	SPINLOCK_BACKOFF_MIN	4
#define	SPINLOCK_BACKOFF_MAX	128
#if defined(__x86_64__) || defined(__i386__)
#define SPINLOCK_BACKOFF_HOOK	__asm volatile("pause" ::: "memory")
#else
#define SPINLOCK_BACKOFF_HOOK
#endif
#define	SPINLOCK_BACKOFF(count)					\
do {								\
	for (int __i = (count); __i != 0; __i--) {		\
		SPINLOCK_BACKOFF_HOOK;				\
	}							\
	if ((count) < SPINLOCK_BACKOFF_MAX)			\
		(count) += (count);				\
} while (/* CONSTCOND */ 0);

typedef struct ringbuf_worker ringbuf_worker_t;

#define	RBUF_OFF_MASK	(0x00000000ffffffffUL)
#define	WRAP_LOCK_BIT	(0x8000000000000000UL)
#define	RBUF_OFF_MAX	(UINT64_MAX & ~WRAP_LOCK_BIT)

#define	WRAP_COUNTER	(0x7fffffff00000000UL)
#define	WRAP_INCR(x)	(((x) + 0x100000000UL) & WRAP_COUNTER)

typedef uint64_t	ringbuf_off_t;

struct ringbuf_worker {
	volatile ringbuf_off_t	seen_off;
	int			registered;
};

template <const int kworkers,
          const size_t length> struct ringbuf {

	static constexpr size_t _space = length;
	static constexpr uint32_t _nworkers = kworkers;

	/*
	 * The NEXT hand is atomically updated by the producer.
	 * WRAP_LOCK_BIT is set in case of wrap-around; in such case,
	 * the producer can update the 'end' offset.
	 */

	volatile ringbuf_off_t _next;
	ringbuf_off_t		_end;

	/* The following are updated by the consumer. */
	ringbuf_off_t		_written;
	ringbuf_worker_t	workers[kworkers];

	char _storage[_space];

	///////////////////////////

	inline int init()
	{
		/////////////////////////
		// we intend on putting this in shared mem segments
		//  so assert that it is trivially copyable and a POD
		/////////////////////////

		static_assert(std::is_trivially_copyable<ringbuf>(), "must be trivially copyable!!!");
		static_assert(std::is_pod<ringbuf>(), "must be a POD!!!");

		/////////////////////////

		if (length >= RBUF_OFF_MASK) {
			errno = EINVAL;
			return -1;
		}
		memset(this, 0, sizeof(ringbuf<kworkers,length>));
		this->_end = RBUF_OFF_MAX;
		return 0;
	}

	/*
	 * ringbuf_register: register the worker (thread/process) as a producer
	 * and pass the pointer to its local store.
	 */

	inline ringbuf_worker_t *
	getWorker(int index)
	{
		assert(index<kworkers);
		auto w = & workers[index];

		w->seen_off = RBUF_OFF_MAX;
		atomic_thread_fence(memory_order_stores);
		w->registered = true;
		return w;
	}
	inline void
	releaseWorker(ringbuf_worker_t *w)
	{
		w->registered = false;
		(void)this;
	}

	///////////////////////////
	/*
	 * stable_nextoff: capture and return a stable value of the 'next' offset.
	 */
	inline ringbuf_off_t
	_stable_nextoff()
	{
		unsigned count = SPINLOCK_BACKOFF_MIN;
		ringbuf_off_t next;

		while ((next = this->_next) & WRAP_LOCK_BIT) {
			SPINLOCK_BACKOFF(count);
		}
		atomic_thread_fence(memory_order_loads);
		assert((next & RBUF_OFF_MASK) < this->_space);
		return next;
	}


	/*
	 * ringbuf_acquire: request a space of a given length in the ring buffer.
	 *
	 * => On success: returns the offset at which the space is available.
	 * => On failure: returns -1.
	 */
	inline char*
	tryEnqueue(ringbuf_worker_t *w, size_t len)
	{
		ringbuf_off_t seen, next, target;

		assert(len > 0 && len <= this->_space);
		assert(w->seen_off == RBUF_OFF_MAX);

		do {
			ringbuf_off_t written;

			/*
			 * Get the stable 'next' offset.  Save the observed 'next'
			 * value (i.e. the 'seen' offset), but mark the value as
			 * unstable (set WRAP_LOCK_BIT).
			 *
			 * Note: CAS will issue a memory_order_release for us and
			 * thus ensures that it reaches global visibility together
			 * with new 'next'.
			 */
			seen = this->_stable_nextoff();
			next = seen & RBUF_OFF_MASK;
			assert(next < this->_space);
			w->seen_off = next | WRAP_LOCK_BIT;

			/*
			 * Compute the target offset.  Key invariant: we cannot
			 * go beyond the WRITTEN offset or catch up with it.
			 */
			target = next + len;
			written = this->_written;
			if (__predict_false(next < written && target >= written)) {
				/* The producer must wait. */
				w->seen_off = RBUF_OFF_MAX;
				return nullptr;
			}

			if (__predict_false(target >= this->_space)) {
				const bool exceed = target > this->_space;

				/*
				 * Wrap-around and start from the beginning.
				 *
				 * If we would exceed the buffer, then attempt to
				 * acquire the WRAP_LOCK_BIT and use the space in
				 * the beginning.  If we used all space exactly to
				 * the end, then reset to 0.
				 *
				 * Check the invariant again.
				 */
				target = exceed ? (WRAP_LOCK_BIT | len) : 0;
				if ((target & RBUF_OFF_MASK) >= written) {
					w->seen_off = RBUF_OFF_MAX;
					return nullptr;
				}
				/* Increment the wrap-around counter. */
				target |= WRAP_INCR(seen & WRAP_COUNTER);
			} else {
				/* Preserve the wrap-around counter. */
				target |= seen & WRAP_COUNTER;
			}
		} while (!atomic_compare_exchange_weak(&this->_next, seen, target));

		/*
		 * Acquired the range.  Clear WRAP_LOCK_BIT in the 'seen' value
		 * thus indicating that it is stable now.
		 */
		w->seen_off &= ~WRAP_LOCK_BIT;

		/*
		 * If we set the WRAP_LOCK_BIT in the 'next' (because we exceed
		 * the remaining space and need to wrap-around), then save the
		 * 'end' offset and release the lock.
		 */
		if (__predict_false(target & WRAP_LOCK_BIT)) {
			/* Cannot wrap-around again if consumer did not catch-up. */
			assert(this->_written <= next);
			assert(this->_end == RBUF_OFF_MAX);
			this->_end = next;
			next = 0;

			/*
			 * Unlock: ensure the 'end' offset reaches global
			 * visibility before the lock is released.
			 */
			atomic_thread_fence(memory_order_stores);
			this->_next = (target & ~WRAP_LOCK_BIT);
		}
		assert((target & RBUF_OFF_MASK) <= this->_space);
		return this->_storage+next;
	}

	/*
	 * ringbuf_produce: indicate the acquired range in the buffer is produced
	 * and is ready to be consumed.
	 */
	inline void
	enqueued(ringbuf_worker_t *w)
	{
		(void)this;
		assert(w->registered);
		assert(w->seen_off != RBUF_OFF_MAX);
		atomic_thread_fence(memory_order_stores);
		w->seen_off = RBUF_OFF_MAX;
	}

	/*
	 * ringbuf_consume: get a contiguous range which is ready to be consumed.
	 */

	struct ConsumptionAttempt 
	{
		const char* _readptr = nullptr;
		size_t _length = 0;

		operator bool() const { return bool(_readptr!=nullptr); }
	};

	inline ConsumptionAttempt tryDequeue()
	{
		ringbuf_off_t written = this->_written, next, ready;
		size_t towrite;
	retry:
		/*
		 * Get the stable 'next' offset.  Note: stable_nextoff() issued
		 * a load memory barrier.  The area between the 'written' offset
		 * and the 'next' offset will be the *preliminary* target buffer
		 * area to be consumed.
		 */
		next = this->_stable_nextoff() & RBUF_OFF_MASK;
		if (written == next) {
			/* If producers did not advance, then nothing to do. */
			return ConsumptionAttempt();
		}

		/*
		 * Observe the 'ready' offset of each producer.
		 *
		 * At this point, some producer might have already triggered the
		 * wrap-around and some (or all) seen 'ready' values might be in
		 * the range between 0 and 'written'.  We have to skip them.
		 */
		ready = RBUF_OFF_MAX;

		for (unsigned i = 0; i < this->_nworkers; i++) {
			ringbuf_worker_t *w = &this->workers[i];
			unsigned count = SPINLOCK_BACKOFF_MIN;
			ringbuf_off_t seen_off;

			/* Skip if the worker has not registered. */
			if (!w->registered) {
				continue;
			}

			/*
			 * Get a stable 'seen' value.  This is necessary since we
			 * want to discard the stale 'seen' values.
			 */
			while ((seen_off = w->seen_off) & WRAP_LOCK_BIT) {
				SPINLOCK_BACKOFF(count);
			}

			/*
			 * Ignore the offsets after the possible wrap-around.
			 * We are interested in the smallest seen offset that is
			 * not behind the 'written' offset.
			 */
			if (seen_off >= written) {
				ready = std::min(seen_off, ready);
			}
			assert(ready >= written);
		}

		/*
		 * Finally, we need to determine whether wrap-around occurred
		 * and deduct the safe 'ready' offset.
		 */
		if (next < written) {
			const ringbuf_off_t end = std::min(ringbuf_off_t(this->_space), this->_end);

			/*
			 * Wrap-around case.  Check for the cut off first.
			 *
			 * Reset the 'written' offset if it reached the end of
			 * the buffer or the 'end' offset (if set by a producer).
			 * However, we must check that the producer is actually
			 * done (the observed 'ready' offsets are clear).
			 */
			if (ready == RBUF_OFF_MAX && written == end) {
				/*
				 * Clear the 'end' offset if was set.
				 */
				if (this->_end != RBUF_OFF_MAX) {
					this->_end = RBUF_OFF_MAX;
					atomic_thread_fence(memory_order_stores);
				}
				/* Wrap-around the consumer and start from zero. */
				this->_written = written = 0;
				goto retry;
			}

			/*
			 * We cannot wrap-around yet; there is data to consume at
			 * the end.  The ready range is smallest of the observed
			 * 'ready' or the 'end' offset.  If neither is set, then
			 * the actual end of the buffer.
			 */
			assert(ready > next);
			ready = std::min(ready, end);
			assert(ready >= written);
		} else {
			/*
			 * Regular case.  Up to the observed 'ready' (if set)
			 * or the 'next' offset.
			 */
			ready = std::min(ready, next);
		}
		towrite = ready - written;

		assert(ready >= written);
		assert(towrite <= this->_space);
		
		ConsumptionAttempt rval;
		rval._readptr = this->_storage+written;
		rval._length = towrite;
		return rval;
	}

	/*
	 * ringbuf_release: indicate that the consumed range can now be released.
	 */

	inline void
	dequeued(size_t numbytes_consumed)
	{
		const size_t nwritten = this->_written + numbytes_consumed;

		assert(this->_written <= this->_space);
		assert(this->_written <= this->_end);
		assert(nwritten <= this->_space);

		this->_written = (nwritten == this->_space) 
		             ? 0 
		             : nwritten;
	}
};


} //namespace cringbuf {
