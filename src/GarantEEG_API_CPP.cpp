/**
@file GarantEEG_API_CPP.h

@brief Интерфейс на языке C++ для управления устройством ЕЕГ

@author Мустакимов Т.Р.
**/
//----------------------------------------------------------------------------------
#include "include/GarantEEG_API_CPP.h"
#include "src/EEG8.h"
//----------------------------------------------------------------------------------
namespace GarantEEG
{
//----------------------------------------------------------------------------------
extern "C" __declspec(dllexport) IGarantEEG* __cdecl CreateDevice(GARANT_EEG_DEVICE_TYPE type)
{
    switch (type)
    {
        case DT_GARANT:
            return new CEeg8();
        default:
            break;
    }

    return nullptr;
}
//----------------------------------------------------------------------------------
} //namespace GarantEEG
//----------------------------------------------------------------------------------
