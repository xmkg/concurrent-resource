/**
*	\file		concurrent_resource.hpp
*	\author		Mustafa Kemal GILOR <mustafagilor@gmail.com>
*	\date		05/01/2019 18:41 PM
*
*   \brief      
*   A header-only library that provides thread-safety for arbitrary kind of resources by wrapping them with 
*   thread synchronization primitives transparently. The API exposes small objects known as `accessors`,
*   enabling user to access resources concurrently in RAII-style lifetime management. As long as accessor
*   object is alive, the user can access the resource concurrently, without worrying about the thread 
*   synchronization. All thread synchronization related operation is done in background, away from user's 
*   perspective.
*
*   The concept is robust, makes it easier to grant thread safety to any kind of resource without adding 
*   too much overhead.
*
*   The library either requires C++17, or BOOST to function. 
*   
*   \copyright
*   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
*   to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
*   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
*   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
*   \disclaimer
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
*   IN THE SOFTWARE.
*/



#if !defined(MKG_CONCURRENT_RESOURCE_USE_BOOST_SHARED_MUTEX) && (__cplusplus >= 201703L)
#include <shared_mutex> // prerequisite : C++17
namespace mkg{
    using shared_mutex_t = std::shared_mutex;
    using read_lock      = std::shared_lock<shared_mutex_t>;
    using write_lock     = std::unique_lock<shared_mutex_t>;
}
#define MG_LOCKABLE_USING_STD
#else
#include <boost/thread/shared_mutex.hpp>
namespace mkg{
    using shared_mutex_t = boost::shared_mutex;
    using read_lock      = boost::shared_lock<shared_mutex_t>;
    using write_lock     = boost::unique_lock<shared_mutex_t>;
    // boost's rwlock implementation has two additional lock kinds, which can be useful
    // in several scenarios.
    using upgrade_lock			= boost::upgrade_lock<shared_mutex_t>;
    using upgrade_to_write_lock = boost::upgrade_to_unique_lock<shared_mutex_t>;
}
#define MG_LOCKABLE_USING_BOOST
#endif

namespace mkg{

    /**
     * @brief Library implementation details.
     * 
     * @warning The code/class/api in this namespace might change drastically over the time.
     *          (not reliable) 
     */
    namespace detail{

        /**
         * @brief This is a simple workaround for the member access operator overload behaviour.
         * By default, member access operator requires return value to be `dereferenceable`, meaning
         * that, the return type must be a pointer to an object. As we intend to return 
         * references from our accessors, this behaviour prevents us to do so.
         * 
         * As a simple solution, we wrap our real type in our decorator class, exposing type pointer to
         * only wrapper (in our scenario, wrapper would be accessor class). The wrapper itself returns 
         * this decorator by value, resulting the desired behaviour.
         * 
         * @tparam TypeToDecorate  Type to add member access operator to.
         */
        template<typename TypeToDecorate>
        struct member_access_operator_decorator{
            constexpr member_access_operator_decorator(TypeToDecorate value)
               : value(value) 
                {}
            constexpr const decltype(auto) operator->() const noexcept { return &value; }
        private:
            TypeToDecorate value;
        };


         /**
         * @brief Wraps a resource with lock(read,write,upgrade), leaving only desired
         * access level to the resource. The lock is acquired on construction, released on
         * destruction of `accessor' type.
         * 
         * To access the resource, just treat `accessor` object as a pointer to
         * the resource. The class has similar semantics with std|boost::optional.
         * @tparam LockType     Lock type indicating the desired access (read|write|upgrade)
         * @tparam ResourceType Type of the resource to wrap with desired access
         */
        template<typename LockType, typename ResourceType>
        struct accessor{
            using base_t = accessor<LockType, ResourceType>;
                        
            /**
             * @brief Construct a new  accessor object
             * 
             * @param resource  Resource to grant desired access to
             * @param mtx       Resource's read-write lock
             */
            constexpr accessor(ResourceType resource, shared_mutex_t & mtx) 
                : lock(mtx), resource(resource)
            {}

            constexpr member_access_operator_decorator<ResourceType> operator->() noexcept 
            {
                return member_access_operator_decorator<ResourceType>(resource); 
            }

            decltype(auto) operator*() noexcept {return resource; }

        private:
            LockType lock;
            ResourceType resource;
        };

        /**
         * @brief Non-thread safe accessor to the resource.
         * 
         * @tparam ResourceType Resource to provide non-thread safe access.
         */
        template<typename ResourceType>
        struct unsafe_accessor {
            constexpr explicit unsafe_accessor(ResourceType resource) 
                : resource(resource)
            {}
            constexpr member_access_operator_decorator<ResourceType> operator->() noexcept 
            {
                return member_access_operator_decorator<ResourceType>(resource); 
            }
        private:
            ResourceType resource;
        };

        template<typename ResourceType>
        using read_accessor_base =  accessor<read_lock, const ResourceType&> ;

        template<typename ResourceType>
        using write_accessor_base = accessor<write_lock, ResourceType&>;

       
        template<typename ResourceType>
        struct read_accessor  : public read_accessor_base<ResourceType>
        { 
            using read_accessor_base<ResourceType>::read_accessor_base;
        };

        template<typename ResourceType>
        struct write_accessor  : public write_accessor_base<ResourceType> 
        {
            using write_accessor_base<ResourceType>::write_accessor_base;
        };


        template<typename ResourceType>
        struct unsafe_read_accessor  : public unsafe_accessor<const ResourceType&>  
        {};

        template<typename ResourceType>
        struct unsafe_write_accessor  : public unsafe_accessor<ResourceType&>  
        {};

    }

    /**
     * @brief A concept, which adds a read-write lock
     * to inheriting class.
     */
    template<typename SharedMutexType = shared_mutex_t>
    struct lockable{
    protected:
        mutable SharedMutexType rwlock;
    };


    /**
     * @brief This class wraps a resource, and exposes accessors only as an interface.
     * The only way to access the underlying resource is, through the accessors. The accessors
     * which are not prefixed with `unsafe_` are all thread-safe and all concurrent operations
     * related to them can be executed safely.
     * 
     * @tparam ResourceType Resource type to wrap
     */
    template<typename ResourceType>
    struct concurrent_resource : lockable<>{
    public:
        /**
         * @brief Grab a read-only, thread-safe accessor to the resource.
         * Concurrent readers using read accessor may access the protected resource. Write
         * accessors can not access to the resource whilst a read accessor is performing.
         * (the opposite is valid as well)
         * 
         * @return detail::read_accessor<ResourceType> Read-only accessor to the resource. 
         */
        detail::read_accessor<ResourceType>     read_access()   
        const noexcept { return {resource, rwlock};  }

        /**
         * @brief Grab an exclusive (read-write), thread safe accessor to the resource. All subsequent
         * read-write access requests will wait until the grabbed write access object is released.
         * 
         * @return detail::write_accessor<ResourceType> Exclusive accessor to the resource.
         */
        detail::write_accessor<ResourceType>    write_access()      
        noexcept { return {resource, rwlock};  }

        /**
         * @brief Grab a read-only, NON-THREAD SAFE accessor to the resource.
         * Note that this does not acquire any thread synchronization primitives.
         * 
         * @return detail::read_accessor<ResourceType>  Unsafe, read-only accessor to the resource.
         */
        detail::read_accessor<ResourceType>     unsafe_read_access()   
        const noexcept { return {resource};  }
        /**
         * @brief Grab a read & write, NON-THREAD SAFE accessor to the resource.
         * Note that this does not acquire any thread synchronization primitives.
         * @return detail::write_accessor<ResourceType> Unsafe, read & write accessor to the resource.
         */
        detail::write_accessor<ResourceType>    unsafe_write_access()  
        noexcept { return {resource};  }
    private:
        /**
         * @brief The underlying resource.
         * The con
         * 
         */
        ResourceType resource;
    };
}