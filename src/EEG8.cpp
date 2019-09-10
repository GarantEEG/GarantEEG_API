/**
@file EEG8.cpp

@brief Класс для управления устройством EEG8

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#include "src/EEG8.h"
#include <windows.h>
#include "../../../components/rapidjson/document.h"
#include "Filtering/ButterworthFilter.hpp"
#include <QDebug>
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
CEeg8::CEeg8()
: m_HeaderSize(5888), m_DataSize(1365)
{
    m_ChannelNames.push_back("Po7");
    m_ChannelNames.push_back("O1");
    m_ChannelNames.push_back("Oz");
    m_ChannelNames.push_back("P3");
    m_ChannelNames.push_back("Pz");
    m_ChannelNames.push_back("P4");
    m_ChannelNames.push_back("O2");
    m_ChannelNames.push_back("Po8");

    //SetupFilter(AddFilter(FT_BUTTERWORTH, 2), 500, 1, 45);
    //SetupFilter(AddFilter(FT_BUTTERWORTH, 8), 500, 1, 30);
}
//----------------------------------------------------------------------------------
CEeg8::~CEeg8()
{
    RemoveAllFilters();
}
//----------------------------------------------------------------------------------
bool CEeg8::Start(bool waitForConnection, int rate, bool protectedMode, const char *host, int port)
{
    if (m_Started || m_ConnectionStage == CS_CONNECTED)
        return true;
    else if (m_ConnectionStage == CS_CONNECTING)
        return false;

    m_ConnectionStage = CS_NONE;
    m_Started = false;
    m_Recording = false;
    m_RecordPaused = false;
    m_TranslationPaused = false;
    m_Rate = rate;

    switch (m_Rate)
    {
        case 250:
        {
            m_DataSize = 765;
            break;
        }
        case 500:
        {
            m_DataSize = 1365;
            break;
        }
        case 1000:
        {
            m_DataSize = 2565;
            break;
        }
        default:
            break;
    }

    WRITE_FILE_BUFFER_SIZE = m_DataSize * 10 * 30; //10 - count of records per second, 30 seconds record
    m_ProtectedMode = protectedMode;
    m_Host = host;
    m_Port = port;
    m_NTPMessage = "";
    m_RecvBuffer.clear();
    m_SendBuffer.clear();

    if (m_Thread.joinable())
        m_Thread.join();

    m_Thread = thread([](CEeg8 *eeg){ qDebug() << "thread start"; eeg->SocketThreadFunction(0); qDebug() << "thread end"; }, this);

    while (waitForConnection && m_ConnectionStage < CS_CONNECTED)
        Sleep(1);

    if (waitForConnection)
        return (m_ConnectionStage == CS_CONNECTED);

    return true;
}
//----------------------------------------------------------------------------------
void CEeg8::Stop()
{
    if (!m_Started)
        return;

    StopRecord();

    m_Started = false;
    m_TranslationPaused = false;

    if (m_Thread.joinable())
        m_Thread.join();

    m_ConnectionStage = CS_NONE;
}
//----------------------------------------------------------------------------------
string fixEdfString(string str, const uint &maxLength)
{
    if (str.length() > maxLength)
        str.resize(maxLength);
    else
    {
        while (str.length() < maxLength)
            str.append(" ");
    }

    return str;
}
//----------------------------------------------------------------------------------
bool CEeg8::StartRecord(const char *userName, const char *filePath)
{
    if (!m_Started || m_Recording || m_TranslationPaused || m_File != nullptr)
        return false;
    else if (!m_HeaderData.size())
    {
        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_HEADER_NOT_FOUND);

        return false;
    }

    if (filePath != nullptr)
        m_RecordFileName = filePath;
    else
    {
        char buff[MAX_PATH] = { 0 };
        GetCurrentDirectoryA(MAX_PATH, &buff[0]);

        std::string directory = buff;
        directory += "/SaveData";
        CreateDirectoryA(directory.c_str(), nullptr);
        directory = directory.c_str();

        SYSTEMTIME st;
        GetLocalTime(&st);

        sprintf(buff, "/EegRecord_%i.%i.%i___%i.%i.%i.bdf", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        m_RecordFileName = directory + buff;
    }

    if (m_FileWriteBuffer != nullptr)
    {
        delete[] m_FileWriteBuffer;
        m_FileWriteBuffer = nullptr;
    }

    qDebug() << "start recording in" << m_RecordFileName.c_str();

    m_File = fopen(m_RecordFileName.c_str(), "wb");

    if (m_File != nullptr)
    {
        vector<char> header = m_HeaderData;

        string name = fixEdfString(userName, 80);
        memcpy(&header[8], &name[0], 80);

        for (int i = 0; i < (int)m_ChannelNames.size(); i++)
        {
            string electrodeName = fixEdfString(m_ChannelNames[i], 16);
            memcpy(&header[256 + (16 * i)], &electrodeName[0], 16);
        }

        fwrite(&header[0], header.size(), 1, m_File);

        m_FileWriteBuffer = new char[WRITE_FILE_BUFFER_SIZE];

        m_CurrentWriteBufferDataSize = 0;
        m_WrittenDataCount = 0;
        m_Recording = true;
        m_RecordPaused = false;

        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_NO_ERROR);
    }
    else
    {
        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_CREATE_FILE_ERROR);
    }

    return m_Recording;
}
//----------------------------------------------------------------------------------
void CEeg8::StopRecord()
{
    if (!m_Started || !m_Recording || m_TranslationPaused || m_File == nullptr)
        return;

    if (m_CurrentWriteBufferDataSize > 0 && m_FileWriteBuffer != nullptr)
    {
        fwrite(&m_FileWriteBuffer[0], m_CurrentWriteBufferDataSize, 1, m_File);
    }

    if (m_WrittenDataCount > 0)
    {
        string recordsCountText = fixEdfString(std::to_string(m_WrittenDataCount), 8);
        fseek(m_File, 236, SEEK_SET);
        fwrite(&recordsCountText[0], recordsCountText.length(), 1, m_File);
    }

    if (m_FileWriteBuffer != nullptr)
    {
        delete[] m_FileWriteBuffer;
        m_FileWriteBuffer = nullptr;
    }

    fflush(m_File);
    fclose(m_File);
    m_CurrentWriteBufferDataSize = 0;
    m_WrittenDataCount = 0;
    m_File = nullptr;
    m_Recording = false;
    m_RecordPaused = false;

    if (m_Callback_OnRecordingStateChanged != nullptr)
        m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_RECORD_STOPPED);
}
//----------------------------------------------------------------------------------
void CEeg8::PauseRecord()
{
    if (m_Started && m_Recording)
    {
        m_RecordPaused = true;

        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_RECORD_PAUSED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::ResumeRecord()
{
    if (m_Started && m_Recording)
    {
        m_RecordPaused = false;

        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(m_CallbackUserData_OnRecordingStateChanged, DRS_RECORD_RESUMED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::StartDataTranslation()
{
    if (m_Started && !m_Recording && m_TranslationPaused)
    {
        if (m_ProtectedMode)
            SendPacket("start -protect eeg.rate " + std::to_string(m_Rate) + "\r\n");
        else
            SendPacket("start eeg.rate " + std::to_string(m_Rate) + "\r\n");

        m_TranslationPaused = false;

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_TRANSLATION_RESUMED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::StopDataTranslation()
{
    if (m_Started && !m_Recording && !m_TranslationPaused)
    {
        SendPacket("stop\r\n");
        m_TranslationPaused = true;

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_TRANSLATION_PAUSED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::SetRxThreshold(int value)
{
    if (m_Started)
    {
        if (value < 10)
            value = 10;
        else if (value > 300)
            value = 300;

        SendPacket("set rx.ths" + std::to_string(value) + "\r\n");
    }
}
//----------------------------------------------------------------------------------
void CEeg8::StartIndicationTest()
{
    if (m_Started)
        SendPacket("device indication test ON\r\n");
}
//----------------------------------------------------------------------------------
void CEeg8::StopIndicationTest()
{
    if (m_Started)
        SendPacket("device indication test OFF\r\n");
}
//----------------------------------------------------------------------------------
void CEeg8::PowerOff()
{
    if (m_Started)
        SendPacket("device powerdown\r\n");
}
//----------------------------------------------------------------------------------
void CEeg8::SynchronizationWithNTP()
{
    if (m_Started)
        SendPacket("sync\r\n");
}
//----------------------------------------------------------------------------------
const CAbstractFilter** CEeg8::GetFilters()
{
    if (m_Filters.empty())
        return nullptr;

    return (const CAbstractFilter**)&m_Filters[0];
}
//----------------------------------------------------------------------------------
const CAbstractFilter* CEeg8::AddFilter(int type, int order)
{
    const int channelsCount = 8;
    const int channelsList[channelsCount] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    return AddFilter(type, order, channelsCount, channelsList);
}
//----------------------------------------------------------------------------------
const CAbstractFilter* CEeg8::AddFilter(int type, int order, int channelsCount, const int *channelsList)
{
    CBaseFilter *filter = nullptr;

    //for (int i = 1; i <= 52; i++) qDebug("case %i: filter = CButterworthFilter<1, %i>::Create(order, channelsList); break;", i, i);

    if (type == FT_BUTTERWORTH)
    {
        switch (channelsCount)
        {
            case 1: filter = CButterworthFilter<1, 1>::Create(order, channelsList); break;
            case 2: filter = CButterworthFilter<1, 2>::Create(order, channelsList); break;
            case 3: filter = CButterworthFilter<1, 3>::Create(order, channelsList); break;
            case 4: filter = CButterworthFilter<1, 4>::Create(order, channelsList); break;
            case 5: filter = CButterworthFilter<1, 5>::Create(order, channelsList); break;
            case 6: filter = CButterworthFilter<1, 6>::Create(order, channelsList); break;
            case 7: filter = CButterworthFilter<1, 7>::Create(order, channelsList); break;
            case 8: filter = CButterworthFilter<1, 8>::Create(order, channelsList); break;
            case 9: filter = CButterworthFilter<1, 9>::Create(order, channelsList); break;
            case 10: filter = CButterworthFilter<1, 10>::Create(order, channelsList); break;
            case 11: filter = CButterworthFilter<1, 11>::Create(order, channelsList); break;
            case 12: filter = CButterworthFilter<1, 12>::Create(order, channelsList); break;
            case 13: filter = CButterworthFilter<1, 13>::Create(order, channelsList); break;
            case 14: filter = CButterworthFilter<1, 14>::Create(order, channelsList); break;
            case 15: filter = CButterworthFilter<1, 15>::Create(order, channelsList); break;
            case 16: filter = CButterworthFilter<1, 16>::Create(order, channelsList); break;
            case 17: filter = CButterworthFilter<1, 17>::Create(order, channelsList); break;
            case 18: filter = CButterworthFilter<1, 18>::Create(order, channelsList); break;
            case 19: filter = CButterworthFilter<1, 19>::Create(order, channelsList); break;
            case 20: filter = CButterworthFilter<1, 20>::Create(order, channelsList); break;
            case 21: filter = CButterworthFilter<1, 21>::Create(order, channelsList); break;
            case 22: filter = CButterworthFilter<1, 22>::Create(order, channelsList); break;
            case 23: filter = CButterworthFilter<1, 23>::Create(order, channelsList); break;
            case 24: filter = CButterworthFilter<1, 24>::Create(order, channelsList); break;
            case 25: filter = CButterworthFilter<1, 25>::Create(order, channelsList); break;
            case 26: filter = CButterworthFilter<1, 26>::Create(order, channelsList); break;
            case 27: filter = CButterworthFilter<1, 27>::Create(order, channelsList); break;
            case 28: filter = CButterworthFilter<1, 28>::Create(order, channelsList); break;
            case 29: filter = CButterworthFilter<1, 29>::Create(order, channelsList); break;
            case 30: filter = CButterworthFilter<1, 30>::Create(order, channelsList); break;
            case 31: filter = CButterworthFilter<1, 31>::Create(order, channelsList); break;
            case 32: filter = CButterworthFilter<1, 32>::Create(order, channelsList); break;
            case 33: filter = CButterworthFilter<1, 33>::Create(order, channelsList); break;
            case 34: filter = CButterworthFilter<1, 34>::Create(order, channelsList); break;
            case 35: filter = CButterworthFilter<1, 35>::Create(order, channelsList); break;
            case 36: filter = CButterworthFilter<1, 36>::Create(order, channelsList); break;
            case 37: filter = CButterworthFilter<1, 37>::Create(order, channelsList); break;
            case 38: filter = CButterworthFilter<1, 38>::Create(order, channelsList); break;
            case 39: filter = CButterworthFilter<1, 39>::Create(order, channelsList); break;
            case 40: filter = CButterworthFilter<1, 40>::Create(order, channelsList); break;
            case 41: filter = CButterworthFilter<1, 41>::Create(order, channelsList); break;
            case 42: filter = CButterworthFilter<1, 42>::Create(order, channelsList); break;
            case 43: filter = CButterworthFilter<1, 43>::Create(order, channelsList); break;
            case 44: filter = CButterworthFilter<1, 44>::Create(order, channelsList); break;
            case 45: filter = CButterworthFilter<1, 45>::Create(order, channelsList); break;
            case 46: filter = CButterworthFilter<1, 46>::Create(order, channelsList); break;
            case 47: filter = CButterworthFilter<1, 47>::Create(order, channelsList); break;
            case 48: filter = CButterworthFilter<1, 48>::Create(order, channelsList); break;
            case 49: filter = CButterworthFilter<1, 49>::Create(order, channelsList); break;
            case 50: filter = CButterworthFilter<1, 50>::Create(order, channelsList); break;
            case 51: filter = CButterworthFilter<1, 51>::Create(order, channelsList); break;
            case 52: filter = CButterworthFilter<1, 52>::Create(order, channelsList); break;
            default:
                break;
        }
    }

    if (filter != nullptr)
        m_Filters.push_back(filter);

    return filter;
}
//----------------------------------------------------------------------------------
bool CEeg8::SetupFilter(const CAbstractFilter *filter, int rate, int lowFrequency, int hightFrequency)
{
    if (filter == nullptr)
        return false;

    for (std::vector<CBaseFilter*>::iterator i = m_Filters.begin(); i != m_Filters.end(); ++i)
    {
        if (filter == *i)
        {
            (*i)->Setup(rate, lowFrequency, hightFrequency);
            return true;
        }
    }

    return false;
}
//----------------------------------------------------------------------------------
void CEeg8::RemoveFilter(const CAbstractFilter *filter)
{
    for (std::vector<CBaseFilter*>::iterator i = m_Filters.begin(); i != m_Filters.end(); ++i)
    {
        if (filter == *i)
        {
            m_Filters.erase(i);
            delete filter;
            break;
        }
    }
}
//----------------------------------------------------------------------------------
void CEeg8::RemoveAllFilters()
{
    for (CBaseFilter *filter : m_Filters)
    {
        if (filter != nullptr)
            delete filter;
    }

    m_Filters.clear();
}
//----------------------------------------------------------------------------------
void CEeg8::SetCallback_OnStartStateChanged(void *userData, EEG_ON_START_STATE_CHANGED *callback)
{
    m_CallbackUserData_OnStartStateChanged = userData;
    m_Callback_OnStartStateChanged = callback;
}
//----------------------------------------------------------------------------------
void CEeg8::SetCallback_OnRecordingStateChanged(void *userData, EEG_ON_RECORDING_STATE_CHANGED *callback)
{
    m_CallbackUserData_OnRecordingStateChanged = userData;
    m_Callback_OnRecordingStateChanged = callback;
}
//----------------------------------------------------------------------------------
void CEeg8::SetCallback_ReceivedData(void *userData, EEG_ON_RECEIVED_DATA *callback)
{
    m_CallbackUserData_OnReceivedData = userData;
    m_Callback_OnReceivedData = callback;
}
//----------------------------------------------------------------------------------
void CEeg8::SocketThreadFunction(int depth)
{
    m_ConnectionStage = CS_CONNECTING;

    WSADATA wsaData;
    memset(&wsaData, 0, sizeof(wsaData));

    if (!WSAStartup(MAKEWORD(2, 2), &wsaData))
        m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    else
        m_Socket = INVALID_SOCKET;

    if (m_Socket == INVALID_SOCKET)
    {
        m_ConnectionStage = CS_SOCKET_CREATION_ERROR;
        m_Started = false;

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_CREATE_SOCKET_ERROR);

        return;
    }

    sockaddr_in caddr;
    memset(&caddr, 0, sizeof(sockaddr_in));
    caddr.sin_family = AF_INET;

    struct hostent *he;

    int rt = inet_addr(m_Host.c_str());

    if (rt != -1)
        caddr.sin_addr.s_addr = rt;
    else
    {
        he = gethostbyname(m_Host.c_str());

        if (he == NULL)
        {
            m_Socket = INVALID_SOCKET;
            m_ConnectionStage = CS_HOST_DETECTION_ERROR;

            if (m_Callback_OnStartStateChanged != nullptr)
                m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_HOST_NOT_FOUND);

            return;
        }

        memcpy(&caddr.sin_addr, he->h_addr, he->h_length);
    }

    caddr.sin_port = htons(m_Port);

    auto waitForConnectionFunc = [this, &caddr]()
    {
        int block = 1;
        int error = ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&block);
        error = connect(m_Socket, (struct sockaddr*)&caddr, sizeof(caddr));

        block = 0;
        ioctlsocket(m_Socket, FIONBIO, (u_long FAR*)&block);

        timeval time_out = { 0, 0 };

        time_out.tv_sec = 3;
        time_out.tv_usec = 0;

        fd_set setW;
        fd_set setE;

        FD_ZERO(&setW);
        FD_ZERO(&setE);
        FD_SET(m_Socket, &setW);
        FD_SET(m_Socket, &setE);

        select(0, NULL, &setW, &setE, &time_out);

        if (FD_ISSET(m_Socket, &setW))
            return true;

        return false;
    };

    if (!waitForConnectionFunc())
    {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        m_ConnectionStage = CS_CONNECTION_ERROR;

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_HOST_NOT_REACHED);

        return;
    }

    m_ConnectionStage = CS_CONNECTED;

    if (m_Callback_OnStartStateChanged != nullptr)
        m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_NO_ERROR);

    const int maxSize = 65535;
    vector<char> buf(maxSize + 1);

    m_Started = true;

    if (m_ProtectedMode)
        SendPacket("start -protect eeg.rate " + std::to_string(m_Rate) + "\r\n");
    else
        SendPacket("start eeg.rate " + std::to_string(m_Rate) + "\r\n");

    if (depth)
    {
        Sleep(100);
        return;
    }

    while (m_Started)
    {
        //qDebug() << "sleeping...";

        fd_set rfds;
        struct timeval tv = { 0, 0 };
        FD_ZERO(&rfds);
        FD_SET(m_Socket, &rfds);

        int dataReady = select((int)m_Socket, &rfds, NULL, NULL, &tv);
        bool recvError = false;

        if (dataReady == SOCKET_ERROR)
        {
            qDebug() << "ReadyRead SOCKET_ERROR";

            if (m_Callback_OnStartStateChanged != nullptr)
                m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_RECEIVE_ERROR);

            recvError = true;
        }
        else if (dataReady != 0)
        {
            int size = recv(m_Socket, &buf[0], maxSize, 0);

            if (size > 0)
            {
                buf[size] = 0;
                //qDebug() << "Receiving, size:" << size; // << &buf[0];
                m_RecvBuffer.insert(m_RecvBuffer.end(), buf.begin(), buf.begin() + size);

                DataReceived();
            }
            else
            {
                qDebug() << "Receiving error, size:" << size;

                if (m_Callback_OnStartStateChanged != nullptr)
                    m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_RECEIVE_ERROR);

                recvError = true;
            }
        }

        if (recvError)
        {
            if (!m_EnableAutoreconnection)
                break;

            if (m_Callback_OnStartStateChanged != nullptr)
                m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, CS_START_RECONNECTING);

            Sleep(3000);

            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;

            SocketThreadFunction(1);
        }

        Sleep(10);
    }

    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;
    m_Started = false;
    m_ConnectionStage = CS_NONE;
    m_TranslationPaused = false;

    if (m_Callback_OnStartStateChanged != nullptr)
        m_Callback_OnStartStateChanged(m_CallbackUserData_OnStartStateChanged, DCS_CONNECTION_CLOSED);
}
//----------------------------------------------------------------------------------
GARANT_EEG_PACKET_VALIDATE_TYPE CEeg8::ValidatePacket(unsigned char *buf, int size, const int &wantType)
{
    uint packetID = UnpackUInt32LE(buf);

    if (packetID != 0x55AA55AA)
        return PVT_BAD_ID;

    int packetLength = UnpackUInt16LE(buf + 4);

    if (packetLength != size)
        return PVT_BAD_LENGTH;

    int packetType = buf[6];

    if (packetType != wantType)
        return PVT_BAD_TYPE;

    uint packetCrc = UnpackUInt32LE(buf + size - 4);
    size -= 4;

    const uint crc32Table[16] =
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
            int index = (crc >> 28) & 0x0F;
            crc = (crc << 4) ^ crc32Table[index];
        }
    }

    if (packetCrc != crc)
        return PVT_BAD_CRC32;

    if (!m_IgnoreCounter && ((m_PrevCounter + 1) % 256) != buf[7])
        return PVT_BAD_COUNTER;

    return PVT_VALIDATED;
}
//----------------------------------------------------------------------------------
void CEeg8::DataReceived()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_NTPMessage.empty())
    {
        if ((int)m_RecvBuffer.size() < NTP_MESSAGE_SIZE)
            return;

        qDebug() << "time sync processed";

        m_NTPMessage.append(&m_RecvBuffer[0], NTP_MESSAGE_SIZE);

        qDebug() << m_NTPMessage.c_str();

        if (!m_SendBuffer.empty())
        {
            ::send(m_Socket, &m_SendBuffer[0], m_SendBuffer.size(), 0);
            m_SendBuffer.clear();
        }
    }

    int wantLength = m_HeaderSize;
    int realDataOffset = 0;

    if (m_ProtectedMode)
    {
        realDataOffset = 8;
        wantLength += 12;
    }

    const string validationErrorMessages[] =
    {
        "No error",
        "Error ID",
        "Error Length",
        "Error Type",
        "Error CRC32",
        "Error Counter"
    };

    m_IgnoreCounter = false;

    if ((int)m_HeaderData.size() < m_HeaderSize)
    {
        m_IgnoreCounter = true;

        while ((int)m_RecvBuffer.size() >= wantLength)
        {
            qDebug() << "header processed";

            if (m_ProtectedMode)
            {
                GARANT_EEG_PACKET_VALIDATE_TYPE validateState = ValidatePacket((unsigned char*)&m_RecvBuffer[0], wantLength, PDT_HEADER);

                if (validateState != PVT_VALIDATED)
                {
                    int skippedCount = 0;

                    skippedCount = 1;
                    m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + 1);

                    while ((int)m_RecvBuffer.size() >= wantLength && UnpackUInt32LE((unsigned char*)&m_RecvBuffer[0]) != 0x55AA55AA)
                    {
                        m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + 1);
                        skippedCount++;
                    }

                    if (validateState != PVT_BAD_COUNTER)
                    {
                        if (UnpackUInt32LE((unsigned char*)&m_RecvBuffer[0]) == 0x55AA55AA)
                            qDebug() << "header validation failed! code:" << validateState << "(" << validationErrorMessages[validateState].c_str() << "), skiped" << skippedCount << "bytes to correct ID";
                        else
                            qDebug() << "header validation failed! code:" << validateState << "(" << validationErrorMessages[validateState].c_str() << "), skiped" << skippedCount << "bytes";
                    }

                    continue;
                }
            }

            m_HeaderData.insert(m_HeaderData.end(), m_RecvBuffer.begin() + realDataOffset, m_RecvBuffer.begin() + realDataOffset + m_HeaderSize);
            m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + wantLength);

            break;
        }

        if ((int)m_HeaderData.size() < m_HeaderSize)
            return;
    }

    wantLength = m_DataSize;

    if (m_ProtectedMode)
        wantLength += 12;

    while ((int)m_RecvBuffer.size() >= wantLength)
    {
        if (m_ProtectedMode)
        {
            GARANT_EEG_PACKET_VALIDATE_TYPE validateState = ValidatePacket((unsigned char*)&m_RecvBuffer[0], wantLength, PDT_DATA);

            unsigned char counter = ((unsigned char*)&m_RecvBuffer[0])[7];

            //qDebug() << "protected, counter:" << counter << " prevCounter: " << m_PrevCounter;

            if (validateState != PVT_VALIDATED)
            {
                //unsigned char badType = ((unsigned char*)&m_RecvBuffer[0])[6];

                int skippedCount = 0;

                if (validateState != PVT_BAD_COUNTER)
                {
                    skippedCount = 1;
                    m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + 1);

                    while ((int)m_RecvBuffer.size() >= wantLength && UnpackUInt32LE((unsigned char*)&m_RecvBuffer[0]) != 0x55AA55AA)
                    {
                        m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + 1);
                        skippedCount++;
                    }
                }

                if (UnpackUInt32LE((unsigned char*)&m_RecvBuffer[0]) == 0x55AA55AA)
                    qDebug() << "data validation failed! code:" << validateState << "(" << validationErrorMessages[validateState].c_str() << "), skiped" << skippedCount << "bytes to correct ID, counter:" << counter << " prevCounter: " << m_PrevCounter;
                else
                    qDebug() << "data validation failed! code:" << validateState << "(" << validationErrorMessages[validateState].c_str() << "), skiped" << skippedCount << "bytes, counter:" << counter << " prevCounter: " << m_PrevCounter;

                if (validateState != PVT_BAD_COUNTER)
                {
                    if (!m_PrevData.empty() && m_PrevCounter != counter)
                    {
                        m_PrevCounter = counter;
                        ProcessData((unsigned char*)&m_PrevData[0], m_PrevData.size());

                        m_PrevData.clear();
                        m_PrevData.insert(m_PrevData.end(), m_PrevData.begin(), m_PrevData.begin() + m_PrevData.size());
                    }

                    continue;
                }
            }

            m_PrevCounter = counter;
        }

        ProcessData((unsigned char*)&m_RecvBuffer[0] + realDataOffset, m_DataSize);

        m_RecvBuffer.erase(m_RecvBuffer.begin(), m_RecvBuffer.begin() + wantLength);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::SendPacket(const string &text)
{
    if (m_Socket == INVALID_SOCKET || !m_Started || text.empty())
        return;

    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_ConnectionStage < CS_CONNECTED || m_NTPMessage.empty())
        m_SendBuffer += text;
    else
        ::send(m_Socket, &text[0], text.size(), 0);
}
//----------------------------------------------------------------------------------
void CEeg8::ProcessData(unsigned char *buf, const int &size)
{
    GARANT_EEG_DATA frameData;

    if (m_Rate == 250)
        frameData.DataRecordsCount = 25;
    else if (m_Rate == 500)
        frameData.DataRecordsCount = 50;
    else if (m_Rate == 1000)
        frameData.DataRecordsCount = 100;

    auto unpack24BitValue = [](unsigned char *buf) -> double
    {
        int intValue = ((buf[2] << 16) | (buf[1] << 8) | buf[0]);

        if (intValue >= 8388608)
            intValue -= 16777216;

        return (double)intValue;
    };

    if (m_Recording && !m_RecordPaused && m_File != nullptr)
    {
        if (m_CurrentWriteBufferDataSize + size > WRITE_FILE_BUFFER_SIZE)
        {
            fwrite(&m_FileWriteBuffer[0], m_CurrentWriteBufferDataSize, 1, m_File);
            m_CurrentWriteBufferDataSize = 0;
        }

        memcpy(&m_FileWriteBuffer[m_CurrentWriteBufferDataSize], &buf[0], size);
        m_CurrentWriteBufferDataSize += size;

        m_WrittenDataCount++;
    }

    for (int i = 0; i < 22; i++)
    {
        if (i < 8) //main channels
        {
            unsigned char *ptr = buf + (i * frameData.DataRecordsCount * 3);

            for (int j = 0; j < frameData.DataRecordsCount; j++, ptr += 3)
            {
                //frameData.ChannelsData[j].Value[i] = (int)(unpack24BitValue(ptr) * 0.000447) / 10.0; // из кода matlab

                frameData.RawChannelsData[j].Value[i] = frameData.FilteredChannelsData[j].Value[i] = (unpack24BitValue(ptr) * 0.000447) / 10.0 / 1000.0; // /1000.0 для перевода в микровольты (из милливольт)
            }
        }
        else if (i < 11) //accelerometr
        {
            //uchar *ptr = buf + ((8 * frameData.Channels.size() * 3) + (i - 8) * frameData.Accelerometr.size() * 3);
        }
        else if (i < 21) //rx
        {
            unsigned char *ptr = buf + ((8 * frameData.DataRecordsCount * 3 + 3 * 5 * 3) + (i - 11) * 1 * 3);

            for (int j = 0; j < 1; j++, ptr += 3)
            {
                frameData.ResistanceData.Value[i - 11] = (int)(unpack24BitValue(ptr) * 0.0677) / 10.0;
            }
        }
        else
        {
            //annotations
            //uchar *ptr = buf + ((8 * frameData.Channels.size() * 3 + 3 * frameData.Accelerometr.size() * 3) + 10 * frameData.RX.size() * 3);
        }
    }

    rapidjson::Document doc;
    doc.Parse((char*)(buf + (size - 90)));

    if (doc.IsObject())
    {
        if (doc.HasMember("FW Version"))
        {
            m_FirmwareVersion = doc["FW Version"].GetString();
            //qDebug() << "FW Version:" << m_FWVersion;
        }

        if (doc.HasMember("Battery %"))
        {
            QString batteryCharges = QString(doc["Battery %"].GetString());
            //qDebug() << "Battery %:" << batteryCharges;
            m_BatteryStatus = batteryCharges.toInt();
        }

        if (doc.HasMember("Block's Time"))
        {
            QString time = QString(doc["Block's Time"].GetString());
            frameData.Time = time.toDouble();
            //qDebug() << "Block's Time:" << (quint64)frameData.Time;
        }
    }

    if (m_Callback_OnReceivedData != nullptr)
    {
        if (!m_Filters.empty())
        {
            for (CBaseFilter *filter : m_Filters)
            {
                if (filter == nullptr)
                    continue;

                int channelsCount = filter->ChannelsCount();
                const int *channelsList = filter->ChannelsList();

                if (channelsCount < 1 || channelsList == nullptr)
                    continue;

                bool error = false;
                float *channels[8];
                //QVector< QVector<float> > channels;
                //QVector<float*> channelsPtr;

                for (int i = 0; i < channelsCount; i++)
                {
                    channels[i] = new float[frameData.DataRecordsCount];
                    //channels.push_back(QVector<float>());
                }

                double multiply = 1.0;

                /*double multiply = 1.0;

                int index = ui->comboBoxVoltage->currentIndex();
                if(0 == index)
                {
                    multiply = 1000000.0;
                }
                else if(1 == index)
                {
                    multiply = 1000.0;
                }*/

                for (int i = 0; i < channelsCount; i++)
                {
                    uint channelIndex = channelsList[i] - 1;

                    if (channelIndex >= 8)
                    {
                        error = true;
                        break;
                    }

                    //QVector<float> &channel = channels[i];
                    //channel.reserve(frameData.DataRecordsCount);

                    for (int j = 0; j < frameData.DataRecordsCount; j++)
                    {
                        float value = (float)frameData.RawChannelsData[j].Value[channelIndex];

                        if (abs(value * 1000000) >= 374000)
                            value = 0.0f;
                        else
                            value *= multiply;

                        channels[i][j] = value;
                        //channel.push_back(value);
                    }

                    //channelsPtr.push_back(&channels[i][0]);
                }

                if (error)
                    continue;

                //filter->Process(frameData.DataRecordsCount, &channelsPtr[0]);
                filter->Process(frameData.DataRecordsCount, channels);

                for (int i = 0; i < channelsCount; i++)
                {
                    int channelIndex = channelsList[i] - 1;
                    //QVector<float> &channel = channels[i];

                    for (int j = 0; j < frameData.DataRecordsCount; j++)
                    {
                        //frameData.ChannelsData[j].Value[channelIndex] = (double)channel[j];
                        frameData.FilteredChannelsData[j].Value[channelIndex] = (double)channels[i][j];
                    }
                }

                for (int i = 0; i < channelsCount; i++)
                {
                     delete[] channels[i];
                }
            }
        }

        m_Callback_OnReceivedData(m_CallbackUserData_OnReceivedData, &frameData);
    }
}
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
