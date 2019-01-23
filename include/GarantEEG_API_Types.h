/**
@file GarantEEG_API_Types.h

@brief Типы устройства ЕЕГ

@author Мустакимов Т.Р.
**/

#ifndef GARANT_EEG_API_TYPES_H
#define GARANT_EEG_API_TYPES_H
//----------------------------------------------------------------------------------
//! global namespace
namespace GarantEEG
{
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Main enumarations
////////////////////////////////////////////////////////////////////////////////////
//! Main device types
enum GARANT_EEG_DEVICE_TYPE
{
    //! Base class for working with GarantEEG device
    DT_GARANT = 0,
    //! Class for working with GarantEEG_MAP device
    DT_GARANT_MAP,
    //! Second version of GarandEEG_MAP device (helmet version 2)
    DT_GARANT_MAP_V2
};
//----------------------------------------------------------------------------------
//! Main filter types
enum GARANT_EEG_FILTER_TYPE
{
    //! Unknown filter
    FT_UNKNOWN = 0,
    //! Butterworth frequency filter
    FT_BUTTERWORTH
};
//----------------------------------------------------------------------------------
//! Device connection state
enum GARANT_EEG_DEVICE_CONNECTION_STATE
{
    //! No error, connected
    DCS_NO_ERROR = 0,
    //! Socket creation error
    DCS_CREATE_SOCKET_ERROR,
    //! Host not found
    DCS_HOST_NOT_FOUND,
    //! Host not reached
    DCS_HOST_NOT_REACHED,
    //! An error occurred while receiving
    DCS_RECEIVE_ERROR,
    //! Connection closed
    DCS_CONNECTION_CLOSED = 10000,
    //! Data stream transplation is paused
    DCS_TRANSLATION_PAUSED,
    //! Data stream transplation is resumed
    DCS_TRANSLATION_RESUMED
};
//----------------------------------------------------------------------------------
//! Device reording state
enum GARANT_EEG_DEVICE_RECORDING_STATE
{
    //! No error, record is started
    DRS_NO_ERROR = 0,
    //! File creation error
    DRS_CREATE_FILE_ERROR,
    //! BDF header is not received
    DRS_HEADER_NOT_FOUND,
    //! Record is paused
    DRS_RECORD_PAUSED = 10000,
    //! Record is resumed
    DRS_RECORD_RESUMED,
    //! Record is stopped
    DRS_RECORD_STOPPED
};
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Main structures
////////////////////////////////////////////////////////////////////////////////////
//! Channels data structure
struct GARANT_EEG_CHANNELS_DATA
{
    //! Values for 8 channels
    double Value[8];
};
//----------------------------------------------------------------------------------
//! Resistance data structure
struct GARANT_EEG_RESISTANCE_DATA
{
    //! Values for 8 channels
    double Value[8];

    //! Values for ref/mastoids
    double Ref;

    //! Values for ground
    double Ground;
};
//----------------------------------------------------------------------------------
//! Accelerometer data structure
struct GARANT_EEG_ACCELEROMETR_DATA
{
    //! X-axis value
    double X;

    //! Y-axis value
    double Y;

    //! Z-axis value
    double Z;
};
//----------------------------------------------------------------------------------
//! GarantEEG frame data
struct GARANT_EEG_DATA
{
    //! Timestamp for current packet
    double Time;

    //! Channels data records count
    int DataRecordsCount;

    //! Channels data array (real size is DataRecordsCount)
    GARANT_EEG_CHANNELS_DATA ChannelsData[100];

    //! Resistance data
    GARANT_EEG_RESISTANCE_DATA ResistanceData;

    //!Accelerometer data
    GARANT_EEG_ACCELEROMETR_DATA AccelerometrData[5];

    //! Annotations for current packet
    char Annitations[30];
};
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////
//! Main callback types definition
////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief EEG_ON_START_STATE_CHANGED Emitted on device connection state changed
 * @param State from GARANT_EEG_DEVICE_CONNECTION_STATE
 */
typedef void __cdecl EEG_ON_START_STATE_CHANGED(void* /*userData*/, unsigned int /*state*/);
//----------------------------------------------------------------------------------
/**
 * @brief EEG_ON_RECORDING_STATE_CHANGED Emitted on device recording state changed
 * @param State from GARANT_EEG_DEVICE_RECORDING_STATE
 */
typedef void __cdecl EEG_ON_RECORDING_STATE_CHANGED(void* /*userData*/, unsigned int /*state*/);
//----------------------------------------------------------------------------------
/**
 * @brief EEG_ON_RECEIVED_DATA Emitted on new data received
 * @param eegData EEG data
 */
typedef void __cdecl EEG_ON_RECEIVED_DATA(void* /*userData*/, const GARANT_EEG_DATA* /*eegData*/);
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // GARANT_EEG_API_TYPES_H
//----------------------------------------------------------------------------------
