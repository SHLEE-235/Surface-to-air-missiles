#include "Mfr.hpp"
#include <thread>

using namespace std;

int main() 
{
    Mfr mfr;    
    
    while (true) 
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    return 0;
}