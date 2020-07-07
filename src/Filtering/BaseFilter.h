/**
@file BaseFilter.h

@brief Базовый класс для работы с фильтрами

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef BASEFILTER_H
#define BASEFILTER_H
//----------------------------------------------------------------------------------
#include "AbstractFilter.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
//! Базовый класс для работы с фильтрами
class CBaseFilter : public CAbstractFilter
{
public:
	/**
	 * @brief ~CBaseFilter Деструктор
	 */
    virtual ~CBaseFilter() {}

	/**
	 * @brief Setup Функция установки настроек фильтра
	 * @param rate Рабочая частота
	 * @param lowFrequency Нижняя планка среза
	 * @param hightFrequency Верхняя планка среза
	 */
    virtual void Setup(int rate, int lowFrequency, int hightFrequency) = 0;

	/**
	 * @brief Process Функция фильтрации данных
	 * @param count Количетво данных
	 * @param samples Указатель на список данных по каналам
	 */
    virtual void Process(int count, float **samples) = 0;
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // BASEFILTER_H
//----------------------------------------------------------------------------------
