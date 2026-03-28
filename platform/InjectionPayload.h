#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Macro to handle export/import of DLL functions
#ifdef INJECTIONPAYLOAD_EXPORTS
#define PAYLOAD_API __declspec(dllexport)
#else
#define PAYLOAD_API __declspec(dllimport)
#endif

extern "C" {
    /**
     * The core function that will be called inside the target process.
     * @param enable If true, sets WDA_EXCLUDEFROMCAPTURE. If false, sets WDA_NONE.
     */
    PAYLOAD_API void SetPrivacyShield(bool enable);
}