#pragma once

#ifndef assert
    #ifdef DEBUG
        #include <exception>
        #include <string>
        #include <cstring>

        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

        #ifndef __PRETTY_FUNCTION__
            #define __PRETTY_FUNCTION__ __func__
        #endif // __PRETTY_FUNCTION__
        #define assert(expr) \
        if (!(expr)) \
            throw std::runtime_error(std::string(# expr) + " evaluated to false in " + __FILENAME__ + " in " + __PRETTY_FUNCTION__ + " at line " + std::to_string(__LINE__))
            //throw std::runtime_error(string(# expr) + " evaluated to false in " + __FILE__ " in " __PRETTY_FUNCTION__ " at line " #line)
    #else
        #define assert(expr)
    #endif // DEBUG
#endif // assert
