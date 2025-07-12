/*
 *  WKCMemoryEventHandler.h
 *
 *  Copyright (c) 2010-2022 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef WKCMemoryEventHandler_h
#define WKCMemoryEventHandler_h

#include <wkc/wkcbase.h>
#include <stddef.h>

// class definition

namespace WKC {

/*@{*/

/**
@brief Class that notifies of memory-related events
*/
class WKC_API WKCMemoryEventHandler
{
public:
    // checks whether requested size of memory is available.
    /**
       @brief Checks whether memory allocation is allowed
       @param request_size Requested size of memory to allocate
       @param in_forimage Requested type @n
       - != false Image memory allocation
       - == false Other than memory allocation
       @retval "!= false" Allocation allowed
       @retval "== false" Allocation not allowed
    */
    virtual bool checkMemoryAvailability(size_t request_size, bool in_forimage) = 0;

    // check whether requested size of something should be allocatable.
    /** @brief Reason for memory allocation */
    enum AllocationReason {
        /** @brief Image memory allocation */
        Image,
        /** @brief Animated image memory allocation */
        AnimatedImage,
        /** @brief Allocate for decoding of JavaScript text */
        JavaScript,
        /** @brief Allocation due to expansion of image buffer area */
        ImageSharedBuffer,
        /** @brief Allocation from layer creation */
        Layer,
        /** @brief Allocation due to expansion of JavaScript heap area */
        JavaScriptHeap,
        /** @brief Allocate from JIT assemblerBuffer */
        AssemblerBuffer,
#ifdef USE_WKC_CAIRO
        /** @brief Allocate from Cairo and Pixman. */
        Pixman,
#endif
        /** @brief Normal allocation */
        Normal
    };
    /**
       @brief Checks whether memory allocation is allowed
       @param request_size Requested size of memory to allocate
       @param reason Reason for memory allocation @n
       - WKC::WKCMemoryEventHandler::Image
       - WKC::WKCMemoryEventHandler::AnimatedImage
       - WKC::WKCMemoryEventHandler::JavaScript
       - WKC::WKCMemoryEventHandler::ImageSharedBuffer
       - WKC::WKCMemoryEventHandler::JavaScriptHeap
       - WKC::WKCMemoryEventHandler::Normal
       @retval "!= false" Allocation allowed
       @retval "== false" Allocation not allowed
       @details
       If false is specified for the return value, processing will continue without memory allocation.
       @attention
       If WKC::WKCMemoryEventHandler::AnimatedImage is specified for reason, the cache cannot be cleared.@n
       In the case of other values, the cache can be cleared.
    */
    virtual bool checkMemoryAllocatable(size_t request_size, AllocationReason reason) = 0;

    // notifies memory exhaust event.
    // arguments and return value are for future implementations.
    // at this point please do exit / finalize application.
    /**
       @brief Notifies of memory allocation failure
       @param request_size Requested memory size
       @param out_allocated_size Reference to allocated memory size
       @retval void* Pointer to allocated memory
    */
    virtual void* notifyMemoryExhaust(size_t request_size, size_t& out_allocated_size) = 0;

    // notifies memory exhaust event at allocation.
    /**
       @brief Notifies of memory allocation failure
       @param request_size Requested memory size
       @param reason Reason for memory allocation @n
       - WKC::WKCMemoryEventHandler::Image
       - WKC::WKCMemoryEventHandler::AnimatedImage
       - WKC::WKCMemoryEventHandler::JavaScript
       - WKC::WKCMemoryEventHandler::ImageSharedBuffer
       - WKC::WKCMemoryEventHandler::JavaScriptHeap
       - WKC::WKCMemoryEventHandler::Normal
       @return None
       @details
       Notification is given when memory allocation fails.
    */
    virtual void notifyMemoryAllocationError(size_t request_size, AllocationReason reason) = 0;
    
    // notifies some CRASH() event
    // applications should do exit at this calling.
    /**
       @brief Notifies of forced termination
       @param file (When assert fails) path (C string) to file where assertion failed
       @param line (When assert fails) line number where assertion failed
       @param function (When assert fails) function (ASCII-only C string) where assertion failed
       @param assertion (When assert fails) assertion (ASCII-only C string) that failed
       @return None
       @details
       Notification is given when the browser engine is forcibly terminated.@n
       In the case of forced termination for reasons other than assertion failure, file, line, function, and assertion must be set to 0.
       @attention
       The file path to the build environment file path is specified for file when there is an assertion failure.@n
       If the file path contains Japanese, etc., it will be in the platform-dependent character encoding, but if there is only ASCII in the file path, then file will be an ASCII-only string.
    */
    virtual void notifyCrash(const char* file, int line, const char* function, const char* assertion) = 0;

    // notifies stack overflow.
    // applications should do exit at this calling if need_restart is true.
    /**
       @brief Notifies of stack overflow
       @param need_restart Notification whether restart is required
       - true  Restart is required.
       - false Restart is not required.
       @param stack_size Stack size
       @param consumption Stack consumption
       @param margin Margin
       @param stack_top Top address of stack area
       @param stack_base Bottom address of stack area
       @param current_stack_top Current top stack address
       @param file Path (C string) of file with process where stack overflow occurred
       @param line Line number in process where stack overflow occurred
       @param function Function (ASCII-only or C string) with process where stack overflow occurred
       @return None
       @details
       Notification is given when stack overflow appears about to occur in the browser engine.@n
       The stack size set by WKC::WKCWebKitSetStackSize() is used as the standard.@n
       It is called when the value after subtracting stack consumption from stack size becomes smaller than the margin. The appropriate value for each process inside the browser engine is applied for the margin.
       @attention
       Cases when need_restart is notified as true are when the HTML content is extremely complicated (there is an extreme number of nested elements, etc.).
       In these cases, basically it is not possible to continue processing in the browser engine, similar to when there is insufficient memory.@n
       Cases when need_restart is notified as false are when the stack becomes insufficient while running JavaScript.
       In these cases, since the behavior itself can continue if you stop JavaScript, the engine will continue to operate as long as the restart process is not performed on the application side, however it is okay to restart if the application side decides to.@n
       The build environment file path is specified for file.@n
       If the file path contains Japanese, etc., it will be in the platform-dependent character encoding, but if there is only ASCII in the file path, then file will be an ASCII-only string.
    */
    virtual void notifyStackOverflow(bool need_restart, size_t stack_size, size_t consumption, size_t margin, void* stack_top, void* stack_base, void* current_stack_top, const char* file, int line, const char* function) = 0;

    /**
       @brief Allocates physical pages for the specified virtual adress region
       @param ptr Pointer to the virtual address region
       @param size Size of the virtual address region
       @retval "!= false" Succeeded in allocating physical pages
       @retval "== false" Failed to allocating physical pages
    */
    virtual bool allocateMemoryPages(void* ptr, size_t size) = 0;

    /**
       @brief Frees the physical pages for the specified virtual adress region
       @param ptr Pointer to the virtual address region
       @param size Size of the virtual address region
    */
    virtual void freeMemoryPages(void* ptr, size_t size) = 0;
};

} // namespace

#endif // WKCMemoryEventHandler_h
