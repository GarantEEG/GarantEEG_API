/**
@file AbstractFilter.h

@brief Абстрактный класс для работы с фильтрами

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef ABSTRACTFILTER_H
#define ABSTRACTFILTER_H
//----------------------------------------------------------------------------------
#include "../GarantEEG_API/include/GarantEEG_API_Types.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
class CAbstractFilter
{
public:
    virtual ~CAbstractFilter() {}

    virtual int Type() const = 0;
    virtual int Order() const = 0;
    virtual int ChannelsCount() const = 0;
    virtual const int *ChannelsList() const = 0;

    virtual int Rate() const = 0;
    virtual int LowFrequency() const = 0;
    virtual int HightFrequency() const = 0;
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // ABSTRACTFILTER_H
//----------------------------------------------------------------------------------
