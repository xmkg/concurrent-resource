/**
 * ______________________________________________________
 * Concurrent wrapper, defaulted with Boost lock primitives.
 * 
 * @file 	concurrent_stl.hpp
 * @author 	Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date 	02.12.2020
 * 
 * SPDX-License-Identifier:	MIT
 * ______________________________________________________
 */

#pragma once

#include "concurrent.hpp"

#if defined __has_include
#  if __has_include (<boost/thread/shared_mutex.hpp>)
#    include <boost/thread/shared_mutex.hpp>
namespace mkg {
    template<typename NonConcurrentType, basic_shared_lockable LockableType = boost::shared_mutex, 
        template <typename...> typename SharedLockType = boost::shared_lock, 
        template <typename...> typename ExclusiveLockType = boost::unique_lock >
    class concurrent;
}
#  else
#  error "Boost implementation of concurrent wrapper requires boost/thread/shared_mutex.hpp to be available."
#  endif
#endif
