using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TinyJson;

namespace GarantEEG
{
    ////////////////////////////////////////////////////////////////////////////////////
    //! Основные перечисления
    ////////////////////////////////////////////////////////////////////////////////////
    //! Перечисление доступных устройств для работы
    public enum GARANT_EEG_DEVICE_TYPE
    {
        //! Базовый класс для работы с устройством GarantEEG
        DT_GARANT = 0
    }

    //! Основные типы фильтров
    public enum GARANT_EEG_FILTER_TYPE
    {
        //! Неизвестный фильтр, не валидно
        FT_UNKNOWN = 0,
        //! Частотный фильтр Butterworth
        FT_BUTTERWORTH
    }

    //! Состояния подключения к устройству
    public enum GARANT_EEG_DEVICE_CONNECTION_STATE
    {
        //! Нет ошибок, подключены
        DCS_NO_ERROR = 0,
        //! Ошибка создания сокета
        DCS_CREATE_SOCKET_ERROR,
        //! Указанный хост не найден
        DCS_HOST_NOT_FOUND,
        //! Указанный хост недоступен
        DCS_HOST_NOT_REACHED,
        //! Ошибка во время получения данных
        DCS_RECEIVE_ERROR,
        //! Подключение разорвано
        DCS_CONNECTION_CLOSED = 10000,
        //! Стрим данных приостановлен
        DCS_TRANSLATION_PAUSED,
        //! Стрим данных возобновлен
        DCS_TRANSLATION_RESUMED
    };

    //! Состояния записи данных в файл
    enum GARANT_EEG_DEVICE_RECORDING_STATE
    {
        //! Нет ошибок, запись началась
        DRS_NO_ERROR = 0,
        //! Ошибка создания файла
        DRS_CREATE_FILE_ERROR,
        //! Не получен заголовок BDF файла с устройства
        DRS_HEADER_NOT_FOUND,
        //! Запись данных в файл приостановлена
        DRS_RECORD_PAUSED = 10000,
        //! Запись данных в файл возобновлена
        DRS_RECORD_RESUMED,
        //! Запись данных в файл остановлена
        DRS_RECORD_STOPPED
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //! Основные структуры
    ////////////////////////////////////////////////////////////////////////////////////
    //! Структура с данными по каналам
    public class GARANT_EEG_CHANNELS_DATA
    {
        public GARANT_EEG_CHANNELS_DATA()
        {
            Value = new double[8];
        }
        //! Значения для 8 каналов
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public double[] Value;
    };

    //! Структура с данными по сопротивлению
    public class GARANT_EEG_RESISTANCE_DATA
    {
        public GARANT_EEG_RESISTANCE_DATA()
        {
            Value = new double[8];
        }
        //! Значения для 8 каналов
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public double[] Value;

        //! Значение референта
        public double Ref = 0.0;

        //! Значение земли
        public double Ground = 0.0;
    };

    //! Структура с данными по акселерометру
    public class GARANT_EEG_ACCELEROMETR_DATA
    {
        //! Значение по оси X
        public double X = 0.0;

        //! Значение по оси Y
        public double Y = 0.0;

        //! Значение по оси Z
        public double Z = 0.0;
    };

    //! Структура с данными фрэйма GarantEEG
    public class GARANT_EEG_DATA
    {
        public GARANT_EEG_DATA()
        {
            RawChannelsData = new GARANT_EEG_CHANNELS_DATA[100];
            FilteredChannelsData = new GARANT_EEG_CHANNELS_DATA[100];

            for (int i = 0; i < 100; i++)
            {
                RawChannelsData[i] = new GARANT_EEG_CHANNELS_DATA();
                FilteredChannelsData[i] = new GARANT_EEG_CHANNELS_DATA();
            }

            ResistanceData = new GARANT_EEG_RESISTANCE_DATA();
            AccelerometrData = new GARANT_EEG_ACCELEROMETR_DATA[5];

            for (int i = 0; i < 5; i++)
            {
                AccelerometrData[i] = new GARANT_EEG_ACCELEROMETR_DATA();
            }

            Annitations = new char[30];
    }

        //! Метка времени текущего пакета
        public double Time = 0.0;

        //! Количество наборов данных по каналам
        public int DataRecordsCount = 0;

        //! Сырые данные по каналам (реальное количество - DataRecordsCount)
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 100)]
        public GARANT_EEG_CHANNELS_DATA[] RawChannelsData;

        //! Отфильтрованные с помощью установленных частотных фильтров данные по каналам (реальное количество - DataRecordsCount)
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 100)]
        public GARANT_EEG_CHANNELS_DATA[] FilteredChannelsData;

        //! Данные по сопротивлению
        public GARANT_EEG_RESISTANCE_DATA ResistanceData;

        //! Данные акселерометра
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
        public GARANT_EEG_ACCELEROMETR_DATA[] AccelerometrData;

        //! Аннотации к текущему фрэйму
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 30)]
        public char[] Annitations;
    };
}
