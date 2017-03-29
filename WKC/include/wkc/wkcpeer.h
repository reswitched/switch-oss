/*
 *  wkcpeer.h
 *
 *  Copyright(c) 2009-2016 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_PEER_H_
#define _WKC_PEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>

/**
   @file
   @brief WKC standard peers.
 */
/*@{*/

WKC_BEGIN_C_LINKAGE

// timer peer
/** @brief Callback function called when timer times out */
typedef bool (*wkcTimeoutProc) (void*);

/**
@brief Initializes Timer Peer layer
@param "in_proc" Pointer to function that calls WKC::WKCTimerEventHandler::requestWakeUp() implemented by application
@param "in_cancel_proc" Pointer to function that calls WKC::WKCTimerEventHandler::cancelWakeUp() implemented by application
@retval "!= false" Succeeded in initialization
@retval "== false" Failed to initialize
@details
Performs the initialization required for the Timer Peer layer.
For related information, see the description of WKC::WKCTimerEventHandler in \"NetFront Browser NX $(NETFRONT_NX_VERSION) WKC Browser/RSS API Reference\".
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool wkcTimerInitializePeer(bool(*in_proc)(void*, wkcTimeoutProc, void*), void(*in_cancel_proc)(void*));
/**
@brief Finalizes Timer Peer layer
@details
Performs the finalization process required for the Timer Peer layer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcTimerFinalizePeer(void);
/**
@brief Forcibly terminates Timer Peer layer
@details
Forcibly terminates the Timer Peer layer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcTimerForceTerminatePeer(void);
/**
@brief Generates timer
@retval "!= 0" Pointer to timer object
@retval "== 0" Failed to generate timer
@details
Generates timer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void* wkcTimerNewPeer(void);
/**
@brief Discards timer
@param "in_timer" Pointer to timer object
@details
Discards timer.
@attention
in_timer must be a value obtained by wkcTimerNewPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcTimerDeletePeer(void* in_timer);
/**
@brief Starts timer that fires only one time
@param "in_timer" Pointer to timer object
@param "in_timeout" Time period until timeout (sec)
@param "in_proc" Pointer to callback function called when timeout occurs
@param "in_data" Data used for in_proc argument
@retval "!= false" Succeeded in starting
@retval "== false" Failed to start
@details
Starts the timer that fires only one time after in_timeout.
@attention
in_timer must be a value obtained by wkcTimerNewPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool wkcTimerStartOneShotPeer(void* in_timer, double in_timeout, wkcTimeoutProc in_proc, void* in_data);
/**
@brief Starts timer that fires only one time
@param "in_timer" Pointer to timer object
@param "in_timeout" Time period until timeout (sec)
@param "in_proc" Pointer to callback function called when timeout occurs
@param "in_data" Data used for in_proc argument
@retval "!= false" Succeeded in starting
@retval "== false" Failed to start
@details
Same as wkcTimerStartOneShotPeer, but call in_proc directly.
@attention
in_timer must be a value obtained by wkcTimerNewPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool wkcTimerStartOneShotDirectPeer(void* in_timer, double in_timeout, wkcTimeoutProc in_proc, void* in_data);
/**
@brief Starts timer that fires periodically at regular intervals
@param "in_timer" Pointer to timer object
@param "in_timeout" Time period until timeout (sec)
@param "in_proc" Pointer to callback function called when timeout occurs
@param "in_data" Data used for in_proc argument
@retval "!= false" Succeeded in starting
@retval "== false" Failed to start
@details
Starts the timer that fires periodically after in_timeout intervals.
@attention
in_timer must be a value obtained by wkcTimerNewPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool wkcTimerStartPeriodicPeer(void* in_timer, double in_timeout, wkcTimeoutProc in_proc, void* in_data);
/**
@brief Stops timer
@param "in_timer" Pointer to timer object
@details
Stops timer.
@attention
in_timer must be a value obtained by wkcTimerNewPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcTimerCancelPeer(void* in_timer);
/**
@brief 
@param "in_timer" Pointer to timer object
@param "in_data" Data passed to timer
@details
Starts a timer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcTimerWakeUpPeer(void* in_timer, void* in_data);

/*
@brief 
@param "in_data" Data passed to timer
@details Starts a timer.
@remarks The existing implementation can be used for this function without change.
@attention Depricated
*/
//WKC_PEER_API void wkcTimerWakeUpPeer(void* in_data);

WKC_PEER_API bool wkcDebugPrintInitializePeer(void);
/**
@brief Finalizes debug output function
@details
Performs the finalization process of debug output function. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcDebugPrintFinalizePeer(void);
/**
@brief Forcibly terminates debug output function
@details
Forcibly terminates process of debug output function. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcDebugPrintForceTerminatePeer(void);
/**
@brief Debug output (formatted)
@param in_fmt Formatted string (C string)
       Conforms to printf function
@param ... (argument list)  Output data
@details
Outputs debug information.
The device to which the debug information is actually output depends on the platform.
@note it will be duplicated.
*/
WKC_PEER_API void wkcDebugPrintfPeer(const char* in_fmt, ...);
/**
@brief  Debug output
@param in_str Debug string
@details
Outputs debug information.
The device to which the debug information is actually output depends on the platform.
@note it will be duplicated.
*/
WKC_PEER_API void wkcDebugPutsPeer(const char* in_str);

// backtrace
WKC_PEER_API int wkcDebugGetBacktracePeer(void** in_buffer, int in_size);
WKC_PEER_API void wkcDebugPrintBacktracePeer(void** in_buffer, int in_size);

// Packet Capture Peer
WKC_PEER_API bool wkcPCAPInitializePeer(const char *in_prefix);
WKC_PEER_API void wkcPCAPFinalizePeer(void);
WKC_PEER_API void wkcPCAPForceTerminatePeer(void);

// tick peer
/**
@brief Gets Tick (elapsed time)
@retval Time elapsed from some moment (ms)
@details
Returns the time elapsed from some moment such as system start in milliseconds.
This function is used to check the elapsed time from some point to another point.
To check the elapsed time from point A to point B, for example, call this function at the points A and B, and do subtraction with the values returned by this function (B ? A).
*/
WKC_PEER_API unsigned int wkcGetTickCountPeer(void); // in ms

/**
@brief Gets Tick base time from epoch
@retval Time elapsed from epoch (ms)
@details
Returns the offset time from epocj in milliseconds.
If wkcGetTickCountPeer() returns the time from epoch, this function should return 0.
If not, return some offset time since epoch.
*/
WKC_PEER_API unsigned int wkcGetBaseTickCountPeer(void); // in ms

/**
@brief Gets Monotonically Time
@retval Monotonically Time (double)
*/
WKC_PEER_API double wkcGetMonotonicallyIncreasingTimePeer(void);

// thread peer
/** @brief Function that is executed in generated thread */
typedef void* (*wkcThreadProc)(void *);

/** @brief Function that is set thread priority. */
typedef void (*wkcSetThreadPriorityProc)(const char* in_name, int* out_priority, int* out_core);

/**
@brief Initializes Thread Peer layer
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the initialization required for the Thread Peer layer.
*/
WKC_PEER_API bool wkcThreadInitializePeer(void);
/**
@brief Finalizes Thread Peer layer
@details
Performs the finalization process required for the Thread Peer layer.
*/
WKC_PEER_API void wkcThreadFinalizePeer(void);
/**
@brief Forcibly terminates Thread Peer layer
@details
Forcibly terminates the Thread Peer layer.
*/
WKC_PEER_API void wkcThreadForceTerminatePeer(void);
/**
@brief Generates thread
@param "in_proc" Pointer to function that executes in thread to generate
@param "in_data" Data used for in_proc argument
@param "in_name" Thread name.
@retval "!= 0" Pointer to thread object
@retval "== 0" Failed to generate thread
@details
Generates thread.
*/
WKC_PEER_API void* wkcThreadCreatePeer(wkcThreadProc in_proc, void* in_data, const char* in_name);
/**
@brief Waits for thread termination
@param "in_thread" Pointer to thread object
@param "out_result" Pointer to the area to which the return value of in_thread thread is stored
@retval "== 0" Succeeded
@retval "!= 0" Failed
@details
Waits for in_thread thread termination.
@attention
- in_thread must be a value obtained by wkcThreadCreatePeer().
- The behavior must be the same as the pthread_join() function.
*/
WKC_PEER_API int wkcThreadJoinPeer(void* in_thread, void** out_result);
/**
@brief Detaches thread
@param "in_thread" Pointer to thread object
@details
Sets the running in_thread thread to the detached state.
@attention
- in_thread must be a value obtained by wkcThreadCreatePeer().
- The behavior must be the same as the pthread_detach() function.
*/
WKC_PEER_API void wkcThreadDetachPeer(void* in_thread);
WKC_PEER_API void wkcThreadSetCurrentThreadNamePeer(const char* in_name);
/**
@brief Gets current thread
@retval Pointer to thread object, or thread ID
@details
Returns the value that identifies the current thread.
*/
WKC_PEER_API void* wkcThreadCurrentThreadPeer(void);
/**
@brief Verify whether current thread is main thread.
@retval If current thread is main thread, return true. otherwise return false.
@details
Verify whether current thread is main thread.
*/
WKC_PEER_API bool wkcThreadCurrentIsMainThreadPeer(void);
/**
@brief Gets thread stack base
@param "in_thread" Pointer to thread object
@retval Address of in_thread thread stack base
@details
Gets an in_thread thread stack base.
@attention
in_thread must be a value obtained by wkcThreadCreatePeer().
*/
WKC_PEER_API void* wkcThreadGetStackBasePeer(void* in_thread);
/**
@brief Gets thread stack size
@param "in_thread" Pointer to thread object
@retval Stack size (bytes)
@details
Gets the stack size of in_thread thread set by wkcThreadSetStackSizePeer().
@attention
in_thread must be a value obtained by wkcThreadCreatePeer().
*/
WKC_PEER_API unsigned int wkcThreadGetStackSizePeer(void* in_thread);
/**
@brief Sets thread stack size
@param "in_thread" Pointer to thread object
@param "in_stack_size" Stack size (bytes)
@details
Sets an in_thread thread stack size.
@attention
in_thread must be a value obtained by wkcThreadCreatePeer().
*/
WKC_PEER_API void wkcThreadSetStackSizePeer(void* in_thread, unsigned int in_stack_size);
/**
@brief Compares thread
@param "in_thread1" Pointer to thread object
@param "in_thread2" Pointer to thread object
@retval "!= 0" Two threads are the same
@retval "== 0" Two theads are different
@details
Checks whether in_thread1 is the same thread as in_thread2.
@attention
in_thread1 and in_thread2 must be values obtained by wkcThreadCreatePeer().
*/
WKC_PEER_API int wkcThreadEqualPeer(void* in_thread1, void* in_thread2);

/**
@brief Interrupts thread execution
@param "in_thread" Pointer to thread object
@details
Interrupts the in_thread thread.
@attention
in_thread must be a value obtained by wkcThreadCreatePeer().@n
This peer will be called at:
@li garbarge collection
@li memory crisis

to suspend threads except the event happened.
*/
WKC_PEER_API void wkcThreadSuspendPeer(void* in_thread);
/**
@brief Resumes execution of thread that was interrupted
@param "in_thread" Pointer to thread object
@details
Resumes execution of in_thread thread that was interrupted.
@attention
in_thread must be a value obtained by wkcThreadCreatePeer().@n
This peer will be called to threads those are suspended by wkcThreadSuspendPeer when a garbage collection is finished.
*/
WKC_PEER_API void wkcThreadResumePeer(void* in_thread);
/**
@brief Interrupts all thread execution
@details
Suspends all threads.
*/
WKC_PEER_API void wkcThreadSuspendAllThreadsPeer(void);
/**
@brief Interrupts current thread execution
@param "in_offsettime_ms" Duration for which execution is interrupted (ms)
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Interrupts the current thread execution for in_offsettime_ms (milliseconds).
*/
WKC_PEER_API bool wkcThreadTimedSleepPeer(unsigned int in_offsettime_ms);
/**
@brief Interrupts current thread execution
@param "in_offsettime_microsec" Duration for which execution is interrupted (microseconds)
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Interrupts the current thread execution for in_offsettime_microsec (microseconds).
*/
WKC_PEER_API bool wkcThreadTimedSleepInMicroSecondsPeer(unsigned int in_offsettime_microsec);
/**
@brief Halts current thread
@details
Halts the current thread so that other threads can be executed.
*/
WKC_PEER_API void wkcThreadYieldPeer(void);
/**
@brief Sets information related to main thread of browser engine
@param "in_id" Thread ID
@param "in_stackbase" Stack base address
@details
Sets information related to main thread of browser engine. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcThreadSetMainThreadInfoPeer(void* in_id, void* in_stackbase);
/**
@brief Stores current register value of current thread
@param "in_buf" Pointer to area that stores register value
@param "in_size" Size of in_buf (bytes)
@details
Stores current register value of current thread to in_buf.
*/
WKC_PEER_API void wkcThreadStoreRegistersPeer(void* in_buf, int in_size);
/**
@brief Stores information related to threads
@param "in_thread" Pointer to thread object
@param "out_buf" Pointer to area that stores register value
@param "in_bufsize" Size of out_buf (bytes)
@retval Size of which is written to out_buf (bytes)
@details
Stores information related to in_thread thread in out_buf.
This function is used for getting data necessary for the argument wkcThreadGetStackPointerPeer().
*/
WKC_PEER_API int wkcThreadStoreThreadRegistersPeer(void* in_thread, void* out_buf, int in_bufsize);
/**
@brief Gets stack pointer of other thread
@param "in_buf" Buffer that stores information related to threads
@details
Gets a stack pointer from in_buf data.
@attention
in_buf must be a value obtained by wkcThreadStoreThreadRegistersPeer().
*/
WKC_PEER_API void* wkcThreadGetStackPointerPeer(const void* in_buf);

/**
@brief Increments variables shared by multiple threads
@param "in_addend" Pointer to variables that should be incremented
@retval Incremented value
@details
Safely increments the in_addend variable shared by multiple threads.
*/
WKC_PEER_API int wkcThreadAtomicIncrementPeer(int volatile* in_addend);
/**
@brief Decrements variables shared by multiple threads
@param "in_addend" Pointer to variables that should be decremented
@retval Decremented value
@details
Safely decrements the in_addend variable shared by multiple threads.
*/
WKC_PEER_API int wkcThreadAtomicDecrementPeer(int volatile* in_addend);

/**
@brief Checks current thread is force-terminated or not
@details
Checks current thread is force-terminated or not.
*/
WKC_PEER_API void wkcThreadCheckAlivePeer(void);

/**
@brief Terminate current thread
@details
Terminate current thread.
*/
WKC_PEER_API void wkcThreadTerminateCurrentThreadPeer(void);

/**
@brief Generates key that identifies thread-specific data area
@param "out_key" Conforms to pthread_key_create()
@param "in_destfunc" Conforms to pthread_key_create()
@retval Conforms to pthread_key_create()
@details
Generates key that identifies thread-specific data area.
@attention
The behavior must be the same as the pthread_key_create() function.
*/
WKC_PEER_API int wkcThreadKeyNewPeer(void** out_key, void(*in_destfunc)(void*));
/**
@brief Discards key that identifies thread-specific data area
@param "in_key" Pointer to key that identifies thread-specific data area
@details
Discards the key that identifies thread-specific data area.
@attention
- in_key must be a value generated by wkcThreadKeyNewPeer().
- The behavior must be the same as the pthread_key_delete() function.
*/
WKC_PEER_API void wkcThreadKeyDeletePeer(void* in_key);
/**
@brief Sets thread-specific data
@param "in_key" Pointer to key that identifies thread-specific data area
@param "in_ptr" Value that is bound to in_key key in called thread
@details
Sets the value that is bound to in_key key in called thread to in_ptr.
@attention
- in_key must be a value generated by wkcThreadKeyNewPeer().
- The behavior must be the same as the pthread_key_setspecific() function.
*/
WKC_PEER_API void wkcThreadKeySetSpecificPeer(void* in_key, void* in_ptr);
/**
@brief Gets thread-specific data
@param "in_key" Pointer to key that identifies thread-specific data area
@retval Conforms to pthread_key_getspecific()
@details
Returns the value that is bound to the in_key key in called thread at the time.
@attention
- in_key must be a value generated by wkcThreadKeyNewPeer().
- The behavior must be the same as the pthread_key_getspecific() function.
*/
WKC_PEER_API void* wkcThreadKeyGetSpecificPeer(void* in_key);

/**
@brief Generates mutex
@retval "!= 0" Pointer to mutex object
@retval "== 0" Failed to generate
@details
Generates mutex.
*/
WKC_PEER_API void* wkcMutexNewPeer(void);
/**
@brief Discards mutex
@param "in_mutex" Pointer to mutex object
@details
Discards mutex.
@attention
in_mutex must be a value obtained by wkcMutexNewPeer().
*/
WKC_PEER_API void wkcMutexDeletePeer(void* in_mutex);
/**
@brief Locks mutex
@param "in_mutex" Pointer to mutex object
@details
Locks the in_mutex mutex.
@attention
in_mutex must be a value obtained by wkcMutexNewPeer().
*/
WKC_PEER_API void wkcMutexLockPeer(void* in_mutex);
/**
@brief Tries to lock mutex
@param "in_mutex" Pointer to mutex object
@retval "!= false" Succeeded in locking
@retval "== false" Failed to lock, since other thread(s) has locked
@details
Tries to lock mutex.
@attention
in_mutex must be a value obtained by wkcMutexNewPeer().
*/
WKC_PEER_API bool wkcMutexTryLockPeer(void* in_mutex);
/**
@brief Unlocks mutex
@param "in_mutex" Pointer to mutex object
@details
Unlocks in_mutex mutex.
@attention
in_mutex must be a value obtained by wkcMutexNewPeer().
*/
WKC_PEER_API void wkcMutexUnlockPeer(void* in_mutex);
/**
@brief Unlocks all mutex
@details
Unlocks all mutex.
*/
WKC_PEER_API void wkcMutexUnlockAllPeer();
/**
@brief Generates condition variable.
@retval "!= 0" Pointer to condition variable object
@retval "== 0" Failed to generate
@details
Generates a condition variable.
*/
WKC_PEER_API void* wkcCondNewPeer(void);
/**
@brief Discards condition variable
@param "in_cond" Pointer to condition variable object
@details
Discards a condition variable.
@attention
in_cond must be a value obtained by wkcCondNewPeer().
*/
WKC_PEER_API void wkcCondDeletePeer(void *in_cond);
/**
@brief Unlocks mutex and waits for sending condition variable
@param "in_cond" Pointer to condition variable object
@param "in_mutex" Pointer to mutex object
@details
Unlocks the in_mutex mutex and waits for sending in_cond condition variable in one go, then halts thread execution until the condition variable is sent.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- in_mutex must be a value obtained by wkcMutexNewPeer().
- The behavior must be the same as the pthread_cond_wait() function.
*/
WKC_PEER_API void wkcCondWaitPeer(void* in_cond, void* in_mutex);
/**
@brief Unlocks mutex and waits for sending condition variable (with maximum waiting period specified)
@param "in_cond" Pointer to condition variable object
@param "in_mutex" Pointer to mutex object
@param "in_offsettime" Maximum waiting period (millisec)
@retval "!= false" Succeeded
@retval "== false" Failed
@details
In addition to behavior of the wkcCondWaitPeer() function, sets maximum waiting period.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- in_mutex must be a value obtained by wkcMutexNewPeer().
- The behavior must be the same as the pthread_cond_timedwait() function.
*/
WKC_PEER_API bool wkcCondTimedWaitPeer(void* in_cond, void* in_mutex, double in_offsettime);
/**
@brief Resumes execution for one of the waiting threads
@param "in_cond" Pointer to condition variable object
@details
Resumes executing one of the threads waiting for the in_cond condition variable.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- The behavior must be the same as the pthread_cond_signal() function.
*/
WKC_PEER_API void wkcCondSignalPeer(void* in_cond);
/**
@brief Resumes execution for all waiting threads
@param "in_cond" Pointer to condition variable object
@details
Resumes executing all the threads waiting for the in_cond condition variable.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- The behavior must be the same as the pthread_cond_broadcast() function.
*/
WKC_PEER_API void wkcCondBroadcastPeer(void* in_cond);

/**
@brief Executes function once only at most
@param "in_key" Key indicating whether in_func was executed
@param "in_func" Function pointer
@details
Executes the in_func function once only at once.
@attention
The behavior must be the same as the pthread_once() function.
*/
WKC_PEER_API void wkcThreadOncePeer(void* in_key, void(*in_func)(void));

/**
@brief Set callback function that sets thread priority
@param in_proc Callback function pointer for set thread priority.
@details
Set the function that sets thread priority.
*/
WKC_PEER_API void wkcThreadSetThreadPriorityProcPeer(wkcSetThreadPriorityProc in_proc);

#ifdef WKC_LIGHTMUTEX_TYPE
typedef WKC_LIGHTMUTEX_TYPE WKCLightMutex;
#else
/**
   @brief LightMutex structure@n
   LightMutex is a mutex whose processing cost is low for discarding mutex object generation (without memory allocation).
 */
struct WKCLightMutex_ {
    void* fMutex;
    long fLockWord;
};
/** @brief LightMutex structure */
typedef struct WKCLightMutex_ WKCLightMutex;
#endif
/**
@brief Initializes LightMutex object
@param "in_mutex" Pointer to LightMutex object
@details
Initializes the LightMutex object.
*/
WKC_PEER_API void wkcLightMutexInitializePeer(WKCLightMutex* in_mutex);
/**
@brief Finalizes LightMutex object
@param "in_mutex" Pointer to LightMutex object
@details
Finalizes the LightMutex object.
*/
WKC_PEER_API void wkcLightMutexFinalizePeer(WKCLightMutex* in_mutex);
/**
@brief Locks LightMutex
@param "in_mutex" Pointer to LightMutex object
@details
Locks the LightMutex in_mutex.
*/
WKC_PEER_API void wkcLightMutexLockPeer(WKCLightMutex* in_mutex);
/**
@brief Tries to lock LightMutex
@param "in_mutex" Pointer to LightMutex object
@retval "!= false" Succeeded in locking
@retval "== false" Failed to lock, since other thread(s) has locked
@details
Tries to lock LightMutex in_mutex.
*/
WKC_PEER_API bool wkcLightMutexTryLockPeer(WKCLightMutex* in_mutex);
/**
@brief Unlocks LightMutex
@param "in_mutex" Pointer to LightMutex object
@details
Unlocks LightMutex in_mutex.
*/
WKC_PEER_API void wkcLightMutexUnlockPeer(WKCLightMutex* in_mutex);
/**
@brief Unlocks LightMutex and waits for sending condition variable
@param "in_cond" Pointer to condition variable object
@param "in_mutex" Pointer to LightMutex object
@details
Unlocks the LightMutex in_mutex and waits for sending in_cond condition variable in one go, then halts thread execution until the condition variable is sent.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- The behavior must be the same as the pthread_cond_wait() function.
*/
WKC_PEER_API void wkcCondWaitLightPeer(void* in_cond, WKCLightMutex* in_mutex);
/**
@brief Unlocks LightMutex and waits for sending condition variable (with maximum waiting period specified)
@param "in_cond" Pointer to condition variable object
@param "in_mutex" Pointer to LightMutex object
@param "in_offsettime" Maximum waiting period (millisec)
@retval "!= false" Succeeded
@retval "== false" Failed
@details
In addition to behavior of the wkcCondWaitLightPeer() function, sets maximum waiting period.
@attention
- in_cond must be a value obtained by wkcCondNewPeer().
- The behavior must be the same as the pthread_cond_timedwait() function.
*/
WKC_PEER_API bool wkcCondTimedWaitLightPeer(void* in_cond, WKCLightMutex* in_mutex, double in_offsettime);

/**
@brief (TBD)
@param "in_path" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void* wkcDLLLoadPeer(const char* in_path);
/**
@brief (TBD)
@param "in_dll" (TBD)
@param "in_symbol" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void* wkcDLLGetSymbolPeer(void* in_dll, const char* in_symbol);
/**
@brief (TBD)
@param "in_dll" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcDLLUnloadPeer(void* in_dll);

// unicode peer
#define WKC_UNICODE_CATEGORY_NOCATEGORY              0x00000000
#define WKC_UNICODE_CATEGORY_OTHERNOTASSIGNED        0x00000001
#define WKC_UNICODE_CATEGORY_LETTERUPPERCASE         0x00000002
#define WKC_UNICODE_CATEGORY_LETTERLOWERCASE         0x00000004
#define WKC_UNICODE_CATEGORY_LETTERTITLECASE         0x00000008
#define WKC_UNICODE_CATEGORY_LETTERMODIFIER          0x00000010
#define WKC_UNICODE_CATEGORY_LETTEROTHER             0x00000020

#define WKC_UNICODE_CATEGORY_MARKNONSPACING          0x00000040
#define WKC_UNICODE_CATEGORY_MARKENCLOSING           0x00000080
#define WKC_UNICODE_CATEGORY_MARKSPACINGCOMBINING    0x00000100

#define WKC_UNICODE_CATEGORY_NUMBERDECIMALDIGIT      0x00000200
#define WKC_UNICODE_CATEGORY_NUMBERLETTER            0x00000400
#define WKC_UNICODE_CATEGORY_NUMBEROTHER             0x00000800

#define WKC_UNICODE_CATEGORY_SEPARATORSPACE          0x00001000
#define WKC_UNICODE_CATEGORY_SEPARATORLINE           0x00002000
#define WKC_UNICODE_CATEGORY_SEPARATORPARAGRAPH      0x00004000

#define WKC_UNICODE_CATEGORY_OTHERCONTROL            0x00008000
#define WKC_UNICODE_CATEGORY_OTHERFORMAT             0x00010000
#define WKC_UNICODE_CATEGORY_OTHERPRIVATEUSE         0x00020000
#define WKC_UNICODE_CATEGORY_OTHERSURROGATE          0x00040000

#define WKC_UNICODE_CATEGORY_PUNCTUATIONDASH         0x00080000
#define WKC_UNICODE_CATEGORY_PUNCTUATIONOPEN         0x00100000
#define WKC_UNICODE_CATEGORY_PUNCTUATIONCLOSE        0x00200000
#define WKC_UNICODE_CATEGORY_PUNCTUATIONCONNECTOR    0x00400000
#define WKC_UNICODE_CATEGORY_PUNCTUATIONOTHER        0x00800000

#define WKC_UNICODE_CATEGORY_SYMBOLMATH              0x01000000
#define WKC_UNICODE_CATEGORY_SYMBOLCURRENCY          0x02000000
#define WKC_UNICODE_CATEGORY_SYMBOLMODIFIER          0x04000000
#define WKC_UNICODE_CATEGORY_SYMBOLOTHER             0x08000000

#define WKC_UNICODE_CATEGORY_PUNCTUATIONINITIALQUOTE 0x10000000
#define WKC_UNICODE_CATEGORY_PUNCTUATIONFINALQUOTE   0x20000000

enum {
    WKC_UNICODE_DIRECTION_LEFTTORIGHT = 0,
    WKC_UNICODE_DIRECTION_RIGHTTOLEFT,
    WKC_UNICODE_DIRECTION_RIGHTTOLEFTARABIC,
    WKC_UNICODE_DIRECTION_LEFTTORIGHTEMBEDDING,
    WKC_UNICODE_DIRECTION_RIGHTTOLEFTEMBEDDING,
    WKC_UNICODE_DIRECTION_LEFTTORIGHTOVERRIDE,
    WKC_UNICODE_DIRECTION_RIGHTTOLEFTOVERRIDE,
    WKC_UNICODE_DIRECTION_POPDIRECTIONALFORMAT,
    WKC_UNICODE_DIRECTION_EUROPEANNUMBER,
    WKC_UNICODE_DIRECTION_ARABICNUMBER,
    WKC_UNICODE_DIRECTION_EUROPEANNUMBERSEPARATOR,
    WKC_UNICODE_DIRECTION_EUROPEANNUMBERTERMINATOR,
    WKC_UNICODE_DIRECTION_COMMONNUMBERSEPARATOR,
    WKC_UNICODE_DIRECTION_NONSPACINGMARK,
    WKC_UNICODE_DIRECTION_BOUNDARYNEUTRAL,
    WKC_UNICODE_DIRECTION_BLOCKSEPARATOR,
    WKC_UNICODE_DIRECTION_SEGMENTSEPARATOR,
    WKC_UNICODE_DIRECTION_WHITESPACENEUTRAL,
    WKC_UNICODE_DIRECTION_OTHERNEUTRAL,
    WKC_UNICODE_DIRECTION_LEFTTORIGHTISOLATE,
    WKC_UNICODE_DIRECTION_RIGHTTOLEFTISOLATE,
    WKC_UNICODE_DIRECTION_FIRSTSTRONGISOLATE,
    WKC_UNICODE_DIRECTION_POPDIRECTIONALISOLATE,
};

enum {
    WKC_UNICODE_LINEBREAKCATEGORY_BK = 0,
    WKC_UNICODE_LINEBREAKCATEGORY_CR,
    WKC_UNICODE_LINEBREAKCATEGORY_LF,
    WKC_UNICODE_LINEBREAKCATEGORY_CM,
    WKC_UNICODE_LINEBREAKCATEGORY_SG,
    WKC_UNICODE_LINEBREAKCATEGORY_GL,
    WKC_UNICODE_LINEBREAKCATEGORY_CB,
    WKC_UNICODE_LINEBREAKCATEGORY_SP,
    WKC_UNICODE_LINEBREAKCATEGORY_ZW,
    WKC_UNICODE_LINEBREAKCATEGORY_NL,
    WKC_UNICODE_LINEBREAKCATEGORY_WJ,
    WKC_UNICODE_LINEBREAKCATEGORY_JL,
    WKC_UNICODE_LINEBREAKCATEGORY_JV,
    WKC_UNICODE_LINEBREAKCATEGORY_JT,
    WKC_UNICODE_LINEBREAKCATEGORY_H2,
    WKC_UNICODE_LINEBREAKCATEGORY_H3,
    WKC_UNICODE_LINEBREAKCATEGORY_XX,
    WKC_UNICODE_LINEBREAKCATEGORY_OP,
    WKC_UNICODE_LINEBREAKCATEGORY_CL,
    WKC_UNICODE_LINEBREAKCATEGORY_CP,
    WKC_UNICODE_LINEBREAKCATEGORY_QU,
    WKC_UNICODE_LINEBREAKCATEGORY_NS,
    WKC_UNICODE_LINEBREAKCATEGORY_EX,
    WKC_UNICODE_LINEBREAKCATEGORY_SY,
    WKC_UNICODE_LINEBREAKCATEGORY_IS,
    WKC_UNICODE_LINEBREAKCATEGORY_PR,
    WKC_UNICODE_LINEBREAKCATEGORY_PO,
    WKC_UNICODE_LINEBREAKCATEGORY_NU,
    WKC_UNICODE_LINEBREAKCATEGORY_AL,
    WKC_UNICODE_LINEBREAKCATEGORY_ID,
    WKC_UNICODE_LINEBREAKCATEGORY_IN,
    WKC_UNICODE_LINEBREAKCATEGORY_HY,
    WKC_UNICODE_LINEBREAKCATEGORY_BB,
    WKC_UNICODE_LINEBREAKCATEGORY_BA,
    WKC_UNICODE_LINEBREAKCATEGORY_SA,
    WKC_UNICODE_LINEBREAKCATEGORY_AI,
    WKC_UNICODE_LINEBREAKCATEGORY_B2,
    WKC_UNICODE_LINEBREAKCATEGORY_HL,
    WKC_UNICODE_LINEBREAKCATEGORY_CJ,
    WKC_UNICODE_LINEBREAKCATEGORY_RI,
    WKC_UNICODE_LINEBREAKCATEGORY_ZWJ,
    WKC_UNICODE_LINEBREAKCATEGORY_EB,
    WKC_UNICODE_LINEBREAKCATEGORY_EM,
    WKC_UNICODE_LINEBREAKCATEGORIES,
};

/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeToLowerPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeToUpperPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeToTitlePeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeIsPrintPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeIsAlnumPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeIsDigitPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeIsPunctPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeIsLowerPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@param "out_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API bool wkcUnicodeGetMirrorCharPeer(int in_char, int* out_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeDigitValuePeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeCategoryPeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeFoldCasePeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@param "out_buf" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeFoldCaseFullPeer(int in_char, int* out_buf);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeDirectionTypePeer(int in_char);
/**
@brief (TBD)
@param "in_char" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeLineBreakCategoryPeer(int in_char);

/**
@brief (TBD)
@param "in_str1" (TBD)
@param "in_str2" (TBD)
@param "in_len" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcUnicodeUCharMemCaseCmpPeer(const unsigned short* in_str1, const unsigned short* in_str2, int in_len);

enum {
    WKC_I18N_CODEC_UNKNOWN = -1,

    WKC_I18N_CODEC_UTF8 = 0,
    WKC_I18N_CODEC_UTF16BE,
    WKC_I18N_CODEC_UTF16LE,

    WKC_I18N_CODEC_ISO_8859_1,
    WKC_I18N_CODEC_ISO_8859_2,
    WKC_I18N_CODEC_ISO_8859_3,
    WKC_I18N_CODEC_ISO_8859_4,
    WKC_I18N_CODEC_ISO_8859_5,
    WKC_I18N_CODEC_ISO_8859_6,
    WKC_I18N_CODEC_ISO_8859_7,
    WKC_I18N_CODEC_ISO_8859_8,
    WKC_I18N_CODEC_ISO_8859_9,
    WKC_I18N_CODEC_ISO_8859_10,
    WKC_I18N_CODEC_TIS_620, /* a.k.a ISO_8859_11 */
    // 12 is not exist
    WKC_I18N_CODEC_ISO_8859_13,
    WKC_I18N_CODEC_ISO_8859_14,
    WKC_I18N_CODEC_ISO_8859_15,
    WKC_I18N_CODEC_ISO_8859_16,

    WKC_I18N_CODEC_CP437,
    WKC_I18N_CODEC_CP737,
    WKC_I18N_CODEC_CP850,
    WKC_I18N_CODEC_CP852,
    WKC_I18N_CODEC_CP855,
    WKC_I18N_CODEC_CP857,
    WKC_I18N_CODEC_CP860,
    WKC_I18N_CODEC_CP861,
    WKC_I18N_CODEC_CP863,
    WKC_I18N_CODEC_CP865,
    WKC_I18N_CODEC_CP866,
    WKC_I18N_CODEC_CP869,
    WKC_I18N_CODEC_CP874,
    WKC_I18N_CODEC_CP949,
    WKC_I18N_CODEC_CP950,

    WKC_I18N_CODEC_CP1250,
    WKC_I18N_CODEC_CP1251,
    WKC_I18N_CODEC_CP1252,
    WKC_I18N_CODEC_CP1253,
    WKC_I18N_CODEC_CP1254,
    WKC_I18N_CODEC_CP1255,
    WKC_I18N_CODEC_CP1256,
    WKC_I18N_CODEC_CP1257,
    WKC_I18N_CODEC_CP1258,

    WKC_I18N_CODEC_KOI8_R,
    WKC_I18N_CODEC_KOI8_U,
    WKC_I18N_CODEC_MIK,

    WKC_I18N_CODEC_ISCII,
    WKC_I18N_CODEC_TSCII,
    WKC_I18N_CODEC_VISCII,

    WKC_I18N_CODEC_MAC_ROMAN,
    WKC_I18N_CODEC_MAC_CYRILLIC,
    WKC_I18N_CODEC_MAC_CENTRALEUROPE,

    WKC_I18N_CODEC_SHIFT_JIS,
    WKC_I18N_CODEC_EUC_JP,
    WKC_I18N_CODEC_ISO_2022_JP,
    WKC_I18N_CODEC_SHIFT_JIS_2004,
    WKC_I18N_CODEC_EUC_JIS_2004,
    WKC_I18N_CODEC_ISO_2022_JP_2004,

    WKC_I18N_CODEC_ISO_2022_KR,
    WKC_I18N_CODEC_EUC_KR,
    WKC_I18N_CODEC_BIG5,
    WKC_I18N_CODEC_BIG5_HKSCS,
    WKC_I18N_CODEC_GBK,
    WKC_I18N_CODEC_HZ,
    WKC_I18N_CODEC_EUC_CN,
    WKC_I18N_CODEC_GB_2312_80,
    WKC_I18N_CODEC_GB18030,

    WKC_I18N_CODECS
};

enum {
    WKC_I18N_ENCODEERRORFALLBACK_NONE = 0,
    WKC_I18N_ENCODEERRORFALLBACK_QUESTION,
    WKC_I18N_ENCODEERRORFALLBACK_ESCAPE_XML_DECIMAL,
    WKC_I18N_ENCODEERRORFALLBACK_ESCAPE_URLENCODE,
    WKC_I18N_ENCODEERRORFALLBACKS
};

enum {
    WKC_I18N_DETECT_ENCODING_NONE                = 0x00000000,
    WKC_I18N_DETECT_ENCODING_UNIVERSAL           = 0x00000001,
    WKC_I18N_DETECT_ENCODING_JAPANESE            = 0x00000002,
    WKC_I18N_DETECT_ENCODING_KOREAN              = 0x00000004,
    WKC_I18N_DETECT_ENCODING_TRADITIONAL_CHINESE = 0x00000008,
    WKC_I18N_DETECT_ENCODING_SIMPLIFIED_CHINESE  = 0x00000010,
    WKC_I18N_DETECT_ENCODING_SBCS                = 0x00000020,

    WKC_I18N_DETECT_ENCODING_NUM_OF_LAUGNAGE_SET = 7
};

/**
@brief (TBD)
@param "in_data" (TBD)
@param "in_len" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API bool wkcI18NRegisterDataPeer(const void* in_data, unsigned int in_len);

/**
@brief (TBD)
@param "in_data" (TBD)
@param "in_len" (TBD)
@param "in_hintencoding" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NDetectEncodingPeer(const char* in_data, unsigned int in_len, const char* in_hintencoding);
/**
@brief (TBD)
@param "in_language_set_flags" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NSetDetectEncodingLanguageSetPeer(int in_language_set_flags);

/**
@brief (TBD)
@param "in_dstcodec" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void* wkcI18NBeginEncodePeer(int in_dstcodec);
/**
@brief (TBD)
@param "in_obj" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NEndEncodePeer(void* in_obj);
/**
@brief (TBD)
@param "in_obj" (TBD)
@param "in_init" (TBD)
@param "out_buf" (TBD)
@param "in_buflen" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NFlushEncodeStatePeer(void* in_obj, bool in_init, char* out_buf, unsigned int in_buflen);
/**
@brief (TBD)
@param "in_obj" (TBD)
@param "in_str" (TBD)
@param "in_len" (TBD)
@param "out_buf" (TBD)
@param "in_buflen" (TBD)
@param "out_remains" (TBD)
@param "in_fallback" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NEncodePeer(void* in_obj, const unsigned short* in_str, unsigned int in_len, char* out_buf, unsigned int in_buflen, int* out_remains, int in_fallback);

/**
@brief (TBD)
@param "in_srccodec" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void* wkcI18NBeginDecodePeer(int in_srccodec);
/**
@brief (TBD)
@param "in_obj" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NEndDecodePeer(void* in_obj);
/**
@brief (TBD)
@param "in_obj" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NSaveDecodeStatePeer(void* in_obj);
/**
@brief (TBD)
@param "in_obj" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NRestoreDecodeStatePeer(void* in_obj);
/**
@brief (TBD)
@param "in_obj" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcI18NFlushDecodeStatePeer(void* in_obj);
/**
@brief (TBD)
@param "in_obj" (TBD)
@param "in_str" (TBD)
@param "in_len" (TBD)
@param "out_buf" (TBD)
@param "in_buflen" (TBD)
@param "out_remains" (TBD)
@param "in_stoponerror" (TBD)
@param "out_error" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NDecodePeer(void* in_obj, const char* in_str, unsigned int in_len, unsigned short* out_buf, unsigned int in_buflen, int* out_remains, int in_stoponerror, int* out_error);
/**
@brief (TBD)
@param "in_idn" (TBD)
@param "in_len" (TBD)
@param "out_host" (TBD)
@param "in_maxlen" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NIDNfromUnicodePeer(const unsigned short* in_idn, int in_len, unsigned char* out_host, int in_maxlen);
/**
@brief (TBD)
@param "in_host" (TBD)
@param "in_len" (TBD)
@param "out_idn" (TBD)
@param "in_maxlen" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcI18NIDNtoUnicodePeer(const unsigned char* in_host, int in_len, unsigned short* out_idn, int in_maxlen);

// text break iterator
enum {
    WKC_TEXTBREAKITERATOR_TYPE_CHARACTER = 0,
    WKC_TEXTBREAKITERATOR_TYPE_CHARACTER_NONSHARED,
    WKC_TEXTBREAKITERATOR_TYPE_WORD,
    WKC_TEXTBREAKITERATOR_TYPE_LINE,
    WKC_TEXTBREAKITERATOR_TYPE_SENTENCE,
    WKC_TEXTBREAKITERATOR_TYPE_CURSORMOVEMENT,
};

/**
@brief (TBD)
@param "in_type" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void* wkcTextBreakIteratorNewPeer(int in_type);
/**
@brief (TBD)
@param "in_self" (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcTextBreakIteratorDeletePeer(void* in_self);
/**
@brief (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcTextBreakIteratorForceTerminatePeer(void);
/**
@brief (TBD)
@param "in_self" (TBD)
@param "in_str" (TBD)
@param "in_length" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API bool wkcTextBreakIteratorSetStringPeer(void* in_self, const unsigned short* in_str, int in_length);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API void wkcTextBreakIteratorReleasePeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorFirstPeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorLastPeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorNextPeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorPreviousPeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorCurrentPeer(void* in_self);
/**
@brief (TBD)
@param "in_self" (TBD)
@param "in_pos" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorPrecedingPeer(void* in_self, int in_pos);
/**
@brief (TBD)
@param "in_self" (TBD)
@param "in_pos" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API int wkcTextBreakIteratorFollowingPeer(void* in_self, int in_pos);
/**
@brief (TBD)
@param "in_self" (TBD)
@param "in_pos" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API bool wkcTextBreakIteratorIsTextBreakPeer(void* in_self, int in_pos);
/**
@brief (TBD)
@param "in_self" (TBD)
@param "in_prior" (TBD)
@param "in_len" (TBD)
@retval (TBD)
@details
(TBD)
*/
WKC_PEER_API bool wkcTextBreakIteratorSetPriorContextPeer(void* in_self, const unsigned short* in_prior, int in_len);

// system peer
/**
@brief Initializes system
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs the initialization process of system. Implementing it is required only if needed by platform.
*/
WKC_PEER_API bool wkcSystemInitializePeer(void);
/**
@brief Finalizes system
@details
Performs the finalization process of system. Implementing it is required only if needed by platform.
*/
WKC_PEER_API void wkcSystemFinalizePeer(void);
/**
@brief Gets platform property of JavaScript Navigator object
@return platform property string
@details
Gets the value set by wkcSystemSetNavigatorPlatformPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetNavigatorPlatformPeer(void);
/**
@brief Gets product property of JavaScript Navigator object
@return product property string
@details
Gets the value set by wkcSystemSetNavigatorProductPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetNavigatorProductPeer(void);
/**
@brief Gets productSub property of JavaScript Navigator object
@return productSub property string
@details
Gets the value set by wkcSystemSetNavigatorProductSubPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetNavigatorProductSubPeer(void);
/**
@brief Gets vendor property of JavaScript Navigator object
@return vendor property string
@details
Gets the value set by wkcSystemSetNavigatorVendorPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetNavigatorVendorPeer(void);
/**
@brief Gets vendorSub property of JavaScript Navigator object
@return vendorSub property string
@details
Gets the value set by wkcSystemSetNavigatorVendorSubPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetNavigatorVendorSubPeer(void);
/**
@brief Gets language property of JavaScript Navigator object
@return language property string
@details
Gets the value set by wkcSystemSetLanguagePeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetLanguagePeer(void);
/**
@brief Gets button string of input element with type="submit" specified 
@return  Button string of input element with type="submit" specified
@details
Gets the value set by wkcSystemSetButtonLabelSubmitPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelSubmitPeer(void);
/**
@brief Gets button string of input element with type="reset" specified
@return Button string of input element with type="reset" specified
@details
Gets the value set by wkcSystemSetButtonLabelResetPeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelResetPeer(void);
/**
@brief Gets button string of input element with type="file" specified
@return Button string of input element with type="file" specified
@details
Gets the value set by wkcSystemSetButtonLabelFilePeer().
*/
WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelFilePeer(void);
/**
@brief Sets platform property of JavaScript Navigator object
@param in_platform String of platform property of JavaScript Navigator object 
*/
WKC_PEER_API void wkcSystemSetNavigatorPlatformPeer(const unsigned short* in_platform);
/**
@brief Sets product property of JavaScript Navigator object
@param in_product String of product property of JavaScript Navigator object
*/
WKC_PEER_API void wkcSystemSetNavigatorProductPeer(const unsigned short* in_product);
/**
@brief Sets productSub property of JavaScript Navigator object
@param in_product_sub String of productSub property of JavaScript Navigator object
*/
WKC_PEER_API void wkcSystemSetNavigatorProductSubPeer(const unsigned short* in_product_sub);
/**
@brief Sets vendor property of JavaScript Navigator object
@param in_vendor String of vendor property of JavaScript Navigator object
*/
WKC_PEER_API void wkcSystemSetNavigatorVendorPeer(const unsigned short* in_vendor);
/**
@brief Sets vendorSub property of JavaScript Navigator object
@param in_vendor_sub String of vendorSub property of JavaScript Navigator object
*/
WKC_PEER_API void wkcSystemSetNavigatorVendorSubPeer(const unsigned short* in_vendor_sub);
/**
@brief Sets language property of JavaScript Navigator object
@param in_language String of language property of JavaScript Navigator object
*/
WKC_PEER_API void wkcSystemSetLanguagePeer(const unsigned short* in_language);
/**
@brief Sets button string of input element with type="submit" specified 
@param in_label_submit Button string of input element with type="submit" specified
*/
WKC_PEER_API void wkcSystemSetButtonLabelSubmitPeer(const unsigned short* in_label_submit);
/**
@brief Sets button string of input element with type="reset" specified 
@param in_label_reset Button string of input element with type="reset" specified
*/
WKC_PEER_API void wkcSystemSetButtonLabelResetPeer(const unsigned short* in_label_reset);
/**
@brief Sets button string of input element with type="file" specified 
@param in_label_file Button string of input element with type="file" specified
*/
WKC_PEER_API void wkcSystemSetButtonLabelFilePeer(const unsigned short* in_label_file);
/**
@brief Sets localized string
@param in_key key string
@param in_text localized string (UTF-8)
@return true Success to set
@return false Fail to set
*/
WKC_PEER_API bool wkcSystemSetSystemStringPeer(const unsigned char* in_key, const unsigned char* in_text);
/**
@brief Gets localized string
@param in_key key string
@retval localized string
*/
WKC_PEER_API const unsigned char* wkcSystemGetSystemStringPeer(const unsigned char* in_key);

// SSL peer
/**
@brief Initializes SSL Peer layer
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Initializes the SSL Peer layer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool  wkcSSLInitializePeer(void);
/**
@brief Finalizes SSL Peer layer
@details
Finalizes the SSL Peer layer.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void  wkcSSLFinalizePeer(void);
/**
@brief Forcibly terminates SSL Peer layer
@details
Forcibly terminates the SSL Peer layer.
@attention
Do not release the memory allocated within this function.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void  wkcSSLForceTerminatePeer(void);
/**
@brief Registers trusted ROOT CA certificate
@param "cert" Pointer to buffer of data to register
@param "cert_len" Length of data to register
@retval "!= 0" void pointer of registered certificate. Used while canceling registration ( wkcSSLUnregisterRootCAPeer() ) of individual certificate
@retval "== 0" Failed to register
@details
Registers a trusted ROOT certificate.
- Data format must be PEM encoded X.509 v3 certificate data (ASN.1 definition)
- 1 item of data may include multiple certificates
- Multiple ROOT CA certificates may be registered by calling this multiple times
@attention
This API must be called after calling wkcSSLInitializePeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void* wkcSSLRegisterRootCAPeer(const char* cert, int cert_len);
/**
@brief Unregisters registered ROOT CA certificate
@param "certid" void pointer of registered certificate
@retval "== 0" Succeeded in unregistration
@retval "< 0" Failed to unregister
@details
Unregisters registered ROOT CA certificate.
@attention
certid must be the value obtained by wkcSSLRegisterRootCAPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int   wkcSSLUnregisterRootCAPeer(void* certid);
/**
@brief Unregisters all registered ROOT CA certificates
@return None
@details
Unregisters all registered ROOT CA certificates.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void  wkcSSLRootCADeleteAllPeer(void);
/**
@brief Registers trusted revoked certificate
@param "crl" Revoked certificate data
@param "crl_len" Revoked certificate data length
@retval "!= 0" void pointer of registered certificate. Used while canceling registration ( wkcSSLUnregisterCRLPeer() ) of individual certificate
@retval "== 0" Failed to register
@details
Registers a trusted CRL certificate.
- Multiple revoked certificates can be included in argument for one call.
- {}Multiple CRL certificates may be registered by calling this multiple times
- Data format must be PEM encoded X.509 v3 certificate data (ASN.1 definition)
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void* wkcSSLRegisterCRLPeer(const char* crl, int crl_len);
/**
@brief Unregisters revoked certificate
@param "crlid" void pointer of registered certificate
@retval "== 0" Succeeded in unregistration
@retval "< 0" Failed to unregister
@details
Unregisters registered revoked certificate.
@attention
crlid must be a value obtained by wkcSSLRegisterCRLPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int   wkcSSLUnregisterCRLPeer(void* crlid);
/**
@brief Unregisters all revoked certificates
@return None
@details
Unregisters all registered revoked certificates.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void  wkcSSLCRLDeleteAllPeer(void);

// only for OpenSSL peer
/**
@brief Pseudo file open function for registered certificate
@param "in_path" Certificate file path
@param "mode" File open mode
@retval "!= NULL" File descriptor
@retval "== NULL" Failed
@details
Performs pseudo file open for registered certificate.
This function is a function uniquely replaced in NetFront Browser NX because fopen() is used when OpenSSL reads the certificate.
@attention
- For in_path, only WKCOSSL_CERT_FILE or WKCOSSL_CRL_FILE is specified.
- Since this function is only used as read-only, the mode is ignored.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API FILE*  wkcOsslCertfOpenPeer(const char* in_path, const char *mode);
/**
@brief Pseudo file close function for registered certificate
@param "stream" File descriptor
@retval "== 0" Succeeded
@retval "EOF"  Failed
@details
Performs pseudo file close for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fclose() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int    wkcOsslCertfClosePeer(FILE *stream);
/**
@brief Pseudo file read function for registered certificate
@param "buffer" Pointer to read data storage
@param "size" Byte length of read data
@param "count" Number of read data
@param "stream" File descriptor
@retval "!= 0" Number of successful elements
@retval "== 0" Failed
@details
Performs pseudo file read for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fread() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API size_t wkcOsslCertfReadPeer(void *buffer, size_t size, size_t count, FILE *stream);
/**
@brief Pseudo file write function for registered certificate
@param "ptr" Pointer to write data storage
@param "size" Byte length of write data
@param "nmemb" Number of write data
@param "stream" File descriptor
@retval "!= 0" Number of successful elements
@retval "== 0" Failed
@details
Performs pseudo file write for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fwrite() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API size_t wkcOsslCertfWritePeer(const void *ptr, size_t size, size_t nmemb, FILE *stream);
/**
@brief Function for reading in units of one line from pseudo file for registered certificate
@param "s" Pointer to write data storage
@param "size" Byte length of write data
@param "stream" File descriptor
@retval "s" Succeeded
@retval "NULL" Failed
@details
Performs pseudo file read in units of one line for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fgets() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API char*  wkcOsslCertfGetsPeer(char *s, int size, FILE *stream);
/**
@brief Function that forcibly outputs (flushes) to pseudo file for registered certificate
@param "stream" File descriptor
@retval "!= 0" Failed
@retval "== 0" Succeeded
@details
Forcibly outputs (flushes) to pseudo file for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fflush() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API size_t wkcOsslCertfFlushPeer(FILE *stream);
/**
@brief Function that changes stream position to pseudo file for registered certificate
@param "stream" File descriptor
@param "offset" New position
@param "whence" Standard position
@retval "== 0" Succeeded
@retval " < 0" Failed
@details
Performs changing stream position to pseudo file for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fseek() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int    wkcOsslCertfSeekPeer(FILE *stream, long offset, int whence);
/**
@brief Function that indicates current stream position to pseudo file for registered certificate
@param "stream" File descriptor
@retval "0 <=" Current offset
@retval " < 0" Failed
@details
Returns the current stream position to pseudo file for file descriptor opened by wkcOsslCertfOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because ftell() is used when OpenSSL reads the certificate.
@attention
stream must be a value obtained by wkcOsslCertfOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API long   wkcOsslCertfTellPeer(FILE *stream);
/**
@brief Determines whether registered certificate exists
@retval "!= false"  Registered
@retval "== false" Not registered
@details
Determines whether trusted ROOT CA certificate is registered.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool   wkcOsslCertfIsRegistPeer(void);
/**
@brief Determines whether registered revoked certificate exists
@retval "!= false"  Registered
@retval "== false" Not registered
@details
Determines whether trusted revoked certificate is registered.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API bool   wkcOsslCRLIsRegistPeer(void);
/**
@brief Pseudo file open function for random file
@param "in_path" Random file path
@param "mode" File open mode
@retval "!= NULL" File descriptor
@retval "== NULL" Failed
@details
Performs pseudo file open for random file.
This function is a function uniquely replaced in NetFront Browser NX because fopen() is used when OpenSSL reads the random file.
@attention
- in_path specifies only WKCOSSL_RANDFILE.
- Since this function is only used as read-only, the mode is ignored.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API FILE*  wkcOsslRandFilefOpenPeer(const char* in_path, const char *mode);
/**
@brief Pseudo file close function for random file
@param "stream" File descriptor
@retval "== 0" Succeeded
@retval "EOF"  Failed
@details
Performs pseudo file close for file descriptor opened by wkcOsslRandFilefOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fclose() is used when OpenSSL reads the random file.
@attention
stream must be a value obtained by wkcOsslRandFilefOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int    wkcOsslRandFilefClosePeer(FILE *stream);
/**
@brief Function for getting pseudo file state for random file
@param "in_path" File path
@param "sb" stat
@retval "0" Succeeded
@retval "< 0"  Failed
@details
Gets pseudo file state for file descriptor opened by wkcOsslRandFilefOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fstat() is used when OpenSSL reads the random file.
@attention
stream must be a value obtained by wkcOsslRandFilefOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int    wkcOsslRandFileStatPeer(const char* in_path, struct stat *sb);
/**
@brief Pseudo file read function for random file
@param "ptr" Pointer to read data storage
@param "size" Byte length of read data
@param "nmemb" Number of read data
@param "stream" File descriptor
@retval "!= 0" Number of successful elements
@retval "== 0" Failed
@details
Performs pseudo file read for file descriptor opened by wkcOsslRandFilefOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fread() is used when OpenSSL reads the random file.
@attention
stream must be a value obtained by wkcOsslRandFilefOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API size_t wkcOsslRandFilefReadPeer(void *ptr, size_t size, size_t nmemb, FILE *stream);
/**
@brief Pseudo file write function for random file
@param "ptr" Pointer to read data storage
@param "size" Byte length of read data
@param "nmemb" Number of read data
@param "stream" File descriptor
@retval "!= 0" Number of successful elements
@retval "== 0" Failed
@details
Performs pseudo file write for file descriptor opened by wkcOsslRandFilefOpenPeer().
This function is a function uniquely replaced in NetFront Browser NX because fwrite() is used when OpenSSL reads the random file.
@attention
stream must be a value obtained by wkcOsslRandFilefOpenPeer().
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API size_t wkcOsslRandFilefWritePeer(const void *ptr, size_t size, size_t nmemb, FILE *stream);

// network utility peer
/**
@brief Checks IP address format
@param "in_ipaddress" String of which format is checked
@retval 0 Not IP address format
@retval 4 IPv4 address
@retval 6 IPv6 address
@details
Determines whether string specified by in_ipaddress is an IP address format.@n
Note that for IPv6, those with '[' and ']' parentheses must be considered.
*/
WKC_PEER_API int wkcNetCheckCorrectIPAddressPeer(const char *in_ipaddress);

// randomness peer
/**
@brief Gets 0 or larger integer/random number
@return 0 or larger integer/random number
@details
Returns 0 or larger integer/random number. The maximum value of random number returned by this function must be returned by wkcRandomMaxPeer().
*/
WKC_PEER_API unsigned int wkcRandomNumberPeer(void);
/**
@brief Gets random numbers array
@param "out_buf" Buffer to store random numbers
@param "in_buflen" Length of buffer
@return length of array
@details
Fills out_buf with 0-255 of random numbers.
*/
WKC_PEER_API int wkcRandomNumbersPeer(unsigned char* out_buf, int in_buflen);
/**
@brief Gets maximum value of random number returned by wkcRandomNumberPeer()
@return Maximum value of random number returned by wkcRandomNumberPeer()
@details
Returns the maximum value of random number returned by wkcRandomNumberPeer()
*/
WKC_PEER_API unsigned int wkcRandomMaxPeer(void);

// MIME-Type Detector
/**
@brief Guess MIME-Type
@param data Data
@param len Length of data
@return Guessed MIME-Type if available
*/
WKC_PEER_API const char* wkcGuessMIMETypeByContentPeer(const unsigned char* data, size_t len);

WKC_PEER_API int wkcSystemGetCurrentProcessIdPeer(void);

WKC_END_C_LINKAGE
/*@}*/

#endif /* _WKC_PEER_H_ */
