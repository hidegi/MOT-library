#ifndef _SHARED_MUTEX_H_
#define _SHARED_MUTEX_H_
#include "SP/config.h"
#include <atomic>
#include <array>
#include <thread>

// this should be defined in the Makefile
// if not defined, use what is most common
// for x86_64 CPUs in 2017...
#ifndef LEVEL1_DCACHE_LINESIZE
#define LEVEL1_DCACHE_LINESIZE	64
#endif

namespace sp {
	template<size_t N>
	class shared_mutex_base {
		// purpose of this structure is to hold
		// status of each individual bucket-mutex
		// object
		// Ideally each thread should be mapped to
		// one entry only of 'el_' during its
		// lifetime
		struct entry_lock {
			const static size_t	W_MASK = 0x8000000000000000,
                                    R_MASK = ~W_MASK;

			// purpose ot this variable is to hold
			// in the first bit (W_MASK) if we're locking
			// in exclusive mode, otherwise use the
			// reamining 63 bits to count how many R/O
			// locks we share in this very bucket
			std::atomic<size_t>	wr_lock;

			entry_lock() : wr_lock(0) {
			}
		}
#ifdef SP_PLATFORM_WINDOWS
            __declspec(aligned(LEVEL1_DCACHE_LINESIZE));
#else
            __attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));
#endif
		// array holding all the buckets
		std::array<entry_lock, N>	el_;
		// atomic variable used to initialize thread
		// ids so that they should evenly spread
		// across all the buckets
		static std::atomic<size_t>	idx_hint_;
		// lock-free function to return a 'unique' id
		static size_t get_hint_idx(void) {
			while(true) {
				size_t cur_hint = idx_hint_.load();
				if(idx_hint_.compare_exchange_weak(cur_hint, cur_hint+1))
					return cur_hint;
			}
		}
		// get index for given thread
		// could hav used something like std::hash<std::thread::id>()(std::this_thread::get_id())
		// but honestly using a controlled idx_hint_
		// seems to be better in terms of putting threads
		// into buckets evenly
		// note - thread_local is supposed to be static...
		inline static size_t get_thread_idx(void) {
			const thread_local size_t	rv = get_hint_idx()%N;
			return rv;
		}
	public:
		shared_mutex_base() {
		}

		void lock_shared(void) {
			// try to replace the wr_lock with current value incremented by one
			while(true) {

				size_t cur_rw_lock = el_[get_thread_idx()].wr_lock.load();
				if(entry_lock::W_MASK & cur_rw_lock) {
					// if someone has got W access yield and retry...
					std::this_thread::yield();
					continue;
				}
				if(el_[get_thread_idx()].wr_lock.compare_exchange_weak(cur_rw_lock, cur_rw_lock+1))
					break;
			}
		}

		void unlock_shared(void) {
			// try to decrement the count
			while(true) {
				size_t	cur_rw_lock = el_[get_thread_idx()].wr_lock.load();
#ifndef _RELEASE
				if(entry_lock::W_MASK & cur_rw_lock)
					throw std::runtime_error("Fatal: unlock_shared but apparently this entry is W_MASK locked!");
#endif //_RELEASE
				if(el_[get_thread_idx()].wr_lock.compare_exchange_weak(cur_rw_lock, cur_rw_lock-1))
					break;
			}
		}

		void lock(void) {
			for(size_t i = 0; i < N; ++i) {
				// acquire all locks from all buckets
				while(true) {
					size_t	cur_rw_lock = el_[i].wr_lock.load();
					if(cur_rw_lock != 0) {
						std::this_thread::yield();
						continue;
					}
					// if cur_rw_lock is 0 then proceed
					if(el_[i].wr_lock.compare_exchange_weak(cur_rw_lock, entry_lock::W_MASK))
						break;
				}
			}
		}

		void unlock(void) {
			for(size_t i = 0; i < N; ++i) {
				// release all locks
				while(true) {
					size_t	cur_rw_lock = el_[i].wr_lock.load();
#ifndef _RELEASE
					if(cur_rw_lock != entry_lock::W_MASK)
						throw std::runtime_error("Fatal: unlock but apparently this entry is shared locked or uninitialized!");
#endif //_RELEASE
					// then proceed resetting to 0
					if(el_[i].wr_lock.compare_exchange_weak(cur_rw_lock, 0))
						break;
				}
			}
		}

		~shared_mutex_base() {
		}
	};

	template<size_t N>
	std::atomic<size_t> shared_mutex_base<N>::idx_hint_{0};

	// utility class for exclusive RAII lock
	template<size_t N>
	class exclusive_lock_base {
		shared_mutex_base<N>&	sm_;
	public:
		exclusive_lock_base(shared_mutex_base<N>& sm) : sm_(sm) {
			sm_.lock();
		}

		~exclusive_lock_base() {
			sm_.unlock();
		}
	};

	// utility class for share RAII lock
	template<size_t N>
	class shared_lock_base
	{
		shared_mutex_base<N>&	sm_;
	public:
		shared_lock_base(shared_mutex_base<N>& sm) : sm_(sm) {
			sm_.lock_shared();
		}

		~shared_lock_base() {
			sm_.unlock_shared();
		}
	};
using shared_lock = shared_lock_base<4>;
using shared_mutex = shared_mutex_base<4>;
using exclusive_lock = exclusive_lock_base<4>;
}

#endif //_SHARED_MUTEX_H_