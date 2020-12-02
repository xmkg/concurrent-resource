/**
 * ______________________________________________________
 * Concurrent wrapper, defaulted with STL lock primitives.
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

#include <shared_mutex>
#include <mutex>

namespace mkg {
    template<typename NonConcurrentType, basic_shared_lockable LockableType = std::shared_mutex, 
        template <typename...> typename SharedLockType = std::shared_lock, 
        template <typename...> typename ExclusiveLockType = std::unique_lock >
    class concurrent;
}