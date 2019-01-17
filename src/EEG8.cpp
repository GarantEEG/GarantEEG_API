/**
@file EEG8.cpp

@brief Класс для управления устройством EEG8

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#include "src/EEG8.h"
#include <windows.h>
#include "../../../components/rapidjson/document.h"
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
}
//----------------------------------------------------------------------------------
CEeg8::~CEeg8()
{
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

    WRITE_FILE_BUFFER_SIZE = m_DataSize * 10 * 30; //30 seconds record
    m_ProtectedMode = protectedMode;
    m_Host = host;
    m_Port = port;
    m_NTPMessage = "";
    m_RecvBuffer.clear();
    m_SendBuffer.clear();

    if (m_Thread.joinable())
        m_Thread.join();

    m_Thread = thread([](CEeg8 *eeg){ qDebug() << "thread start"; eeg->SocketThreadFunction(); qDebug() << "thread end"; }, this);

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
    if (!m_Started || m_Recording || m_File != nullptr)
        return false;
    else if (!m_HeaderData.size())
    {
        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(DRS_HEADER_NOT_FOUND);

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

        SYSTEMTIME st;
        GetLocalTime(&st);

        sprintf_s(buff, "/EegRecord_%i.%i.%i___%i.%i.%i.bdf", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        m_RecordFileName = directory + buff;
    }

    if (m_FileWriteBuffer != nullptr)
    {
        delete[] m_FileWriteBuffer;
        m_FileWriteBuffer = nullptr;
    }

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
            m_Callback_OnRecordingStateChanged(DRS_NO_ERROR);
    }
    else
    {
        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(DRS_CREATE_FILE_ERROR);
    }

    return m_Recording;
}
//----------------------------------------------------------------------------------
void CEeg8::StopRecord()
{
    if (!m_Started || !m_Recording || m_File == nullptr)
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
        m_Callback_OnRecordingStateChanged(DRS_RECORD_STOPPED);
}
//----------------------------------------------------------------------------------
void CEeg8::PauseRecord()
{
    if (m_Started && m_Recording)
    {
        m_RecordPaused = true;

        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(DRS_RECORD_PAUSED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::ResumeRecord()
{
    if (m_Started && m_Recording)
    {
        m_RecordPaused = false;

        if (m_Callback_OnRecordingStateChanged != nullptr)
            m_Callback_OnRecordingStateChanged(DRS_RECORD_RESUMED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::StartDataTranslation()
{
    if (m_Started && !m_Recording)
    {
        if (m_ProtectedMode)
            SendPacket("start -protect eeg.rate " + std::to_string(m_Rate) + "\r\n");
        else
            SendPacket("start eeg.rate " + std::to_string(m_Rate) + "\r\n");
    }
}
//----------------------------------------------------------------------------------
void CEeg8::StopDataTranslation()
{
    if (m_Started && !m_Recording)
    {
        SendPacket("stop\r\n");

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(DCS_TRANSLATION_PAUSED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::SynchronizationWithNTP()
{
    if (m_Started)
    {
        SendPacket("sync\r\n");

        if (m_Callback_OnStartStateChanged != nullptr)
            m_Callback_OnStartStateChanged(DCS_TRANSLATION_RESUMED);
    }
}
//----------------------------------------------------------------------------------
void CEeg8::SocketThreadFunction()
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
            m_Callback_OnStartStateChanged(DCS_CREATE_SOCKET_ERROR);

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
                m_Callback_OnStartStateChanged(DCS_HOST_NOT_FOUND);

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
            m_Callback_OnStartStateChanged(DCS_HOST_NOT_REACHED);

        return;
    }

    m_ConnectionStage = CS_CONNECTED;

    if (m_Callback_OnStartStateChanged != nullptr)
        m_Callback_OnStartStateChanged(DCS_NO_ERROR);

    const int maxSize = 65535;
    vector<char> buf(maxSize + 1);

    m_Started = true;

    if (m_ProtectedMode)
        SendPacket("start -protect eeg.rate " + std::to_string(m_Rate) + "\r\n");
    else
        SendPacket("start eeg.rate " + std::to_string(m_Rate) + "\r\n");

    while (m_Started)
    {
        qDebug() << "sleeping...";

        fd_set rfds;
        struct timeval tv = { 0, 0 };
        FD_ZERO(&rfds);
        FD_SET(m_Socket, &rfds);

        int dataReady = select((int)m_Socket, &rfds, NULL, NULL, &tv);

        if (dataReady == SOCKET_ERROR)
        {
            qDebug() << "ReadyRead SOCKET_ERROR";

            if (m_Callback_OnStartStateChanged != nullptr)
                m_Callback_OnStartStateChanged(DCS_RECEIVE_ERROR);

            break;
        }
        else if (dataReady != 0)
        {
            int size = recv(m_Socket, &buf[0], maxSize, 0);

            if (size > 0)
            {
                buf[size] = 0;
                qDebug() << "Receiving, size:" << size << &buf[0];
                m_RecvBuffer.insert(m_RecvBuffer.end(), buf.begin(), buf.begin() + size);

                DataReceived();
            }
            else
            {
                qDebug() << "Receiving error, size:" << size;

                if (m_Callback_OnStartStateChanged != nullptr)
                    m_Callback_OnStartStateChanged(DCS_RECEIVE_ERROR);

                break;
            }
        }

        Sleep(500);
    }

    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;

    if (m_Callback_OnStartStateChanged != nullptr)
        m_Callback_OnStartStateChanged(DCS_CONNECTION_CLOSED);
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
    m_PrevData.clear();
    m_PrevData.insert(m_PrevData.end(), buf, buf + size);

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

                frameData.ChannelsData[j].Value[i] = (unpack24BitValue(ptr) * 0.000447) / 10.0 / 1000.0; // /1000.0 для перевода в микровольты (из милливольт)
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
}
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
