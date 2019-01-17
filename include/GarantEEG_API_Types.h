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
} //namespace GarantEEG
//----------------------------------------------------------------------------------
#endif // GARANT_EEG_API_TYPES_H
//----------------------------------------------------------------------------------
