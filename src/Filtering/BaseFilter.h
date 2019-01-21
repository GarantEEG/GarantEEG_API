/**
@file BaseFilter.h

@brief Абстрактный класс для работы с фильтрами

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef BASEFILTER_H
#define BASEFILTER_H
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
class CBaseFilter
{
public:
    virtual ~CBaseFilter() {}

    virtual void Setup(int rate, int lowFrequency, int hightFrequency) = 0;

    virtual void Parse() = 0;

    virtual int Rate() const = 0;
    virtual int LowFrequency() const = 0;
    virtual int HightFrequency() const = 0;
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // BASEFILTER_H
//----------------------------------------------------------------------------------
