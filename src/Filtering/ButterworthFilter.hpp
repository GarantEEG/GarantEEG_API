/**
@file ButterworthFilter.cpp

@brief Класс для работы с фильтром Баттерворта

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef BUTTERWORTHFILTER_H
#define BUTTERWORTHFILTER_H
//----------------------------------------------------------------------------------
#include "BaseFilter.h"
#include "dspfilter/Butterworth.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
template<int MaxOrder, int MaxChannels = 8>
class CButterworthFilter : public CBaseFilter
{
protected:
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<MaxOrder>, MaxChannels> m_Filter;

    int m_Rate = 500;
    int m_LowFrequency = 1;
    int m_HightFrequency = 20;

public:
    CButterworthFilter() {}
    virtual ~CButterworthFilter() {}

    virtual void Setup(int rate, int lowFrequency, int hightFrequency) override
    {
        m_Rate = rate;
        m_LowFrequency = lowFrequency;
        m_HightFrequency = hightFrequency;

        double centerFrequency = ((m_LowFrequency +  (m_HightFrequency - m_LowFrequency)) / 2.0);
        double widthFrequency = ((m_HightFrequency - m_LowFrequency) / 2.0);

        m_Filter.setup(MaxOrder, rate, centerFrequency, widthFrequency);
    }

    virtual void Parse() override
    {
    }

    virtual int Rate() const override { return m_Rate; }
    virtual int LowFrequency() const override { return m_LowFrequency; }
    virtual int HightFrequency() const override { return m_HightFrequency; }
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // BUTTERWORTHFILTER_H
//----------------------------------------------------------------------------------
