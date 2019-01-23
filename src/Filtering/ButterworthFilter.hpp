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

    int m_ChannelsList[MaxChannels];

public:
    CButterworthFilter(const int *channelsList) { memcpy(&m_ChannelsList[0], &channelsList[0], sizeof(m_ChannelsList)); }
    virtual ~CButterworthFilter() {}

    virtual void Setup(int rate, int lowFrequency, int hightFrequency) override
    {
        m_Rate = rate;
        m_LowFrequency = lowFrequency;
        m_HightFrequency = hightFrequency;

        double centerFrequency = ((m_LowFrequency + (m_HightFrequency - m_LowFrequency)) / 2.0);
        double widthFrequency = ((m_HightFrequency - m_LowFrequency) / 2.0);

        m_Filter.setup(MaxOrder, rate, centerFrequency, widthFrequency);
    }

    virtual void Process(int count, float **samples) override
    {
        m_Filter.process(count, samples);
    }

    virtual int Type() const override { return FT_BUTTERWORTH; }
    virtual int Order() const override { return MaxOrder; }
    virtual int ChannelsCount() const override { return MaxChannels; }
    virtual const int *ChannelsList() const override { return &m_ChannelsList[0]; }

    virtual int Rate() const override { return m_Rate; }
    virtual int LowFrequency() const override { return m_LowFrequency; }
    virtual int HightFrequency() const override { return m_HightFrequency; }

    static CBaseFilter *Create(int order, const int *channelsList)
    {
        //for (int i = 1; i <= 8; i++) qDebug("case %i: return new CButterworthFilter<%i, MaxChannels>(channelsList);", i, i);

        switch (order)
        {
            case 1: return new CButterworthFilter<1, MaxChannels>(channelsList);
            case 2: return new CButterworthFilter<2, MaxChannels>(channelsList);
            case 3: return new CButterworthFilter<3, MaxChannels>(channelsList);
            case 4: return new CButterworthFilter<4, MaxChannels>(channelsList);
            case 5: return new CButterworthFilter<5, MaxChannels>(channelsList);
            case 6: return new CButterworthFilter<6, MaxChannels>(channelsList);
            case 7: return new CButterworthFilter<7, MaxChannels>(channelsList);
            case 8: return new CButterworthFilter<8, MaxChannels>(channelsList);
            default:
                break;
        }

        return nullptr;
    }
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // BUTTERWORTHFILTER_H
//----------------------------------------------------------------------------------
