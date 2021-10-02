concurrent-resource
============
A header-only C++ library that allows easily creating thread-safe, concurrency friendly resources.

brief
------------
The aim of this library is making it easier to create thread-safe objects without dealing with thread-synchronization primitives.
To achieve this, we use encapsulation to restrict access to resource, then grant access again in a controlled manner via `accessor` pattern.

accessors?
------------
Accessors are simple types, bundled with a reference to shared object, and a lock. The attributes of shared object and lock depends on
accessor's type (read,write). As they contain lock object internally, the lock is held during entire life of an accessor object's lifetime.

~~~cpp
#include "concurrent_stl.hpp" // or concurrent_boost.hpp
#include <vector>
#include <iostream>

// A very-basic example
int main(void){
  // Wrap a regular vector to make it thread-safe
  mkg::concurrent<std::vector<std::string>> books;
  {
    // Grab a write accessor (we can manipulate the vector through it)
    auto writer = books.write_access_handle();
    // Now, an exclusive lock is held upon the construction of write accessor
    // by std::unique_lock<std::shared_mutex>(...) (or boost::)
    // We can reach the underlying vector by just ->'ing the writer.
    writer->push_back("White Fang");
    writer->push_back("Harry Potter and the Philosopher's Stone");
    // Attention: Do not attempt to acquire another write accessor from same thread whilst
    // another one is already alive.
  }
  // Here, right after the accessor object is destroyed (by going out of scope)
  // the lock is released. 
  {
    // Grab a read accessor (read-only, cannot modify the vector through it)
    auto reader = books.read_access_handle();
    // Treat the accessors as if they're pointers to the underlying resource
    // (or std|boost::optional's if you will)
    for(const auto & book : (*reader)){
      std::cout << book << std::endl;
    }
  }
}
~~~


dependencies?
------------
C++20 is required as project now uses `concepts`.

how to compile?
------------
As this is a `header-only` library, you do not need to do anything special. Just include `concurrent_resource.hpp` and you'll be fine. To compile `main.cpp`, which contains the examples, you need `CMake` (if you're lazy), or just type
* C++20(std): g++ main.cpp -lpthread -o example --std=c++2a   
* BOOST : g++ main.cpp -lpthread -lboost_system -lboost_thread -o example --std=c++2a
in source folder. (or equivalent in Windows)

anything to worry about?
------------
Well, there is a single pitfall you should consider while using this library.

* Do not try to grab two accessors from one thread at the same time. The reason for this is, std:: and boost:: shared mutexes are `non-reentrant` (non-recursive)

pros
------------
* extremely easy to use
* thread synchronization primitives are totally abstract from user's perspective
* read-write rights are enforced through accessors, leading to safer code in many cases
* raii style lifetime management

cons
------------
* std::shared_mutex & boost::shared_mutex are not known with their blow-your-socks-off performance (but they'll do `just fine ™` ) :shipit:


what is the difference from the first release?
------------
- Concurrent type is now fully generic, you can now implement your custom Lockable and Lock types and use it with concurrent wrapper
- Concept validation (via C++20 concepts)
- Simplifications 

future plans
------------
* well, most of the future plans are now obsolete, because this new release is abstract from any lock implementation.

license
------------
The project is released under MIT license. 
