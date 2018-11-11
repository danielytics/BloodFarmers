#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
typedef float DeltaTime_t;
typedef std::chrono::duration<DeltaTime_t> DeltaTime;
typedef long long ElapsedTime_t;
typedef std::chrono::duration<ElapsedTime_t> ElapsedTime;

#endif // CLOCK_H
