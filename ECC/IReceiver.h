#pragma once

// �������̽�
class IReceiver
{
public:
    virtual void receive(int len, const char* packet) = 0;
 
};
