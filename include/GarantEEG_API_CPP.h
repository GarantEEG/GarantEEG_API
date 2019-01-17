/**
@file GarantEEG_API_CPP.h

@brief Интерфейс на языке C++ для управления устройством ЕЕГ

@author Мустакимов Т.Р.
**/

#ifndef GARANT_EEG_API_CPP_H
#define GARANT_EEG_API_CPP_H
//----------------------------------------------------------------------------------
#include "GarantEEG_API_Types.h"
//----------------------------------------------------------------------------------
//! global namespace
namespace GarantEEG
{
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Main class interface
////////////////////////////////////////////////////////////////////////////////////
class IGarantEEG
{
public:
    virtual ~IGarantEEG() {}

    virtual void Dispose() = 0;



    virtual GARANT_EEG_DEVICE_TYPE GetType() const = 0;



    virtual bool Start(bool waitForConnection = true, int rate = 500, bool protectedMode = true, const char *host = "192.168.127.125", int port = 12345) = 0;
    virtual void Stop() = 0;
    virtual bool IsStarted() const = 0;

    virtual bool StartRecord(const char *userName, const char *filePath = nullptr) = 0;
    virtual void StopRecord() = 0;
    virtual bool IsRecording() const = 0;

    virtual void PauseRecord() = 0;
    virtual void ResumeRecord() = 0;
    virtual bool IsRecordPaused() const = 0;



    virtual void StartDataTranslation() = 0;
    virtual void StopDataTranslation() = 0;



    virtual void SynchronizationWithNTP() = 0;

    virtual int GetBatteryStatus() const = 0;

    virtual const char *GetFirmwareVersion() const = 0;



    virtual void SetCallback_OnStartStateChanged(EEG_ON_START_STATE_CHANGED *callback) = 0;
    virtual void SetCallback_OnRecordingStateChanged(EEG_ON_RECORDING_STATE_CHANGED *callback) = 0;
    virtual void SetCallback_ReceivedData(EEG_ON_RECEIVED_DATA *callback) = 0;

};
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Exported functions
////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport) IGarantEEG* __cdecl CreateDevice(GARANT_EEG_DEVICE_TYPE type);
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // GARANT_EEG_API_CPP_H
//----------------------------------------------------------------------------------
