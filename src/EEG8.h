/**
@file EEG8.h

@brief Класс для управления устройством EEG8

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#ifndef EEG8_H
#define EEG8_H
//----------------------------------------------------------------------------------
#include <string>
using std::string;

#include <vector>
using std::vector;

#include <thread>
using std::thread;
//----------------------------------------------------------------------------------
#include "include/GarantEEG_API_CPP.h"
//----------------------------------------------------------------------------------
#include <winsock.h>
#include <mutex>
//----------------------------------------------------------------------------------
#include "Filtering/BaseFilter.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
//! Состояние подключения к устройству
enum CONNECTION_STAGE
{
	//! Нет ошибок
    CS_NONE = 0,
	//! Идет подключение
    CS_CONNECTING,
	//! Подключено
    CS_CONNECTED,
	//! Ошибка создания сокета
    CS_SOCKET_CREATION_ERROR,
	//! Ошибка определения хоста
    CS_HOST_DETECTION_ERROR,
	//! Ошибка подключения
    CS_CONNECTION_ERROR,
	//! Попытка переподключения
    CS_START_RECONNECTING
};
//----------------------------------------------------------------------------------
//! Типы валидации пакетов с устройств
enum GARANT_EEG_PACKET_VALIDATE_TYPE
{
	//! Валиден
    PVT_VALIDATED = 0,
	//! Некорректный идентификатор
    PVT_BAD_ID,
	//! Некорректная длина
    PVT_BAD_LENGTH,
	//! Некорректный тип
    PVT_BAD_TYPE,
	//! Некорректная контрольная суммы
    PVT_BAD_CRC32,
	//! Некорректный счетчик
    PVT_BAD_COUNTER
};
//----------------------------------------------------------------------------------
//! Типы данных  в пакетах
enum GARANT_EEG_PACKET_DATA_TYPE
{
	//! Заголовок
    PDT_HEADER = 1,
	//! Данные
    PDT_DATA
};
//----------------------------------------------------------------------------------
//! Класс для работы с устройством GarantEEG
class CEeg8 : public IGarantEEG
{
protected:
	//! Уровень заряда аккумулятора
    int m_BatteryStatus = 0;

	//! Версия прошивки
    string m_FirmwareVersion = "";

	//! Текущая частота дискретизации данных устройства
    int m_Rate = 500;

	//! Хост для подключения
    string m_Host = "192.168.127.125";

	//! Порт для подключения
    int m_Port = 12345;

	//! Имя записываемого файла
    string m_RecordFileName = "";

	//! Состояние подключения
    CONNECTION_STAGE m_ConnectionStage = CS_NONE;

	//! Состояние работы устройства
    bool m_Started = false;

	//! Состояние стрима данных с устройства
    bool m_TranslationPaused = false;

	//! Состояние записи данных в файл
    bool m_Recording = false;

	//! Состояние паузы записи данных в файл
    bool m_RecordPaused = false;

	//! Рабочий тред подключения
    thread m_Thread;

	//! Сообщение синхронизации времени
    string m_NTPMessage = "";

	//! Сокет
    SOCKET m_Socket;

	//! Мьютекс для доступа
    std::mutex m_Mutex;

	//! Файл записи данных
    FILE *m_File = nullptr;

	//! Текущий размер буфера записи
    int m_CurrentWriteBufferDataSize = 0;

	//! Количество записанных фрэймов
    int m_WrittenDataCount = 0;

	//! Размер для защищенного режима
    const int PROTECTED_MODE_EXTRA_SIZE = 12;

	//! Размер сообщения синхронизации времени
    const int NTP_MESSAGE_SIZE = 40;

	//! Размер буфера записи данных в файл
    int WRITE_FILE_BUFFER_SIZE = 0;

	//! Размер заголовка
    int m_HeaderSize = 0;

	//! Размер данных
    int m_DataSize = 0;

	//! Флаг игнорирования каунтера
    bool m_IgnoreCounter = false;

	//! Предыдущее значение каунтера
    int m_PrevCounter = 0;

	//! Флаг автореконнекта
    bool m_EnableAutoreconnection = true;

	//! Список названий каналов
    vector<string> m_ChannelNames;

	//! Загололвок BDF
    vector<char> m_HeaderData;

	//! Предыдущий фрэйм данных
    vector<char> m_PrevData;

	//! Буфер приема
    vector<char> m_RecvBuffer;

	//! Буфер записи данных в файл
    char *m_FileWriteBuffer = nullptr;

	//! Буфер отправки
    string m_SendBuffer;

	//! Применяемые фильтры
    std::vector<CBaseFilter*> m_Filters;

	//! Данные пользователя для коллбэка состояния подключения к устройству
    void *m_CallbackUserData_OnStartStateChanged = nullptr;

	//! Коллбэк состояния подключения к устройству
    EEG_ON_START_STATE_CHANGED *m_Callback_OnStartStateChanged = nullptr;

	//! Данные пользователя для коллбэка состояния записи данных в файл
    void *m_CallbackUserData_OnRecordingStateChanged = nullptr;

	//! Коллбэк состояния записи данных в файл
    EEG_ON_RECORDING_STATE_CHANGED *m_Callback_OnRecordingStateChanged = nullptr;

	//! Данные пользователя для коллбэка приема нового фрэйма данных
    void *m_CallbackUserData_OnReceivedData = nullptr;

	//! Коллбэк приема нового фрэйма данных
    EEG_ON_RECEIVED_DATA *m_Callback_OnReceivedData = nullptr;



    /**
     * @brief ValidatePacket Функция проверки чексуммы
     * @param buf Массив данных
     * @param size Размер
     * @return GARANT_EEG_PACKET_VALIDATE_TYPE состояние обработки
     */
    GARANT_EEG_PACKET_VALIDATE_TYPE ValidatePacket(unsigned char *buf, int size, const int &wantType);

    /**
	 * @brief UnpackUInt32LE Функция преобразования буфера в 32-битное целое (Little Endian)
	 * @param buf Буфер
     * @return
     */
    inline unsigned int UnpackUInt32LE(unsigned char *buf)
    {
        return (unsigned int)((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    }

	/**
	 * @brief UnpackUInt16LE Функция преобразования буфера в 16-битное целое (Little Endian)
	 * @param buf Буфер
	 * @return
	 */
    inline unsigned int UnpackUInt16LE(unsigned char *buf)
    {
        return (unsigned int)((buf[1] << 8) | buf[0]);
    }

	/**
	 * @brief UnpackUInt16BE Функция преобразования буфера в 16-битное целое (Big Endian)
	 * @param buf Буфер
	 * @return
	 */
    inline unsigned int UnpackUInt16BE(unsigned char *buf)
    {
        return (unsigned int)((buf[0] << 8) | buf[1]);
    }

	/**
	 * @brief DataReceived Функция обработки принятых данных
	 */
    void DataReceived();

    /**
     * @brief SendPacket Функция отправки пакета устройству
     * @param text Данные пакета
     */
    void SendPacket(const string &text);

    /**
     * @brief ProcessData Функция обработки набора данных
     * @param buf Указатель на массив данных
     * @param size Размер данных
     */
    void ProcessData(unsigned char *buf, const int &size);

public:
	/**
	 * @brief CEeg8 Конструктор
	 */
    CEeg8();

	/**
	 * @brief ~CEeg8 Деструктор
	 */
    virtual ~CEeg8();

	/**
	 * @brief SocketThreadFunction Функция асинхронного подключения
	 * @param depth Глубина вызова
	 */
    void SocketThreadFunction(int depth);



	/**
	 * @brief Dispose Функция удаления
	 */
    virtual void Dispose() override { delete this; }



	/**
	 * @brief GetType Получение типа устройства
	 * @return Тип устройства
	 */
    virtual GARANT_EEG_DEVICE_TYPE GetType() const override { return DT_GARANT; }



	/**
	 * @brief IsConnecting Получение состояния подключения устройства
	 * @return true
	 */
    virtual bool IsConnecting() const { return (m_ConnectionStage == CS_CONNECTING); }

	/**
	 * @brief Start Функция старта работы с устройством
	 * @param waitForConnection Ожидать ли подключения или подключиться к оборудованию в асинхронном режиме
	 * @param rate Частота работы устройства (250/500/1000)
	 * @param host IP-адрес для подключения
	 * @param port Порт для подключения
	 * @return true если подключено (или подключение началось, для асинхронного режима)
	 */
    virtual bool Start(bool waitForConnection = true, int rate = 500, const char *host = "192.168.127.125", int port = 12345) override;

	/**
	 * @brief Stop Остановить работу с устройством
	 */
    virtual void Stop() override;

	/**
	 * @brief IsStarted Состояние работы с устройством
	 * @return true если подключены, false если нет подключения
	 */
    virtual bool IsStarted() const override { return m_Started; }

	/**
	 * @brief IsPaused Состояние стримов данных
	 * @return true если стримы активны, false если передача данных приостановлена пользователем
	 */
    virtual bool IsPaused() const override { return m_TranslationPaused; }

	/**
	 * @brief StartRecord Начать запись данных в файл
	 * @param userName Имя респондента
	 * @param filePath Путь к BDF файлу, в который нужно записывать данные
	 * @return true если запись в файл началась успешно, false если запись не началась
	 */
    virtual bool StartRecord(const char *userName, const char *filePath = nullptr) override;

	/**
	 * @brief StopRecord Остановить запись данных в файл
	 */
    virtual void StopRecord() override;

	/**
	 * @brief IsRecording Получить состояние записи данных в файл
	 * @return true если запись данных в файл идет, false если нет
	 */
    virtual bool IsRecording() const override { return m_Recording; }

	/**
	 * @brief PauseRecord Приостановить запись данных в файл (без фактической остановки записи, с возможностью продолжения записи)
	 */
    virtual void PauseRecord() override;

	/**
	 * @brief ResumeRecord Возобновить запись данных в файл
	 */
    virtual void ResumeRecord() override;

	/**
	 * @brief IsRecordPaused Получить состояние паузы записи данных в файл
	 * @return true если запись данных в файл на паузе, false если нет
	 */
    virtual bool IsRecordPaused() const override { return m_RecordPaused; }

	/**
	 * @brief SetAutoReconnection Установить состояние автопереподключения к устройству при разрыве соединения
	 * @param enable Новое состояние
	 */
    virtual void SetAutoReconnection(bool enable) override { m_EnableAutoreconnection = enable; }

	/**
	 * @brief AutoReconnectionEnabled Получить состояние автопереподключения к устройству при разрыве соединения
	 * @return true если автопереподключение активно, false если нет
	 */
    virtual bool AutoReconnectionEnabled() const override { return m_EnableAutoreconnection; }



	/**
	 * @brief StartDataTranslation Запустить стрим данных с устройства
	 */
    virtual void StartDataTranslation() override;

	/**
	 * @brief StopDataTranslation Остановить стрим данных с устройства (без фактического отключения от устройства)
	 */
    virtual void StopDataTranslation() override;



	/**
	 * @brief SetRxThreshold Установить пороговое значение для сопротивления
	 * @param value Значение
	 */
    virtual void SetRxThreshold(int value) override;

	/**
	 * @brief StartIndicationTest Запустить тест индикации светодиодов
	 */
    virtual void StartIndicationTest() override;

	/**
	 * @brief StopIndicationTest Остановить тест индикации светодиодов
	 */
    virtual void StopIndicationTest() override;



	/**
	 * @brief PowerOff Выключить устройство
	 */
    virtual void PowerOff() override;

	/**
	 * @brief SynchronizationWithNTP Послать команду синхронизации с NTP сервером (NTP сервер должен быть настроек на клиенте)
	 */
    virtual void SynchronizationWithNTP() override;

	/**
	 * @brief GetRate Получить текущую частоту дискретизации данных, с которой работает устройство
	 * @return Частота дискретизации данных
	 */
    virtual int GetRate() const override { return m_Rate; }

	/**
	 * @brief GetBatteryStatus Получить уровень заряда аккумулятора
	 * @return Значение в процентах
	 */
    virtual int GetBatteryStatus() const override { return m_BatteryStatus; }

	/**
	 * @brief GetFirmwareVersion Получить версию прошивки
	 * @return Версия прошивки
	 */
    virtual const char *GetFirmwareVersion() const override { return m_FirmwareVersion.c_str(); }



	/**
	 * @brief GetFiltersCount Получить количество фильтров
	 * @return Количество фильтров
	 */
    virtual int GetFiltersCount() const override { return m_Filters.size(); }

	/**
	 * @brief GetFilters Получить список указателей на фильтры
	 * @return Список указателей на фильтры
	 */
    virtual const CAbstractFilter** GetFilters() override;

	/**
	 * @brief AddFilter Добавить фильтр
	 * @param type Тип фильтра
	 * @param order Порядок фильтра
	 * @return Указатель на созданный фильтр или nullptr в случае, если не удалось создать фильтр с указанными параметрами
	 */
    virtual const CAbstractFilter* AddFilter(int type, int order) override;

	/**
	 * @brief AddFilter Добавить фильтр с указанием каналов, для которых он должен применяться
	 * @param type Тип фильтра
	 * @param order Порядок фильтра
	 * @param channelsCount Количество каналов
	 * @param channelsList Список каналов
	 * @return Указатель на созданный фильтр или nullptr в случае, если не удалось создать фильтр с указанными параметрами
	 */
    virtual const CAbstractFilter* AddFilter(int type, int order, int channelsCount, const int *channelsList) override;

	/**
	 * @brief SetupFilter Установить настройки фильтра
	 * @param filter Указатель на фильтр для установки настроек
	 * @param rate Частота дискретизации данных
	 * @param lowFrequency Нижняя частота среза
	 * @param hightFrequency Верхняя частота среза
	 * @return true если настройки применены, false если нет
	 */
    virtual bool SetupFilter(const CAbstractFilter *filter, int rate, int lowFrequency, int hightFrequency) override;

	/**
	 * @brief RemoveFilter Удалить фильтр
	 * @param filter Указатель на фильтр для удаления
	 */
    virtual void RemoveFilter(const CAbstractFilter *filter) override;

	/**
	 * @brief RemoveAllFilters Удалить все фильтры
	 */
    virtual void RemoveAllFilters() override;



	/**
	 * @brief SetCallback_OnStartStateChanged Установить коллбэк для старта/остановки работы с устройством
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_OnStartStateChanged(void *userData, EEG_ON_START_STATE_CHANGED *callback) override;

	/**
	 * @brief SetCallback_OnRecordingStateChanged Установить коллбэк для старта/остановки записи данных в файл
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_OnRecordingStateChanged(void *userData, EEG_ON_RECORDING_STATE_CHANGED *callback) override;

	/**
	 * @brief SetCallback_ReceivedData Установить коллбэк для приема новой порции данных с устройства
	 * @param userData Данные пользователя
	 * @param callback Коллбэк
	 */
    virtual void SetCallback_ReceivedData(void *userData, EEG_ON_RECEIVED_DATA *callback) override;
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // EEG8_H
//----------------------------------------------------------------------------------
