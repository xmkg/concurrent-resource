/**
*	\file		main.cpp
*	\author		Mustafa Kemal GILOR <mustafagilor@gmail.com>
*	\date		05/01/2019 18:41 PM
*
*   \brief      
*   A simple program to illustrate concurrent_resource library's usage.
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




#include <array>        // std::array
#include <vector>       // std::vector
#include <string>       // std::string
#include <iostream>     // std::cout, std::endl
#include <map>          // std::map, std::pair
#include <memory>       // std::unique_ptr
#include <thread>       // std::thread
#include <atomic>       // std::atomic
#include <chrono>       // std::chrono

#include "concurrent_resource.hpp"

using namespace mkg;

struct user_defined_type{
    std::array<std::uint8_t, 128> buffer;
    float coefficient = {0.1f};
    std::map<std::uint32_t, std::string> lookup_table;
};

int main(void){
    // Supports arbitrary types, including standard library containers, user defined types
    // primitive types, pointer types..
    {
        concurrent_resource<std::vector<std::string>> concurrent_vector;
        {

          
            {
                // scopes are for limiting the accessor's lifetime
                auto write_accessor = concurrent_vector.write_access();
                // We can safely access to the underlying vector now
                write_accessor->emplace_back("you can treat the accessor's as a pointer to underlying resource.");
            }

            // Be careful, if you attempt acquiring two write accessors from same thread, it will cause deadlock.
         
            #if __cplusplus >= 201703L
            // can alternatively done neater, if C++17 is available
            if(auto wa = concurrent_vector.write_access(); !wa->empty()){
                wa->emplace_back("C++17 is awesome.");
            }
            #endif
           
        }
        // Write accessor is gone, held lock is released

        {
            // Grab a read-only accessor to the vector
            auto read_accessor = concurrent_vector.read_access();
            // We now hold a read lock to the object
            // Iterate over the vector safely
            for(const auto & str : (*read_accessor)){
                    std::cout <<str << std::endl;
            }
        }
        // Read accessor is gone, held lock is released
    }
    {
        concurrent_resource<std::map<std::string, std::uint64_t>> concurrent_map;
        {
            auto write_accessor = concurrent_map.write_access();
            write_accessor->insert(std::make_pair("First", 1));
            write_accessor->emplace("First", 1);
        }
    }

    {
        concurrent_resource<std::string> concurrent_string;
        {
            auto write_accessor = concurrent_string.write_access();
            (*write_accessor) = "this is awesome";
            std::cout << (*write_accessor) << std::endl;
        }
        {
            auto read_accessor = concurrent_string.read_access();
            // (*read_accessor) = "this is not possible";
            std::cout << (*read_accessor) << std::endl;
        }
    }

    {
        {
            // Declare a concurrent resource pointer
            concurrent_resource<std::unique_ptr<user_defined_type>> concurrent_resource;
            auto write_access = concurrent_resource.write_access();
                
            (*write_access) = std::make_unique<user_defined_type>();
            // or 
            write_access->reset(new user_defined_type());

            // Member access operator cannot be used here to access underlying resource,
            // because it is also a pointer-like type. For pointer-like types, we must stick
            // to dereferencing by * operator.
            (*write_access)->buffer.fill(std::uint8_t(0));

            auto iter = (*write_access)->lookup_table.find(1);
        }

        // or even, it might be vice-versa.

        {
            // This approach is syntatically simpler.
            std::unique_ptr<concurrent_resource<user_defined_type>> cr;
            cr = std::make_unique<concurrent_resource<user_defined_type>>();

            auto write_access = cr->write_access();
            write_access->coefficient = 0.1;
        }
    }


    {
        concurrent_resource<std::map<std::string,std::string>> shared_resource;
        std::vector<std::thread> producer_threads, consumer_threads;
        static std::atomic<std::uint64_t> idx = {0};
        {         
            // spawn producers
            producer_threads.emplace_back(std::thread(
                [&shared_resource](){                       
                    for(;;){
                        {
                            auto write_accessor = shared_resource.write_access();
                            write_accessor->emplace(std::to_string(idx++), "foo");
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(750));
                    }      
                }
            ));
        }

        {
            // spawn consumers
            consumer_threads.emplace_back(std::thread(
                [&shared_resource](){
                    
                    for(;;){
                        {
                            auto read_accessor = shared_resource.read_access();
                            for(const auto & pair : (*read_accessor)){
                                std::cout << pair.first << ":" << pair.second << std::endl;                      
                            }
                            std::flush(std::cout);
                        }
                        {
                            auto write_accessor = shared_resource.write_access();
                            if(!write_accessor->empty())
                                write_accessor->erase(write_accessor->begin());
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }
                }
            ));
        }


        for(auto & t :producer_threads){
            t.join();
        }
        for(auto & t : consumer_threads){
            t.join();
        }

    }


}

