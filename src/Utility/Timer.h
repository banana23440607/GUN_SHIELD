#pragma once
#include "Common.h"
class Timer{
public:
    Timer(){LARGE_INTEGER f;QueryPerformanceFrequency(&f);m_freq=(double)f.QuadPart;QueryPerformanceCounter(&m_prev);}
    void Tick(){LARGE_INTEGER n;QueryPerformanceCounter(&n);m_delta=(float)((n.QuadPart-m_prev.QuadPart)/m_freq);m_prev=n;m_total+=m_delta;}
    float DeltaTime()const{return m_delta;}
    float TotalTime()const{return m_total;}
private:
    double m_freq=1.0;LARGE_INTEGER m_prev={};float m_delta=0,m_total=0;
};
