//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef QUDP_LIB_H
#define QUDP_LIB_H

#include <QtGlobal>

#ifdef __cplusplus
    extern "C" {
#endif

#include "UDP_Defs.h"

#ifdef QUDP_LIB_EXPORT
    #define QUDP_DLL_API Q_DECL_EXPORT
    //#define QUDP_DLL_API __declspec(dllexport)
    //#define QUDP_DLL_API
#else
    #define QUDP_DLL_API Q_DECL_IMPORT
    //#define QUDP_DLL_API __declspec(dllimport)
    //#define QUDP_DLL_API
#endif

namespace UDP_LIB
{
        QUDP_DLL_API UDP_LIB::TStatus init();
        QUDP_DLL_API UDP_LIB::TStatus cleanUp();
        QUDP_DLL_API UDP_LIB::TStatus getStatus();
        QUDP_DLL_API bool isSocketExist(unsigned long hostAddr, unsigned hostPort);
        /*not exist in UDP_LIB */ QUDP_DLL_API bool isDirectionExist(unsigned long hostAddr, unsigned hostPort, UDP_LIB::TDirection dir);
        QUDP_DLL_API UDP_LIB::TStatus createSocket(unsigned long hostAddr, unsigned hostPort, const UDP_LIB::TParams* rxParams, const UDP_LIB::TParams* txParams);
        QUDP_DLL_API UDP_LIB::TStatus submitTransfer(unsigned long hostAddr, unsigned hostPort, UDP_LIB::TDirection dir, UDP_LIB::Transfer& transfer);
        QUDP_DLL_API UDP_LIB::TStatus getTransfer(unsigned long hostAddr, unsigned hostPort, UDP_LIB::TDirection dir, UDP_LIB::Transfer& transfer, unsigned timeout = INFINITE);
        QUDP_DLL_API unsigned getReadyTransferNum(unsigned long hostAddr, unsigned hostPort, UDP_LIB::TDirection dir);
        QUDP_DLL_API UDP_LIB::TStatus tryGetTransfer(unsigned long hostAddr, unsigned hostPort, UDP_LIB::TDirection dir, UDP_LIB::Transfer& transfer);
}

#ifdef __cplusplus
    }
#endif

#endif /* QUDP_LIB_H */

