#include "timeTrans.h"
#include <chrono>

TimeStamp getCurrentTimeMillis()
{
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return static_cast<TimeStamp>(duration.count());
}