using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TinyJson;

namespace GarantEEG
{
    public interface IGarantEEG
    {
	    /**
	     * @brief GetType Получение типа устройства
	     * @return Тип устройства
	     */
        GARANT_EEG_DEVICE_TYPE GetDeviceType();



        /**
         * @brief IsConnecting Получение состояния подключения устройства
         * @return true
         */
        bool IsConnecting();

        /**
         * @brief Start Функция старта работы с устройством
         * @param waitForConnection Ожидать ли подключения или подключиться к оборудованию в асинхронном режиме
         * @param rate Частота работы устройства (250/500/1000)
         * @param host IP-адрес для подключения
         * @param port Порт для подключения
         * @return true если подключено (или подключение началось, для асинхронного режима)
         */
        bool Start(bool waitForConnection = true, int rate = 500, string host = "192.168.127.125", int port = 12345);

        /**
         * @brief Stop Остановить работу с устройством
         */
        void Stop();

	    /**
	     * @brief IsStarted Состояние работы с устройством
	     * @return true если подключены, false если нет подключения
	     */
        bool IsStarted();

        /**
         * @brief IsPaused Состояние стримов данных
         * @return true если стримы активны, false если передача данных приостановлена пользователем
         */
        bool IsPaused();

        /**
         * @brief StartRecord Начать запись данных в файл
         * @param userName Имя респондента
         * @param filePath Путь к BDF файлу, в который нужно записывать данные
         * @return true если запись в файл началась успешно, false если запись не началась
         */
        bool StartRecord(string userName, string filePath = "");

        /**
         * @brief StopRecord Остановить запись данных в файл
         */
        void StopRecord();

	    /**
	     * @brief IsRecording Получить состояние записи данных в файл
	     * @return true если запись данных в файл идет, false если нет
	     */
        bool IsRecording();

        /**
         * @brief PauseRecord Приостановить запись данных в файл (без фактической остановки записи, с возможностью продолжения записи)
         */
        void PauseRecord();

	    /**
	     * @brief ResumeRecord Возобновить запись данных в файл
	     */
        void ResumeRecord();

	    /**
	     * @brief IsRecordPaused Получить состояние паузы записи данных в файл
	     * @return true если запись данных в файл на паузе, false если нет
	     */
        bool IsRecordPaused();

        /**
         * @brief SetAutoReconnection Установить состояние автопереподключения к устройству при разрыве соединения
         * @param enable Новое состояние
         */
        void SetAutoReconnection(bool enable);

	    /**
	     * @brief AutoReconnectionEnabled Получить состояние автопереподключения к устройству при разрыве соединения
	     * @return true если автопереподключение активно, false если нет
	     */
        bool AutoReconnectionEnabled();



        /**
         * @brief StartDataTranslation Запустить стрим данных с устройства
         */
        void StartDataTranslation();

	    /**
	     * @brief StopDataTranslation Остановить стрим данных с устройства (без фактического отключения от устройства)
	     */
        void StopDataTranslation();



	    /**
	     * @brief SetRxThreshold Установить пороговое значение для сопротивления
	     * @param value Значение
	     */
        void SetRxThreshold(int value);

	    /**
	     * @brief StartIndicationTest Запустить тест индикации светодиодов
	     */
        void StartIndicationTest();

	    /**
	     * @brief StopIndicationTest Остановить тест индикации светодиодов
	     */
        void StopIndicationTest();



	    /**
	     * @brief PowerOff Выключить устройство
	     */
        void PowerOff();

	    /**
	     * @brief SynchronizationWithNTP Послать команду синхронизации с NTP сервером (NTP сервер должен быть настроек на клиенте)
	     */
        void SynchronizationWithNTP();

	    /**
	     * @brief GetRate Получить текущую частоту дискретизации данных, с которой работает устройство
	     * @return Частота дискретизации данных
	     */
        int GetRate();

        /**
         * @brief GetBatteryStatus Получить уровень заряда аккумулятора
         * @return Значение в процентах
         */
        int GetBatteryStatus();

        /**
         * @brief GetFirmwareVersion Получить версию прошивки
         * @return Версия прошивки
         */
        string GetFirmwareVersion();



        /**
         * @brief GetFiltersCount Получить количество фильтров
         * @return Количество фильтров
         */
        int GetFiltersCount();

        /**
         * @brief GetFilters Получить список указателей на фильтры
         * @return Список указателей на фильтры
         */
        List<IFilter> GetFilters();

        /**
         * @brief AddFilter Добавить фильтр
         * @param type Тип фильтра
         * @param rate Частота дискретизации данных
         * @param frequency Частота среза
         * @return Указатель на созданный фильтр или null в случае, если не удалось создать фильтр с указанными параметрами
         */
        IFilter AddFilter(int type, int rate, int frequency);

        /**
         * @brief RemoveFilter Удалить фильтр
         * @param filter Указатель на фильтр для удаления
         */
        void RemoveFilter(IFilter filter);

        /**
         * @brief RemoveAllFilters Удалить все фильтры
         */
        void RemoveAllFilters();



        //! Коллбэк состояния подключения к устройству
        event EventHandler<int> ConnectionStateChanged;

        //! Коллбэк состояния записи данных в файл
        event EventHandler<int> RecordingStateChanged;

        //! Коллбэк приема нового фрэйма данных
        event EventHandler<GARANT_EEG_DATA> ReceivedData;
    }
}
