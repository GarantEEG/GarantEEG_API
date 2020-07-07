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
//! Реализация фильтра Butterworth
template<int MaxOrder, int MaxChannels = 8>
class CButterworthFilter : public CBaseFilter
{
protected:
	//! Фильтр
    Dsp::SimpleFilter<Dsp::Butterworth::BandPass<MaxOrder>, MaxChannels> m_Filter;

	//! Рабочая частота
    int m_Rate = 500;

	//! Нижняя планка среза
    int m_LowFrequency = 1;

	//! Верхняя планка среза
    int m_HightFrequency = 20;

	//! Список каналов, для которых применяется фильтр
    int m_ChannelsList[MaxChannels];

public:
	/**
	 * @brief CButterworthFilter Конструктор
	 * @param channelsList Указатель на список каналов
	 */
    CButterworthFilter(const int *channelsList) { memcpy(&m_ChannelsList[0], &channelsList[0], sizeof(m_ChannelsList)); }

	/**
	 * @brief ~CButterworthFilter Деструктор
	 */
    virtual ~CButterworthFilter() {}

	/**
	 * @brief Setup Функция установки настроек фильтра
	 * @param rate Рабочая частота
	 * @param lowFrequency Нижняя планка среза
	 * @param hightFrequency Верхняя планка среза
	 */
    virtual void Setup(int rate, int lowFrequency, int hightFrequency) override
    {
        m_Rate = rate;
        m_LowFrequency = lowFrequency;
        m_HightFrequency = hightFrequency;

        double centerFrequency = ((m_LowFrequency + (m_HightFrequency - m_LowFrequency)) / 2.0);
        double widthFrequency = ((m_HightFrequency - m_LowFrequency) / 2.0);

        m_Filter.setup(MaxOrder, rate, centerFrequency, widthFrequency);
    }

	/**
	 * @brief Process Функция фильтрации данных
	 * @param count Количетво данных
	 * @param samples Указатель на список данных по каналам
	 */
    virtual void Process(int count, float **samples) override
    {
        m_Filter.process(count, samples);
    }

	/**
	 * @brief Type Получить тип фильтра
	 * @return Тип фильтра
	 */
    virtual int Type() const override { return FT_BUTTERWORTH; }

	/**
	 * @brief Order Получить порядок фильтра
	 * @return Порядок фильтра
	 */
    virtual int Order() const override { return MaxOrder; }

	/**
	 * @brief ChannelsCount Получить количество каналов, для которых работает фильтр
	 * @return Количество каналов
	 */
    virtual int ChannelsCount() const override { return MaxChannels; }

	/**
	 * @brief ChannelsList Получить указатель на список каналов, для которых работает фильтр
	 * @return Указатель на список каналов
	 */
    virtual const int *ChannelsList() const override { return &m_ChannelsList[0]; }

	/**
	 * @brief Rate Получить рабочую частоту фильтра
	 * @return Частота
	 */
    virtual int Rate() const override { return m_Rate; }

	/**
	 * @brief LowFrequency Получить нижнюю планку среза
	 * @return Частота
	 */
    virtual int LowFrequency() const override { return m_LowFrequency; }

	/**
	 * @brief HightFrequency Получить верхнюю планку среза
	 * @return Частота
	 */
    virtual int HightFrequency() const override { return m_HightFrequency; }

	/**
	 * @brief Create Функция создания фильтра
	 * @param order Порядок фильтра
	 * @param channelsList Список каналов
	 * @return Указатель на созданный фильтр или nullptr если фильтр не был создан
	 */
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
