/**
@file GarantEEG_API_CPP.h

@brief Интерфейс на языке C++ для управления устройством ЕЕГ

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef GARANT_EEG_API_CPP_H
#define GARANT_EEG_API_CPP_H
//----------------------------------------------------------------------------------
#include "GarantEEG_API_Types.h"
#include "../src/Filtering/AbstractFilter.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Интерфейс для работы с устройством
////////////////////////////////////////////////////////////////////////////////////
class IGarantEEG
{
public:
	/**
	 * @brief ~IGarantEEG Деструктор
	 */
    virtual ~IGarantEEG() {}

	/**
	 * @brief Dispose Функция удаления
	 */
    virtual void Dispose() = 0;



	/**
	 * @brief GetType Получение типа устройства
	 * @return Тип устройства
	 */
    virtual GARANT_EEG_DEVICE_TYPE GetType() const = 0;



	/**
	 * @brief IsConnecting Получение состояния подключения устройства
	 * @return true
	 */
    virtual bool IsConnecting() const = 0;

	/**
	 * @brief Start Функция старта работы с устройством
	 * @param waitForConnection Ожидать ли подключения или подключиться к оборудованию в асинхронном режиме
	 * @param rate Частота работы устройства (250/500/1000)
	 * @param host IP-адрес для подключения
	 * @param port Порт для подключения
	 * @return true если подключено (или подключение началось, для асинхронного режима)
	 */
    virtual bool Start(bool waitForConnection = true, int rate = 500, const char *host = "192.168.127.125", int port = 12345) = 0;

	/**
	 * @brief Stop Остановить работу с устройством
	 */
    virtual void Stop() = 0;

	/**
	 * @brief IsStarted Состояние работы с устройством
	 * @return true если подключены, false если нет подключения
	 */
    virtual bool IsStarted() const = 0;

	/**
	 * @brief IsPaused Состояние стримов данных
	 * @return true если стримы активны, false если передача данных приостановлена пользователем
	 */
    virtual bool IsPaused() const = 0;

	/**
	 * @brief StartRecord Начать запись данных в файл
	 * @param userName Имя респондента
	 * @param filePath Путь к BDF файлу, в который нужно записывать данные
	 * @return true если запись в файл началась успешно, false если запись не началась
	 */
    virtual bool StartRecord(const char *userName, const char *filePath = nullptr) = 0;

	/**
	 * @brief StopRecord Остановить запись данных в файл
	 */
    virtual void StopRecord() = 0;

	/**
	 * @brief IsRecording Получить состояние записи данных в файл
	 * @return true если запись данных в файл идет, false если нет
	 */
    virtual bool IsRecording() const = 0;

	/**
	 * @brief PauseRecord Приостановить запись данных в файл (без фактической остановки записи, с возможностью продолжения записи)
	 */
    virtual void PauseRecord() = 0;

	/**
	 * @brief ResumeRecord Возобновить запись данных в файл
	 */
    virtual void ResumeRecord() = 0;

	/**
	 * @brief IsRecordPaused Получить состояние паузы записи данных в файл
	 * @return true если запись данных в файл на паузе, false если нет
	 */
    virtual bool IsRecordPaused() const = 0;

	/**
	 * @brief SetAutoReconnection Установить состояние автопереподключения к устройству при разрыве соединения
	 * @param enable Новое состояние
	 */
    virtual void SetAutoReconnection(bool enable) = 0;

	/**
	 * @brief AutoReconnectionEnabled Получить состояние автопереподключения к устройству при разрыве соединения
	 * @return true если автопереподключение активно, false если нет
	 */
    virtual bool AutoReconnectionEnabled() const = 0;



	/**
	 * @brief StartDataTranslation Запустить стрим данных с устройства
	 */
    virtual void StartDataTranslation() = 0;

	/**
	 * @brief StopDataTranslation Остановить стрим данных с устройства (без фактического отключения от устройства)
	 */
    virtual void StopDataTranslation() = 0;



	/**
	 * @brief SetRxThreshold Установить пороговое значение для сопротивления
	 * @param value Значение
	 */
    virtual void SetRxThreshold(int value) = 0;

	/**
	 * @brief StartIndicationTest Запустить тест индикации светодиодов
	 */
    virtual void StartIndicationTest() = 0;

	/**
	 * @brief StopIndicationTest Остановить тест индикации светодиодов
	 */
    virtual void StopIndicationTest() = 0;



	/**
	 * @brief PowerOff Выключить устройство
	 */
    virtual void PowerOff() = 0;

	/**
	 * @brief SynchronizationWithNTP Послать команду синхронизации с NTP сервером (NTP сервер должен быть настроек на клиенте)
	 */
    virtual void SynchronizationWithNTP() = 0;

	/**
	 * @brief GetRate Получить текущую частоту дискретизации данных, с которой работает устройство
	 * @return Частота дискретизации данных
	 */
    virtual int GetRate() const = 0;

	/**
	 * @brief GetBatteryStatus Получить уровень заряда аккумулятора
	 * @return Значение в процентах
	 */
    virtual int GetBatteryStatus() const = 0;

	/**
	 * @brief GetFirmwareVersion Получить версию прошивки
	 * @return Версия прошивки
	 */
    virtual const char *GetFirmwareVersion() const = 0;



	/**
	 * @brief GetFiltersCount Получить количество фильтров
	 * @return Количество фильтров
	 */
    virtual int GetFiltersCount() const = 0;

	/**
	 * @brief GetFilters Получить список указателей на фильтры
	 * @return Список указателей на фильтры
	 */
    virtual const CAbstractFilter** GetFilters() = 0;

	/**
	 * @brief AddFilter Добавить фильтр
	 * @param type Тип фильтра
	 * @param order Порядок фильтра
	 * @return Указатель на созданный фильтр или nullptr в случае, если не удалось создать фильтр с указанными параметрами
	 */
    virtual const CAbstractFilter* AddFilter(int type, int order) = 0;

	/**
	 * @brief AddFilter Добавить фильтр с указанием каналов, для которых он должен применяться
	 * @param type Тип фильтра
	 * @param order Порядок фильтра
	 * @param channelsCount Количество каналов
	 * @param channelsList Список каналов
	 * @return Указатель на созданный фильтр или nullptr в случае, если не удалось создать фильтр с указанными параметрами
	 */
    virtual const CAbstractFilter* AddFilter(int type, int order, int channelsCount, const int *channelsList) = 0;

	/**
	 * @brief SetupFilter Установить настройки фильтра
	 * @param filter Указатель на фильтр для установки настроек
	 * @param rate Частота дискретизации данных
	 * @param lowFrequency Нижняя частота среза
	 * @param hightFrequency Верхняя частота среза
	 * @return true если настройки применены, false если нет
	 */
    virtual bool SetupFilter(const CAbstractFilter *filter, int rate, int lowFrequency, int hightFrequency) = 0;

	/**
	 * @brief RemoveFilter Удалить фильтр
	 * @param filter Указатель на фильтр для удаления
	 */
    virtual void RemoveFilter(const CAbstractFilter *filter) = 0;

	/**
	 * @brief RemoveAllFilters Удалить все фильтры
	 */
    virtual void RemoveAllFilters() = 0;



	/**
	 * @brief SetCallback_OnStartStateChanged Установить коллбэк для старта/остановки работы с устройством
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_OnStartStateChanged(void *userData, EEG_ON_START_STATE_CHANGED *callback) = 0;

	/**
	 * @brief SetCallback_OnRecordingStateChanged Установить коллбэк для старта/остановки записи данных в файл
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_OnRecordingStateChanged(void *userData, EEG_ON_RECORDING_STATE_CHANGED *callback) = 0;

	/**
	 * @brief SetCallback_ReceivedData Установить коллбэк для приема новой порции данных с устройства
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_ReceivedData(void *userData, EEG_ON_RECEIVED_DATA *callback) = 0;

};
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Экспортируемые функции
////////////////////////////////////////////////////////////////////////////////////
//! Функция создания устройства
extern "C" __declspec(dllexport) IGarantEEG* __cdecl CreateDevice(GARANT_EEG_DEVICE_TYPE type);
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // GARANT_EEG_API_CPP_H
//----------------------------------------------------------------------------------
