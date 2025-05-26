#include "Mfr.hpp"
#include <thread>

using namespace std;

int main() 
{
    Mfr mfr;
    
    mfr.startTcp();
    mfr.startUdp();
    mfr.startMfrStatusSender();

    while (true) 
    {
        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}