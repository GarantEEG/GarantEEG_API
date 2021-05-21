using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using TinyJson;
using System.Diagnostics;
using System.Threading;

namespace GarantEEG
{
    public class CGarantEEG : IGarantEEG
    {
        //! Состояние подключения к устройству
        public enum CONNECTION_STAGE
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

        //! Типы валидации пакетов с устройств
        public enum GARANT_EEG_PACKET_VALIDATE_TYPE
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

        //! Типы данных  в пакетах
        public enum GARANT_EEG_PACKET_DATA_TYPE
        {
            //! Заголовок
            PDT_HEADER = 1,
            //! Данные
            PDT_DATA
        };


        //! Уровень заряда аккумулятора
        protected int m_BatteryStatus = 0;

        //! Версия прошивки
        protected string m_FirmwareVersion = "";

        //! Текущая частота дискретизации данных устройства
        protected int m_Rate = 500;

        //! Хост для подключения
        protected string m_Host = "192.168.127.125";

        //! Порт для подключения
        protected int m_Port = 12345;

        //! Имя записываемого файла
        protected string m_RecordFileName = "";

        //! Состояние подключения
        protected CONNECTION_STAGE m_ConnectionStage = CONNECTION_STAGE.CS_NONE;

        //! Состояние работы устройства
        protected bool m_Started = false;

        //! Состояние стрима данных с устройства
        protected bool m_TranslationPaused = false;

        //! Состояние записи данных в файл
        protected bool m_Recording = false;

        //! Состояние паузы записи данных в файл
        protected bool m_RecordPaused = false;

        //! Сообщение синхронизации времени
        protected string m_NTPMessage = "";

        //! Размер буфера сетевых данных
        protected const int BUFF_SIZE = 0x10000;

        //! Размер заголовка
        protected const int HEADER_SIZE = 5888;

        //! Размер данных
        protected int DATA_SIZE = 1365;

        //! Сокет
        protected Socket m_Socket;

        //! Файл записи данных
        protected FileStream m_File;

        //! Количество записанных фрэймов
        protected int m_WrittenDataCount = 0;

        //! Размер для защищенного режима
        protected const int PROTECTED_MODE_EXTRA_SIZE = 12;

        //! Размер сообщения синхронизации времени
        protected const int NTP_MESSAGE_SIZE = 40;

        //! Размер буфера записи данных в файл
        protected int WRITE_FILE_BUFFER_SIZE = 0;

        //! Флаг игнорирования каунтера
        protected bool m_IgnoreCounter = false;

        //! Предыдущее значение каунтера
        protected int m_PrevCounter = 0;

        //! Флаг автореконнекта
        protected bool m_EnableAutoreconnection = true;

        //! Список названий каналов
        protected List<string> m_ChannelNames = new List<string>();

        //! Загололвок BDF
        protected List<byte> m_HeaderData;

        //! Предыдущий фрэйм данных
        protected List<byte> m_PrevData;

        //! Буфер приема
        protected byte[] m_RecvBuffer;

        //! Буфер приема (для обработки)
        protected List<byte> m_PacketBuffer;

        //! Буфер записи данных в файл
        protected List<byte> m_FileWriteBuffer;

        //! Буфер отправки
        protected string m_SendBuffer = "";

        //! Применяемые фильтры
        protected List<IFilter> m_Filters;



        /**
         * @brief ValidatePacket Функция проверки чексуммы
         * @param buf Массив данных
         * @param size Размер
         * @return GARANT_EEG_PACKET_VALIDATE_TYPE состояние обработки
         */
        protected GARANT_EEG_PACKET_VALIDATE_TYPE ValidatePacket(ref List<byte> buf, int size, int wantType)
        {
            uint packetID = UnpackUInt32LE(ref buf, 0);

            if (packetID != 0x55AA55AA)
                return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_ID;

            uint packetLength = UnpackUInt16LE(ref buf, 4);

            if (packetLength != size)
                return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_LENGTH;

            int packetType = buf[6];

            if (packetType != wantType)
                return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_TYPE;

            uint packetCrc = UnpackUInt32LE(ref buf, size - 4);
            size -= 4;

            uint[] crc32Table =
            {
                0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
                0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
                0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
                0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
            };

            uint crc = 0xFFFFFFFF;

            for (int i = 0; i < size; i++)
            {
                crc ^= buf[i];

                for (int j = 0; j < 8; j++)
                {
                    uint index = (crc >> 28) & 0x0F;
                    crc = (crc << 4) ^ crc32Table[index];
                }
            }

            if (packetCrc != crc)
                return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_CRC32;

            if (!m_IgnoreCounter && ((m_PrevCounter + 1) % 256) != buf[7])
                return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_COUNTER;

            return GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_VALIDATED;
        }

        /**
	     * @brief UnpackUInt32LE Функция преобразования буфера в 32-битное целое (Little Endian)
	     * @param buf Буфер
         * @return
         */
        protected uint UnpackUInt32LE(ref List<byte> buf, int offset)
        {
            return (uint)((buf[offset + 3] << 24) | (buf[offset + 2] << 16) | (buf[offset + 1] << 8) | buf[offset + 0]);
        }

        /**
         * @brief UnpackUInt16LE Функция преобразования буфера в 16-битное целое (Little Endian)
         * @param buf Буфер
         * @return
         */
        protected uint UnpackUInt16LE(ref List<byte> buf, int offset)
        {
            return (uint)((buf[offset + 1] << 8) | buf[offset + 0]);
        }

        /**
         * @brief UnpackUInt16BE Функция преобразования буфера в 16-битное целое (Big Endian)
         * @param buf Буфер
         * @return
         */
        protected uint UnpackUInt16BE(ref List<byte> buf, int offset)
        {
            return (uint)((buf[offset + 0] << 8) | buf[offset + 1]);
        }

        /**
         * @brief DataReceived Функция обработки принятых данных
         */
        protected void DataReceived()
        {
            if (m_NTPMessage.Length == 0)
            {
                if (m_PacketBuffer.Count < NTP_MESSAGE_SIZE)
                    return;

                Debug.WriteLine("time sync processed");

                Debug.WriteLine("m_PacketBuffer.Count {0} {1}", m_PacketBuffer.Count, System.Text.Encoding.ASCII.GetString(m_PacketBuffer.ToArray()));
                m_NTPMessage = System.Text.Encoding.ASCII.GetString(m_PacketBuffer.Take(NTP_MESSAGE_SIZE).ToArray());
                m_PacketBuffer.RemoveRange(0, NTP_MESSAGE_SIZE);
                Debug.WriteLine("after m_PacketBuffer.Count {0} {1}", m_PacketBuffer.Count, System.Text.Encoding.ASCII.GetString(m_PacketBuffer.ToArray()));

                Debug.WriteLine(m_NTPMessage);

                if (m_SendBuffer.Length != 0)
                {
                    byte[] bytes = Encoding.ASCII.GetBytes(m_SendBuffer);
                    m_Socket.Send(bytes, 0, bytes.Length, SocketFlags.None);
                    m_SendBuffer = "";
                }
            }

            //byte[] buffer = m_RecvBuffer;

            //Debug.WriteLine("ProcessRecv m_RecvBuffer: {0}", System.Text.Encoding.UTF8.GetString(m_RecvBuffer));

            int wantLength = HEADER_SIZE + 12;
            int realDataOffset = 8;

            string[] validationErrorMessages =
            {
                "No error",
                "Error ID",
                "Error Length",
                "Error Type",
                "Error CRC32",
                "Error Counter"
            };

            m_IgnoreCounter = false;

            if (m_HeaderData.Count < HEADER_SIZE)
            {
                m_IgnoreCounter = true;

                while (m_PacketBuffer.Count >= wantLength)
                {
                    Debug.WriteLine("header processed");

                    GARANT_EEG_PACKET_VALIDATE_TYPE validateState = ValidatePacket(ref m_PacketBuffer, wantLength, (int)GARANT_EEG_PACKET_DATA_TYPE.PDT_HEADER);

                    if (validateState != GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_VALIDATED)
                    {
                        int skippedCount = 0;

                        skippedCount = 1;
                        m_PacketBuffer.RemoveAt(0);

                        while (m_PacketBuffer.Count >= wantLength && UnpackUInt32LE(ref m_PacketBuffer, 0) != 0x55AA55AA)
                        {
                            m_PacketBuffer.RemoveAt(0);
                            skippedCount++;
                        }

                        if (validateState != GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_COUNTER)
                        {
                            if (UnpackUInt32LE(ref m_PacketBuffer, 0) == 0x55AA55AA)
                                Debug.WriteLine("header validation failed! code: {0} ({1}), skiped {2} bytes to correct ID", validateState, validationErrorMessages[(int)validateState], skippedCount);
                            else
                                Debug.WriteLine("header validation failed! code: {0} ({1}), skiped {2} bytes", validateState, validationErrorMessages[(int)validateState], skippedCount);
                        }

                        continue;
                    }

                    List<byte> temp = m_PacketBuffer.Take(wantLength).ToList();
                    temp.RemoveRange(0, realDataOffset);
                    m_HeaderData.InsertRange(m_HeaderData.Count, temp.Take(HEADER_SIZE).ToList());
                    m_PacketBuffer.RemoveRange(0, wantLength);

                    break;
                }

                if (m_HeaderData.Count < HEADER_SIZE)
                    return;
            }

            wantLength = DATA_SIZE + 12;

            while (m_PacketBuffer.Count >= wantLength)
            {
                GARANT_EEG_PACKET_VALIDATE_TYPE validateState = ValidatePacket(ref m_PacketBuffer, wantLength, (int)GARANT_EEG_PACKET_DATA_TYPE.PDT_DATA);

                byte counter = m_PacketBuffer[7];

                //qDebug() << "protected, counter:" << counter << " prevCounter: " << m_PrevCounter;

                if (validateState != GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_VALIDATED)
                {
                    //unsigned char badType = ((unsigned char*)&m_RecvBuffer[0])[6];

                    int skippedCount = 0;

                    if (validateState != GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_COUNTER)
                    {
                        skippedCount = 1;
                        m_PacketBuffer.RemoveAt(0);

                        while (m_PacketBuffer.Count >= wantLength && UnpackUInt32LE(ref m_PacketBuffer, 0) != 0x55AA55AA)
                        {
                            m_PacketBuffer.RemoveAt(0);
                            skippedCount++;
                        }
                    }

                    if (UnpackUInt32LE(ref m_PacketBuffer, 0) == 0x55AA55AA)
                        Debug.WriteLine("data validation failed! code: {0} ({1}), skiped {2} bytes to correct ID, counter: {3} prevCounter: {4}", validateState, validationErrorMessages[(int)validateState], skippedCount, counter, m_PrevCounter);
                    else
                        Debug.WriteLine("data validation failed! code: {0} ({1}), skiped {2} bytes, counter: {3} prevCounter: {4}", validateState, validationErrorMessages[(int)validateState], skippedCount, counter, m_PrevCounter);

                    if (validateState != GARANT_EEG_PACKET_VALIDATE_TYPE.PVT_BAD_COUNTER)
                    {
                        if (m_PrevData.Count > 0 && m_PrevCounter != counter)
                        {
                            m_PrevCounter = counter;
                            ProcessData(ref m_PrevData, m_PrevData.Count);
                        }

                        continue;
                    }
                }

                m_PrevCounter = counter;

                List<byte> temp = m_PacketBuffer.Take(wantLength).ToList();
                temp.RemoveRange(0, realDataOffset);

                ProcessData(ref temp, DATA_SIZE);

                m_PacketBuffer.RemoveRange(0, wantLength);
            }
        }

        /**
         * @brief SendPacket Функция отправки пакета устройству
         * @param text Данные пакета
         */
        protected void SendPacket(string text)
        {
            if (m_Socket == null || !m_Socket.Connected || !m_Started || text.Length == 0)
                return;

            if (m_ConnectionStage < CONNECTION_STAGE.CS_CONNECTED || m_NTPMessage.Length == 0)
                m_SendBuffer += text;
            else
            {
                byte[] bytes = Encoding.ASCII.GetBytes(text);
                m_Socket.Send(bytes, 0, bytes.Length, SocketFlags.None);
            }
        }

        protected double Unpack24BitValue(ref List<byte> buf, int offset)
        {
            int intValue = ((buf[offset + 2] << 16) | (buf[offset + 1] << 8) | buf[offset + 0]);

            if (intValue >= 8388608)
                intValue -= 16777216;

            return (double)intValue;
        }

        /**
            * @brief ProcessData Функция обработки набора данных
            * @param buf Указатель на массив данных
            * @param size Размер данных
            */
        protected void ProcessData(ref List<byte> buf, int size)
        {
            //Debug.WriteLine("ProcessData: {0}", buf.Count);
            GARANT_EEG_DATA frameData = new GARANT_EEG_DATA();

            if (m_Rate == 250)
                frameData.DataRecordsCount = 25;
            else if (m_Rate == 500)
                frameData.DataRecordsCount = 50;
            else if (m_Rate == 1000)
                frameData.DataRecordsCount = 100;

            if (m_Recording && !m_RecordPaused && m_File != null)
            {
                if (m_FileWriteBuffer.Count > WRITE_FILE_BUFFER_SIZE)
                    m_File.Write(m_FileWriteBuffer.ToArray(), 0, m_FileWriteBuffer.Count);

                m_FileWriteBuffer.AddRange(buf.Take(size));
                m_WrittenDataCount++;
            }

            for (int i = 0; i < 22; i++)
            {
                if (i < 8) //main channels
                {
                    int ptr = (i * frameData.DataRecordsCount * 3);

                    for (int j = 0; j < frameData.DataRecordsCount; j++, ptr += 3)
                    {
                        //frameData.ChannelsData[j].Value[i] = (int)(unpack24BitValue(ptr) * 0.000447) / 10.0; // из кода matlab

                        double v = (Unpack24BitValue(ref buf, ptr) * 0.000447) / 10.0 / 1000.0;
                        frameData.RawChannelsData[j].Value[i] = v;
                        frameData.FilteredChannelsData[j].Value[i] = v; // /1000.0 для перевода в микровольты (из милливольт)
                    }
                }
                else if (i < 11) //accelerometr
                {
                    //uchar *ptr = buf + ((8 * frameData.Channels.size() * 3) + (i - 8) * frameData.Accelerometr.size() * 3);
                }
                else if (i < 21) //rx
                {
                    int ptr = ((8 * frameData.DataRecordsCount * 3 + 3 * 5 * 3) + (i - 11) * 1 * 3);

                    for (int j = 0; j < 1; j++, ptr += 3)
                    {
                        double val = (Unpack24BitValue(ref buf, ptr) * 0.0677) / 10.0;

                        int pos = i - 11;

                        if (pos < 8)
                            frameData.ResistanceData.Value[pos] = val;
                        else if (pos == 8)
                            frameData.ResistanceData.Ref = val;
                        else
                            frameData.ResistanceData.Ground = val;
                    }
                }
                else
                {
                    //annotations
                    //uchar *ptr = buf + ((8 * frameData.Channels.size() * 3 + 3 * frameData.Accelerometr.size() * 3) + 10 * frameData.RX.size() * 3);
                }
            }

            buf.RemoveRange(0, size - 90);
            string jsonData = System.Text.Encoding.ASCII.GetString(buf.ToArray());

            //Debug.WriteLine("data.ToString() => {0} {1}", buf.Count, jsonData);
            Dictionary<string, object> data = jsonData.Decode<Dictionary<string, object>>();

            if (data != null)
            {
                if (data.ContainsKey("FW Version"))
                {
                    m_FirmwareVersion = data["FW Version"].ToString();
                    //Debug.WriteLine("FW Version: {0}", m_FirmwareVersion);
                }

                if (data.ContainsKey("Battery %"))
                {
                    m_BatteryStatus = int.Parse(data["Battery %"].ToString());
                    //Debug.WriteLine("Battery %: {0}", m_BatteryStatus);
                }

                if (data.ContainsKey("Block's Time"))
                {
                    frameData.Time = double.Parse(data["Block's Time"].ToString());
                    //Debug.WriteLine("Block's Time: {0}", frameData.Time);
                }
            }

            if (m_Filters.Count > 0)
            {
                foreach (var filter in m_Filters)
                {
                    if (filter == null)
                        continue;

                    float[][] channels = new float[8][];

                    for (int i = 0; i < 8; i++)
                    {
                        channels[i] = new float[frameData.DataRecordsCount];
                    }

                    float multiply = 1.0f;

                    /*int index = ui->comboBoxVoltage->currentIndex();
                    if(0 == index)
                    {
                        multiply = 1000000.0;
                    }
                    else if(1 == index)
                    {
                        multiply = 1000.0;
                    }*/

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < frameData.DataRecordsCount; j++)
                        {
                            float value = (float)frameData.RawChannelsData[j].Value[i];

                            if (Math.Abs(value * 1000000) >= 374000)
                                value = 0.0f;
                            else
                                value *= multiply;

                            channels[i][j] = value;
                        }

                        filter.Process(channels[i]);
                    }

                    for (int i = 0; i < 8; i++)
                    {
                        for (int j = 0; j < frameData.DataRecordsCount; j++)
                        {
                            frameData.FilteredChannelsData[j].Value[i] = (double)channels[i][j];
                        }
                    }
                }
            }

            ReceivedData.Invoke(this, frameData);
        }


















        /**
         * @brief CGarantEEG Конструктор
         */
        public CGarantEEG()
        {
            m_Filters = new List<IFilter>();

            m_ChannelNames.Add("Po7");
            m_ChannelNames.Add("O1");
            m_ChannelNames.Add("Oz");
            m_ChannelNames.Add("P3");
            m_ChannelNames.Add("Pz");
            m_ChannelNames.Add("P4");
            m_ChannelNames.Add("O2");
            m_ChannelNames.Add("Po8");
        }

        /**
         * @brief ~CEeg8 Деструктор
         */
        ~CGarantEEG()
        {
            RemoveAllFilters();
            m_Filters = null;
        }



        /**
         * @brief GetType Получение типа устройства
         * @return Тип устройства
         */
        public GARANT_EEG_DEVICE_TYPE GetDeviceType()
        {
            return GARANT_EEG_DEVICE_TYPE.DT_GARANT;
        }



        /**
	     * @brief IsConnecting Получение состояния подключения устройства
	     * @return true
	     */
        public bool IsConnecting()
        {
            return (m_ConnectionStage == CONNECTION_STAGE.CS_CONNECTING);
        }

        //! Коллбэк состояния подключения к устройству
        public event EventHandler<int> ConnectionStateChanged;

        //! Коллбэк состояния записи данных в файл
        public event EventHandler<int> RecordingStateChanged;

        //! Коллбэк приема нового фрэйма данных
        public event EventHandler<GARANT_EEG_DATA> ReceivedData;

        private bool ConnectTo(EndPoint endpoint)
        {
            m_ConnectionStage = CONNECTION_STAGE.CS_CONNECTING;

            try
            {
                m_Socket.Connect(endpoint);

                if (m_Socket.Connected)
                {
                    m_ConnectionStage = CONNECTION_STAGE.CS_CONNECTED;

                    if (ConnectionStateChanged != null)
                        ConnectionStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_CONNECTION_STATE.DCS_NO_ERROR);

                    StartRecv();
                }
                else
                {
                    m_ConnectionStage = CONNECTION_STAGE.CS_CONNECTION_ERROR;

                    if (ConnectionStateChanged != null)
                        ConnectionStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_CONNECTION_STATE.DCS_HOST_NOT_REACHED);
                }
            }
            catch (SocketException e)
            {
                m_ConnectionStage = CONNECTION_STAGE.CS_CONNECTION_ERROR;

                if (ConnectionStateChanged != null)
                    ConnectionStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_CONNECTION_STATE.DCS_HOST_NOT_REACHED);

                return false;
            }

            return (m_ConnectionStage == CONNECTION_STAGE.CS_CONNECTED);
        }

        private void StartRecv()
        {
            //Debug.WriteLine("StartRecv conn {0}", m_Socket.Connected);
            if (m_Socket.Connected)
            {
                m_Socket.BeginReceive(m_RecvBuffer, 0, BUFF_SIZE - HEADER_SIZE, SocketFlags.None, ProcessRecv, null);
                //Debug.WriteLine("StartRecv done");
            }
        }

        private void ProcessRecv(IAsyncResult e)
        {
            //Debug.WriteLine("ProcessRecv start {0}", m_Socket.Connected);
            if (m_Socket == null || !m_Socket.Connected)
            {
                Stop();
                return;
            }

            try
            {
                int bytesLen = m_Socket.EndReceive(e);

                //Debug.WriteLine("ProcessRecv bytesLen: {0}", bytesLen);
                if (bytesLen > 0)
                {
                    m_PacketBuffer.AddRange(m_RecvBuffer.Take(bytesLen));
                    DataReceived();

                    StartRecv();
                }
                else
                {
                    //Disconnect(SocketError.SocketError);
                }
            }
            catch (SocketException socketException)
            {
                Debug.WriteLine("ProcessRecv err1");
                //Disconnect(socketException.SocketErrorCode);
                Stop();
            }
            catch (Exception ex)
            {
                Debug.WriteLine("ProcessRecv err2 {0}", ex);
                Stop();
            }
        }

        /**
         * @brief Start Функция старта работы с устройством
         * @param waitForConnection Ожидать ли подключения или подключиться к оборудованию в асинхронном режиме
         * @param rate Частота работы устройства (250/500/1000)
         * @param host IP-адрес для подключения
         * @param port Порт для подключения
         * @return true если подключено (или подключение началось, для асинхронного режима)
         */
        public bool Start(bool waitForConnection = true, int rate = 500, string host = "192.168.127.125", int port = 12345)
        {
            if (m_Started || m_ConnectionStage == CONNECTION_STAGE.CS_CONNECTED)
                return true;
            else if (m_ConnectionStage == CONNECTION_STAGE.CS_CONNECTING)
                return false;

            IPAddress address = ResolveIP(host);

            if (address == null)
            {
                return false;
            }

            m_ConnectionStage = CONNECTION_STAGE.CS_NONE;
            m_Started = false;
            m_Recording = false;
            m_RecordPaused = false;
            m_TranslationPaused = false;
            m_Rate = rate;

            switch (m_Rate)
            {
                case 250:
                    {
                        DATA_SIZE = 765;
                        break;
                    }
                case 500:
                    {
                        DATA_SIZE = 1365;
                        break;
                    }
                case 1000:
                    {
                        DATA_SIZE = 2565;
                        break;
                    }
                default:
                    break;
            }

            WRITE_FILE_BUFFER_SIZE = DATA_SIZE * 10 * 30; //10 - count of records per second, 30 seconds record
            m_Host = host;
            m_Port = port;
            m_NTPMessage = "";
            m_SendBuffer = "";


            m_Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp)
            { ReceiveBufferSize = BUFF_SIZE, SendBufferSize = BUFF_SIZE };

            m_Socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);
            m_HeaderData = new List<byte>();
            m_PrevData = new List<byte>();
            m_RecvBuffer = new byte[BUFF_SIZE];
            m_PacketBuffer = new List<byte>();
            m_FileWriteBuffer = new List<byte>();

            m_Socket.ReceiveTimeout = -1;
            m_Socket.SendTimeout = -1;

            IPEndPoint endpoint = new IPEndPoint(address, port);

            if (waitForConnection)
            {
                m_Started = ConnectTo(endpoint);

                if (m_Started)
                    SendPacket(string.Format("start -protect eeg.rate {0}\r\n", m_Rate));
            }
            else
            {
                Task.Run(() =>
                {
                    m_Started = ConnectTo(endpoint);

                    if (m_Started)
                        SendPacket(string.Format("start -protect eeg.rate {0}\r\n", m_Rate));
                });
            }

            while (waitForConnection && m_ConnectionStage < CONNECTION_STAGE.CS_CONNECTED)
                Thread.Sleep(1);

            if (waitForConnection)
                return (m_ConnectionStage == CONNECTION_STAGE.CS_CONNECTED);

            return true;
        }

        private static IPAddress ResolveIP(string addr)
        {
            IPAddress result = IPAddress.None;

            if (string.IsNullOrEmpty(addr))
            {
                return result;
            }

            if (!IPAddress.TryParse(addr, out result))
            {
                try
                {
                    IPHostEntry hostEntry = Dns.GetHostEntry(addr);

                    if (hostEntry.AddressList.Length != 0)
                    {
                        result = hostEntry.AddressList[hostEntry.AddressList.Length - 1];
                    }
                }
                catch
                {
                }
            }

            return result;
        }

        /**
	     * @brief Stop Остановить работу с устройством
	     */
        public void Stop()
        {
            if (!m_Started)
                return;

            StopRecord();

            m_Started = false;
            m_TranslationPaused = false;

            m_Socket.Shutdown(SocketShutdown.Both);

            m_ConnectionStage = CONNECTION_STAGE.CS_NONE;

            m_HeaderData = null;
            m_PrevData = null;
            m_RecvBuffer = null;
            m_PacketBuffer = null;
            m_FileWriteBuffer = null;
            m_Socket = null;
        }

        /**
	     * @brief IsStarted Состояние работы с устройством
	     * @return true если подключены, false если нет подключения
	     */
        public bool IsStarted()
        {
            return m_Started;
        }

        /**
	     * @brief IsPaused Состояние стримов данных
	     * @return true если стримы активны, false если передача данных приостановлена пользователем
	     */
        public bool IsPaused()
        {
            return m_TranslationPaused;
        }

        string FixEdfString(string str, int maxLength)
        {
            if (str.Length > maxLength)
                str = str.Take(maxLength).ToString();
            else
            {
                while (str.Length < maxLength)
                    str += " ";
            }

            return str;
        }

        /**
         * @brief StartRecord Начать запись данных в файл
         * @param userName Имя респондента
         * @param filePath Путь к BDF файлу, в который нужно записывать данные
         * @return true если запись в файл началась успешно, false если запись не началась
         */
        public bool StartRecord(string userName, string filePath = "")
        {
            Debug.WriteLine("StartRecord");
            if (!m_Started || m_Recording || m_TranslationPaused || m_File != null)
            {
                Debug.WriteLine("StartRecord error 1");
                return false;
            }
            else if (m_HeaderData.Count == 0)
            {
                Debug.WriteLine("StartRecord error 2");
                if (RecordingStateChanged != null)
                    RecordingStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_RECORDING_STATE.DRS_HEADER_NOT_FOUND);

                return false;
            }

            if (filePath != "")
                m_RecordFileName = filePath;
            else
            {
                string path = AppDomain.CurrentDomain.BaseDirectory + "SaveData";

                if (!Directory.Exists(path))
                    Directory.CreateDirectory(path);

                m_RecordFileName = path + "/EegRecord_" + DateTime.Now.ToString("yyyy_MM_dd___HH_mm_ss") + ".bdf";
            }

            m_FileWriteBuffer = null;

            Debug.WriteLine("start recording in {0}", m_RecordFileName);

            m_File = new FileStream(m_RecordFileName, FileMode.OpenOrCreate, FileAccess.ReadWrite);

            if (m_File != null)
            {
                m_File.Write(m_HeaderData.ToArray(), 0, m_HeaderData.Count);

                string name = FixEdfString(userName, 80);
                byte[] writeBuf = Encoding.ASCII.GetBytes(name);
                m_File.Seek(8, SeekOrigin.Begin);
                m_File.Write(writeBuf, 0, writeBuf.Length);

                for (int i = 0; i < (int)m_ChannelNames.Count; i++)
                {
                    string electrodeName = FixEdfString(m_ChannelNames[i], 16);
                    byte[] writeElectrodeBuf = Encoding.ASCII.GetBytes(electrodeName);
                    m_File.Seek(256 + (16 * i), SeekOrigin.Begin);
                    m_File.Write(writeElectrodeBuf, 0, writeElectrodeBuf.Length);
                }

                m_File.Seek(m_HeaderData.Count, SeekOrigin.Begin);

                m_FileWriteBuffer = new List<byte>();
                m_WrittenDataCount = 0;
                m_Recording = true;
                m_RecordPaused = false;

                if (RecordingStateChanged != null)
                    RecordingStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_RECORDING_STATE.DRS_NO_ERROR);
            }
            else
            {
                if (RecordingStateChanged != null)
                    RecordingStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_RECORDING_STATE.DRS_CREATE_FILE_ERROR);
            }

            return m_Recording;
        }

        /**
	     * @brief StopRecord Остановить запись данных в файл
	     */
        public void StopRecord()
        {
            Debug.WriteLine("StopRecord");
            if (!m_Started || !m_Recording || m_TranslationPaused || m_File == null)
                return;

            if (m_FileWriteBuffer != null && m_FileWriteBuffer.Count > 0)
            {
                m_File.Write(m_FileWriteBuffer.ToArray(), 0, m_FileWriteBuffer.Count);
            }

            if (m_WrittenDataCount > 0)
            {
                string recordsCountText = FixEdfString(m_WrittenDataCount.ToString(), 8);
                m_File.Seek(236, SeekOrigin.Begin);
                byte[] writeBuf = Encoding.ASCII.GetBytes(recordsCountText);
                m_File.Write(writeBuf, 0, writeBuf.Length);
            }

            m_FileWriteBuffer = null;

            m_File.Close();
            m_File = null;
            m_WrittenDataCount = 0;
            m_Recording = false;
            m_RecordPaused = false;
            Debug.WriteLine("Record stopped");

            if (RecordingStateChanged != null)
                RecordingStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_RECORDING_STATE.DRS_RECORD_STOPPED);
        }

        /**
	     * @brief IsRecording Получить состояние записи данных в файл
	     * @return true если запись данных в файл идет, false если нет
	     */
        public bool IsRecording() { return m_Recording; }

        /**
	     * @brief PauseRecord Приостановить запись данных в файл (без фактической остановки записи, с возможностью продолжения записи)
	     */
        public void PauseRecord()
        {
            if (m_Started && m_Recording)
            {
                m_RecordPaused = true;

                if (RecordingStateChanged != null)
                    RecordingStateChanged.Invoke(this, (int)GARANT_EEG_DEVICE_RECORDING_STATE.DRS_RECORD_PAUSED);
            }
        }

        /**
	     * @brief ResumeRecord Возобновить запись данных в файл
	     */
        public void ResumeRecord()
        {
        }

        /**
	     * @brief IsRecordPaused Получить состояние паузы записи данных в файл
	     * @return true если запись данных в файл на паузе, false если нет
	     */
        public bool IsRecordPaused() { return m_RecordPaused; }

        /**
	     * @brief SetAutoReconnection Установить состояние автопереподключения к устройству при разрыве соединения
	     * @param enable Новое состояние
	     */
        public void SetAutoReconnection(bool enable) { m_EnableAutoreconnection = enable; }

        /**
	     * @brief AutoReconnectionEnabled Получить состояние автопереподключения к устройству при разрыве соединения
	     * @return true если автопереподключение активно, false если нет
	     */
        public bool AutoReconnectionEnabled() { return m_EnableAutoreconnection; }



        /**
	     * @brief StartDataTranslation Запустить стрим данных с устройства
	     */
        public void StartDataTranslation()
        {
        }

        /**
	     * @brief StopDataTranslation Остановить стрим данных с устройства (без фактического отключения от устройства)
	     */
        public void StopDataTranslation()
        {
        }



        /**
	     * @brief SetRxThreshold Установить пороговое значение для сопротивления
	     * @param value Значение
	     */
        public void SetRxThreshold(int value)
        {
        }

        /**
	     * @brief StartIndicationTest Запустить тест индикации светодиодов
	     */
        public void StartIndicationTest()
        {
        }

        /**
	     * @brief StopIndicationTest Остановить тест индикации светодиодов
	     */
        public void StopIndicationTest()
        {
        }



        /**
	     * @brief PowerOff Выключить устройство
	     */
        public void PowerOff()
        {
        }

        /**
	     * @brief SynchronizationWithNTP Послать команду синхронизации с NTP сервером (NTP сервер должен быть настроек на клиенте)
	     */
        public void SynchronizationWithNTP()
        {
        }

        /**
	     * @brief GetRate Получить текущую частоту дискретизации данных, с которой работает устройство
	     * @return Частота дискретизации данных
	     */
        public int GetRate()
        {
            return m_Rate;
        }

        /**
	     * @brief GetBatteryStatus Получить уровень заряда аккумулятора
	     * @return Значение в процентах
	     */
        public int GetBatteryStatus()
        {
            return m_BatteryStatus;
        }

        /**
	     * @brief GetFirmwareVersion Получить версию прошивки
	     * @return Версия прошивки
	     */
        public string GetFirmwareVersion()
        {
            return m_FirmwareVersion;
        }



        /**
	     * @brief GetFiltersCount Получить количество фильтров
	     * @return Количество фильтров
	     */
        public int GetFiltersCount()
        {
            return m_Filters.Count;
        }

        /**
	     * @brief GetFilters Получить список указателей на фильтры
	     * @return Список указателей на фильтры
	     */
        public List<IFilter> GetFilters()
        {
            return m_Filters.ToList<IFilter>();
        }

        /**
	     * @brief AddFilter Добавить фильтр
	     * @param type Тип фильтра
         * @param rate Частота дискретизации данных
         * @param frequency Частота среза
	     * @return Указатель на созданный фильтр или nullptr в случае, если не удалось создать фильтр с указанными параметрами
	     */
        public IFilter AddFilter(int type, int rate, int frequency)
        {
            IFilter filter = null;

            if (type == (int)GARANT_EEG_FILTER_TYPE.FT_BUTTERWORTH)
            {
                filter = new CButterworthFilter(rate, frequency);
            }

            if (filter != null)
                m_Filters.Add(filter);

            return m_Filters.Last();
        }

        /**
	     * @brief RemoveFilter Удалить фильтр
	     * @param filter Указатель на фильтр для удаления
	     */
        public void RemoveFilter(IFilter filter)
        {
            m_Filters.Remove(filter);
        }

        /**
	     * @brief RemoveAllFilters Удалить все фильтры
	     */
        public void RemoveAllFilters()
        {
            m_Filters.Clear();
        }
    }
}
