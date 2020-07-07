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
//! Абстрактный класс для работы с фильтрами
class CAbstractFilter
{
public:
	/**
	 * @brief ~CAbstractFilter Деструктор
	 */
    virtual ~CAbstractFilter() {}

	/**
	 * @brief Type Получить тип фильтра
	 * @return Тип фильтра
	 */
    virtual int Type() const = 0;

	/**
	 * @brief Order Получить порядок фильтра
	 * @return Порядок фильтра
	 */
    virtual int Order() const = 0;

	/**
	 * @brief ChannelsCount Получить количество каналов, для которых работает фильтр
	 * @return Количество каналов
	 */
    virtual int ChannelsCount() const = 0;

	/**
	 * @brief ChannelsList Получить указатель на список каналов, для которых работает фильтр
	 * @return Указатель на список каналов
	 */
    virtual const int *ChannelsList() const = 0;

	/**
	 * @brief Rate Получить рабочую частоту фильтра
	 * @return Частота
	 */
    virtual int Rate() const = 0;

	/**
	 * @brief LowFrequency Получить нижнюю планку среза
	 * @return Частота
	 */
    virtual int LowFrequency() const = 0;

	/**
	 * @brief HightFrequency Получить верхнюю планку среза
	 * @return Частота
	 */
    virtual int HightFrequency() const = 0;
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // ABSTRACTFILTER_H
//----------------------------------------------------------------------------------
