/**
 * ______________________________________________________
 * Header-only concurrency wrapper.
 * 
 * @file 	concurrent.hpp
 * @author 	Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date 	02.12.2020
 * 
 * SPDX-License-Identifier:	MIT
 * ______________________________________________________
 */

#pragma once

#include <memory>
#include <chrono>
#include <type_traits>

namespace mkg {

    /**
     * Checks whether given type T satisfies the requirements of
     * `BasicLockable` concept (a.k.a named requirement).
     * 
     * @see http://www.cplusplus.com/reference/concept/BasicLockable/
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept basic_lockable = requires(T a) {

        /**
         * @brief Instantiation of type T must have a public function named `lock` 
         * and it must have return type of `void`.
         */
        { a.lock()   } -> std::same_as<void>;

        /**
         * @brief Instantiation of type T must have a public function named `unlock` 
         * and it must have return type of `void`, and it must be noexcept.
         */
        { a.unlock() } -> std::same_as<void>;
    };
    
    /**
     * Checks whether given type T satisfies the requirements of
     * `Lockable` concept (a.k.a named requirement).
     * 
     * @see http://www.cplusplus.com/reference/concept/Lockable/
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept lockable = basic_lockable<T> && requires (T a){
        /**
         * @brief Instantiation of type T must have a public function named `try_lock` 
         * and it must have return type of `bool`.
         */
        { a.try_lock() } -> std::same_as<bool>;
    };


    /**
     * Checks whether given type T satisfies the requirements of
     * `TimedLockable` concept (a.k.a named requirement).
     * 
     * @see http://www.cplusplus.com/reference/concept/TimedLockable/
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept timed_lockable = lockable<T> && requires (T a) {
        /**
         * @brief Instantiation of type T must have a public function named `try_lock_for`
         * which accepts std::chrono::duration<> as its argumentand it must have return type 
         * of `bool`.
         */
        { a.try_lock_for(std::chrono::seconds{}) } -> std::same_as<bool>;
        /**
         * @brief Instantiation of type T must have a public function named `try_lock_until`
         * which accepts std::chrono::time_point<> as its argumentand it must have return type 
         * of `bool`.
         */
        { a.try_lock_until(std::chrono::time_point<std::chrono::steady_clock>{ std::chrono::seconds{} }) } -> std::same_as<bool>;
    };

    /**
     * Checks whether given type T satisfies the requirements of
     * `basic_shared_lockable` concept (a.k.a named requirement).
     * 
     * ** There are no official named requirements for this **.
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept basic_shared_lockable = basic_lockable<T> && requires (T a) {

        /**
         * @brief Instantiation of type T must have a public function named `lock_shared`
         * it must have return type of `void`.
         */
        { a.lock_shared() } -> std::same_as<void>;

        /**
         * @brief Instantiation of type T must have a public function named `unlock_shared` 
         * and it must have return type of `void`, and it must be noexcept.
         */
        { a.unlock_shared() } -> std::same_as<void>;

    };


    /**
     * Checks whether given type T satisfies the requirements of
     * `shared_lockable` concept (a.k.a named requirement).
     * 
     * ** There are no official named requirements for this **.
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept shared_lockable = basic_shared_lockable<T> && requires (T a) {

        /**
         * @brief Instantiation of type T must have a public function named `try_lock_shared` 
         * and it must have return type of `bool`.
         */
        { a.try_lock_shared() } -> std::same_as<bool>;

    };


    /**
     * Checks whether given type T satisfies the requirements of
     * `shared_timed_lockable` concept (a.k.a named requirement).
     * 
     * ** There are no official named requirements for this **.
     * 
     * @tparam T Type to check
     */
    template<typename T>
    concept shared_timed_lockable = shared_lockable<T> && requires (T a) {

        /**
         * @brief Instantiation of type T must have a public function named `try_lock_shared_for`
         * which accepts std::chrono::duration<> as its argumentand it must have return type 
         * of `bool`.
         */
        { a.try_lock_shared_for(std::chrono::seconds{}) } -> std::same_as<bool>;

        /**
         * @brief Instantiation of type T must have a public function named `try_lock_shared_until`
         * which accepts std::chrono::time_point<> as its argumentand it must have return type 
         * of `bool`.
         */
        { a.try_lock_shared_until(std::chrono::time_point<std::chrono::steady_clock>{ std::chrono::seconds{} }) } -> std::same_as<bool>;

    };

    /**
     * A base class to make derived class non-copyable.
     * 
     * Inherit from this class to in order to make derived class noncopyable.
     * By non-copyable, it means the following operations are explicitly removed from the derived;
     * 
     * Copy-construction from mutable lvalue reference
     * Copy-construction from immutable lvalue reference
     * Copy-assignment from mutable lvalue reference
     * Copy-assignment from immutable lvalue reference
     */
    class noncopyable{
        noncopyable(noncopyable&)                   = delete;
        noncopyable(const noncopyable&)             = delete;
        noncopyable& operator=(noncopyable&)        = delete;
        noncopyable& operator=(const noncopyable&)  = delete;
    public:
        noncopyable()                               = default;
        ~noncopyable()                              = default;
        noncopyable(noncopyable&&)                  = default;
        noncopyable& operator=(noncopyable&&)       = default;
    };

    /**
     * A RAII wrapper type which wraps a resource and its' associated lock. The wrapper locks given `Lockable` object 
     * via instantiating a `LockType` object upon construction. This `LockType` object and the resource will be held 
     * as a class member together. This will guarantee that acquired type of lock will not be released until the wrapper
     * object's destructor is called.
     * 
     * The wrapper has "*" and "->" operator overloads to provide access wrapped resource. If wrapped resource
     * type is a class type, "->" operator will allow client to access its` members directly. The (*) operator
     * will return cv-qualified reference to the wrapped object.
     * 
     * @tparam TypeQualifiedNonConcurrentType CV qualified type of the wrapped resource. This will determine the access level.
     * @tparam LockableType Type of the `Lockable` object
     * @tparam LockType Type of the `Lock` object to lock the `Lockable` type
     */
    template<typename TypeQualifiedNonConcurrentType, typename LockableType, template <typename...> typename LockType>
    requires (std::is_reference_v<TypeQualifiedNonConcurrentType>)
    class accessor : private noncopyable {
    public:

        /**
         * @brief Accessor object constructor
         * 
         * @param resource  Resource to grant desired access to
         * @param lockable  Lockable, which will be locked during the access
         */
        constexpr explicit accessor(LockableType& lockable, TypeQualifiedNonConcurrentType resource)
            requires (!std::is_const_v<LockableType>)
            : lock(lockable), locked_resource(resource)
        {}

        /**
         * @brief Accessor object constructor
         *
         * @param resource  Resource to grant desired access to
         * @param lock  Existing lock, which will be moved into the accessor
         */
        constexpr explicit accessor(LockType<LockableType> && lock, TypeQualifiedNonConcurrentType resource)
            requires (std::is_rvalue_reference_v<decltype(lock)>)
            : lock(std::forward<LockType<LockableType>>(lock)), locked_resource(resource)
        {}

        /**
         * @brief Class member access operator overload to behave as if
         * instance of `accessor` class is a `locked_resource` pointer.
         * 
         * @note This operator has a special condition.
         * 
         * Quoting from C++98 standard §13.5.6/1 "Class member access":
         * 
         * "An expression x->m is interpreted as (x.operator->())->m for a class 
         * object x of type T if T::operator-> exists and if the operator is selected 
         * at the best match function by the overload resolution mechanism (13.3)."
         * 
         * which basically means when we call this operator, we will access member functions
         * of `TypeQualifiedNonConcurrentType`.
         * 
         * @return Provides member access if `TypeQualifiedNonConcurrentType` is a class type. Otherwise, it has no use.
         */
        inline decltype(auto) operator->() noexcept {
            struct member_access_operator_decorator {
                constexpr explicit member_access_operator_decorator(TypeQualifiedNonConcurrentType value)
                    : value(value)
                {}
                constexpr decltype(auto) operator->() const noexcept { return &value; }
            private:
                TypeQualifiedNonConcurrentType value;
            };

            return member_access_operator_decorator { locked_resource };
        }

        /**
         * Dereference (star) operator overload
         * 
         * @return cv-qualified reference to the resource
         */
        inline decltype(auto) operator*() noexcept { return locked_resource; }
    private:
        LockType<LockableType> lock;
        TypeQualifiedNonConcurrentType locked_resource;
    };

    template<typename NonConcurrentType, basic_shared_lockable LockableType, template <typename...> typename LockType>
    using shared_accessor = accessor<std::add_lvalue_reference_t<std::add_const_t<NonConcurrentType>>, LockableType, LockType>;

    template<typename NonConcurrentType, basic_lockable LockableType, template <typename...> typename LockType>
    using exclusive_accessor = accessor<std::add_lvalue_reference_t<NonConcurrentType>, LockableType, LockType>;

    /**
     * A wrapper type to protect a `NonConcurrentType` via locking a `LockableType`
     * via;
     * 
     * - `SharedLockType` when read only access
     * - `ExclusiveLockType` when read & write access
     * 
     * is requested.
     * 
     * @tparam NonConcurrentType A non thread-safe type to wrap
     * @tparam LockableType A type which satisfies the `BasicSharedLockable` named requirement (eg. std::shared_mutex)
     * @tparam SharedLockType A RAII type which will be used to lock the `Lockable` when read access is requested (eg. std::shared_lock)
     * @tparam ExclusiveLockType A RAII type which will be used to lock the `Lockable` when write access is requested (eg. std::unique_lock)
     */
    template<typename NonConcurrentType, basic_shared_lockable LockableType, 
        template <typename...> typename SharedLockType, 
        template <typename...> typename ExclusiveLockType >
    class concurrent {
    public:
        using shared_accessor_t = shared_accessor<NonConcurrentType, LockableType, SharedLockType>;
        using exclusive_accessor_t = exclusive_accessor<NonConcurrentType, LockableType, ExclusiveLockType>;

        /**
         * @brief Default constructor
         * 
         * Instantiates the `NonConcurrentType` by invoking its' default constructor.
         * 
         * @exception noexcept if `NonConcurrentType` is `NoThrowDefaultConstructible`
         */
        concurrent() 
            noexcept(std::is_nothrow_default_constructible_v<NonConcurrentType>) 
            requires (std::is_default_constructible_v<NonConcurrentType>)
            : resource{}    
        {}

        /**
         * @brief Copy constructor (from wrapped type)
         * 
         * Instantiates the `NonConcurrentType` by invoking its' copy constructor.
         * 
         * @exception noexcept if `NonConcurrentType` is `NoThrowCopyConstructible`
         */
        explicit concurrent(const NonConcurrentType& value) 
            noexcept(std::is_nothrow_copy_constructible_v<NonConcurrentType>) 
            requires (std::is_copy_constructible_v<NonConcurrentType>)
            : resource(value)    
        {}

        /**
         * @brief Move constructor (from wrapped type)
         * 
         * Instantiates the `NonConcurrentType` by invoking its' move constructor.
         * 
         * @exception noexcept if `NonConcurrentType` is `NoThrowMoveConstructible`
         */
        explicit concurrent(NonConcurrentType&& value) 
            noexcept(std::is_nothrow_move_constructible_v<NonConcurrentType>) 
            requires (std::is_move_constructible_v<NonConcurrentType>)
            : resource(std::move(value))    
        {}

        /**
         * @brief Destructor
         * 
         * @exception noexcept if `NonConcurrentType` is `NoThrowDestructible`
         */
        ~concurrent() noexcept(std::is_nothrow_destructible_v<NonConcurrentType>) = default;


        /**
         * @brief Get read-only (shared) access to underlying wrapped object. Access object will guarantee
         * `Lockable` object will be locked via `SharedLockType` from beginning to end of the 
         * returned accessor object.
         * 
         * @return shared_accessor_t Read-only (shared) accessor object to the wrapped object 
         */
        decltype(auto) read_access_handle() const noexcept { return shared_accessor_t{ lockable, resource }; }

        /**
         * @brief Get write (exclusive) access to underlying wrapped object. Access object will guarantee
         * `Lockable` object will be locked via `ExclusiveLockType` from beginning to end of the 
         * returned accessor object.
         * 
         * @exception Exception-safe
         * 
         * @return exclusive_accessor_t Read & write (exclusive) accessor object to the wrapped object 
         */
        decltype(auto) write_access_handle() noexcept { return exclusive_accessor_t{ lockable, resource }; }
        
    private:
        NonConcurrentType resource;
        mutable LockableType lockable; 
    };


}
