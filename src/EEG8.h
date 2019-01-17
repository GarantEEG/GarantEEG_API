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
namespace GarantEEG
{
//----------------------------------------------------------------------------------
enum CONNECTION_STAGE
{
    CS_NONE = 0,
    CS_CONNECTING,
    CS_CONNECTED,
    CS_SOCKET_CREATION_ERROR,
    CS_HOST_DETECTION_ERROR,
    CS_CONNECTION_ERROR
};
//----------------------------------------------------------------------------------
enum GARANT_EEG_PACKET_VALIDATE_TYPE
{
    PVT_VALIDATED = 0,
    PVT_BAD_ID,
    PVT_BAD_LENGTH,
    PVT_BAD_TYPE,
    PVT_BAD_CRC32,
    PVT_BAD_COUNTER
};
//----------------------------------------------------------------------------------
enum GARANT_EEG_PACKET_DATA_TYPE
{
    PDT_HEADER = 1,
    PDT_DATA
};
//----------------------------------------------------------------------------------
class CEeg8 : public IGarantEEG
{
protected:
    int m_BatteryStatus = 0;
    string m_FirmwareVersion = "";
    int m_Rate = 500;
    bool m_ProtectedMode = true;
    string m_Host = "192.168.127.125";
    int m_Port = 12345;
    string m_RecordFileName = "";

    CONNECTION_STAGE m_ConnectionStage = CS_NONE;
    bool m_Started = false;
    bool m_Recording = false;
    bool m_RecordPaused = false;
    thread m_Thread;
    string m_NTPMessage = "";
    SOCKET m_Socket;
    std::mutex m_Mutex;

    FILE *m_File = nullptr;
    int m_CurrentWriteBufferDataSize = 0;
    int m_WrittenDataCount = 0;

    const int PROTECTED_MODE_EXTRA_SIZE = 12;

    const int NTP_MESSAGE_SIZE = 40;

    int WRITE_FILE_BUFFER_SIZE = 0;

    int m_HeaderSize = 0;
    int m_DataSize = 0;
    bool m_IgnoreCounter = false;
    int m_PrevCounter = 0;

    vector<string> m_ChannelNames;

    vector<char> m_HeaderData;
    vector<char> m_PrevData;
    vector<char> m_RecvBuffer;
    char *m_FileWriteBuffer = nullptr;
    string m_SendBuffer;

    EEG_ON_START_STATE_CHANGED *m_Callback_OnStartStateChanged = nullptr;
    EEG_ON_RECORDING_STATE_CHANGED *m_Callback_OnRecordingStateChanged = nullptr;
    EEG_ON_RECEIVED_DATA *m_Callback_OnReceivedData = nullptr;



    /**
     * @brief ValidatePacket Функция проверки чексуммы
     * @param buf Массив данных
     * @param size Размер
     * @return GARANT_EEG_PACKET_VALIDATE_TYPE состояние обработки
     */
    GARANT_EEG_PACKET_VALIDATE_TYPE ValidatePacket(unsigned char *buf, int size, const int &wantType);

    /**
     * @brief UnpackUInt32LE Функция преобразования
     * @param buf
     * @return
     */
    inline unsigned int UnpackUInt32LE(unsigned char *buf)
    {
        return (unsigned int)((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    }

    inline unsigned int UnpackUInt16LE(unsigned char *buf)
    {
        return (unsigned int)((buf[1] << 8) | buf[0]);
    }

    inline unsigned int UnpackUInt16BE(unsigned char *buf)
    {
        return (unsigned int)((buf[0] << 8) | buf[1]);
    }

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
    CEeg8();
    virtual ~CEeg8();

    void SocketThreadFunction();



    virtual void Dispose() override { delete this; }



    virtual GARANT_EEG_DEVICE_TYPE GetType() const override { return DT_GARANT; }



    virtual bool Start(bool waitForConnection = true, int rate = 500, bool protectedMode = true, const char *host = "192.168.127.125", int port = 12345) override;
    virtual void Stop() override;
    virtual bool IsStarted() const override { return m_Started; }

    virtual bool StartRecord(const char *userName, const char *filePath = nullptr) override;
    virtual void StopRecord() override;
    virtual bool IsRecording() const override { return m_Recording; }

    virtual void PauseRecord() override;
    virtual void ResumeRecord() override;
    virtual bool IsRecordPaused() const override { return m_RecordPaused; }



    virtual void StartDataTranslation() override;
    virtual void StopDataTranslation() override;



    virtual void SynchronizationWithNTP() override;

    virtual int GetBatteryStatus() const override { return m_BatteryStatus; }

    virtual const char *GetFirmwareVersion() const override { return m_FirmwareVersion.c_str(); }



    virtual void SetCallback_OnStartStateChanged(EEG_ON_START_STATE_CHANGED *callback) override { m_Callback_OnStartStateChanged = callback; }
    virtual void SetCallback_OnRecordingStateChanged(EEG_ON_RECORDING_STATE_CHANGED *callback) override { m_Callback_OnRecordingStateChanged = callback; }
    virtual void SetCallback_ReceivedData(EEG_ON_RECEIVED_DATA *callback) override { m_Callback_OnReceivedData = callback; }
};
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // EEG8_H
//----------------------------------------------------------------------------------
