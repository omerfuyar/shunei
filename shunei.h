#pragma once

#pragma region Shunei Macros

#pragma endregion Shunei Macros

#pragma region Shunei Declarations

/// @brief
void SHU_Initialize(void);

/// @brief
void SHU_Terminate(void);

#pragma endregion Shunei Declarations

#pragma region Shunei Definitions

#ifdef SHUNEI_IMPLEMENTATION

#ifdef _WIN32
#include "winsock2.h"
#else
#endif

void SHU_Initialize(void)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa); // request Winsock version 2.2
#endif
}

void SHU_Terminate(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

#endif

#pragma endregion Shunei Definitions
