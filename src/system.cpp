

#include <iostream>
#include <cstdlib>
#include <csignal>
#if defined(__GLIBC__) && !defined(_WIN32) && !defined(__CYGWIN__)
#   include <execinfo.h> 
#endif

#include "system.h"

namespace util {
namespace system {


void
print_build_timestamp (char const* application_name,
        char const* date, char const* time)
{
    std::cout << application_name << " (built on "
        << date << ", " << time << ")" << std::endl;
}


void
register_segfault_handler (void)
{
    std::signal(SIGSEGV, util::system::signal_segfault_handler);
}


void
signal_segfault_handler (int code)
{
    if (code != SIGSEGV)
        return;
    std::cerr << "Received signal SIGSEGV (segmentation fault)" << std::endl;
    print_stack_trace();
}


void
print_stack_trace (void)
{
#if defined(__GLIBC__) && !defined(_WIN32) &&  !defined(__CYGWIN__)
    void *array[32];
    int const size = ::backtrace(array, 32);

    std::cerr << "Obtained " << size << " stack frames:";
    for (int i = 0; i < size; ++i)
        std::cerr << " " << array[i];
    std::cerr << std::endl;

    ::backtrace_symbols_fd(array, size, 2);
#endif
    std::cerr << "Segmentation fault" << std::endl;
    ::exit(1);
}

}
}