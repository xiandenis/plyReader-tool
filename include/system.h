
#ifndef UTIL_SYSTEM_HEADER
#define UTIL_SYSTEM_HEADER

#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <vector>
#include <istream>


namespace util {
namespace system {

void sleep (std::size_t msec);

void sleep_sec (float secs);

void rand_init (void);

void rand_seed (int seed);

float rand_float (void);

int rand_int (void);


void
print_build_timestamp (char const* application_name);

void
print_build_timestamp (char const* application_name,
    char const* date, char const* time);

void register_segfault_handler (void);

void signal_segfault_handler (int code);

void print_stack_trace (void);


template <int N>
inline void
byte_swap (char* data);

template <typename T>
inline T
letoh (T const& x);

template <typename T>
inline T
betoh (T const& x);

template <typename T>
inline T
read_binary_little_endian(std::istream* stream);


inline void
sleep (std::size_t msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

inline void
sleep_sec (float secs)
{
    sleep((std::size_t)(secs * 1000.0f));
}

inline void
rand_init (void)
{
    std::srand(std::time(nullptr));
}

inline void
rand_seed (int seed)
{
    std::srand(seed);
}

inline float
rand_float (void)
{
    return (float)std::rand() / (float)RAND_MAX;
}

inline int
rand_int (void)
{
    return std::rand();
}

inline void
print_build_timestamp (char const* application_name)
{
    print_build_timestamp(application_name, __DATE__, __TIME__);
}


#if defined(_WIN32) || defined(__CYGWIN__)
#   define HOST_BYTEORDER_LE
#endif

#if defined(__APPLE__)
#   if defined(__ppc__) || defined(__ppc64__)
#       define HOST_BYTEORDER_BE
#   elif defined(__i386__) || defined(__x86_64__)
#       define HOST_BYTEORDER_LE
#   endif
#endif

#if defined(__linux__)
#   include <endian.h> 
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#   include <machine/endian.h>
#   if BYTE_ORDER == LITTLE_ENDIAN
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

#if defined(__SunOS)
#   include <sys/byteorder.h>
#   if defined(_LITTLE_ENDIAN)
#       define HOST_BYTEORDER_LE
#   else
#       define HOST_BYTEORDER_BE
#   endif
#endif

template <>
inline void
byte_swap<2> (char* data)
{
    std::swap(data[0], data[1]);
}

template <>
inline void
byte_swap<4> (char* data)
{
    std::swap(data[0], data[3]);
    std::swap(data[1], data[2]);
}

template <>
inline void
byte_swap<8> (char* data)
{
    std::swap(data[0], data[7]);
    std::swap(data[1], data[6]);
    std::swap(data[2], data[5]);
    std::swap(data[3], data[4]);
}

#if defined(HOST_BYTEORDER_LE) && defined(HOST_BYTEORDER_BE)
#   error "Host endianess can not be both LE and BE!"
#elif defined(HOST_BYTEORDER_LE)

template <typename T>
inline T
letoh (T const& x)
{
    return x;
}

template <typename T>
inline T
betoh (T const& x)
{
    T copy(x);
    byte_swap<sizeof(T)>(reinterpret_cast<char*>(&copy));
    return copy;
}

#elif defined(HOST_BYTEORDER_BE)

template <typename T>
inline T
letoh (T const& x)
{
    T copy(x);
    byte_swap<sizeof(T)>(reinterpret_cast<char*>(&copy));
    return copy;
}

template <typename T>
inline T
betoh (T const& x)
{
    return x;
}

#else
#   error "Couldn't determine host endianess!"
#endif

template <typename T>
inline T
read_binary_little_endian(std::istream* stream)
{
    T data_little_endian;
    stream->read(reinterpret_cast<char*>(&data_little_endian), sizeof(T));
    return letoh(data_little_endian);
}

}
}

#endif /* UTIL_SYSTEM_HEADER */
