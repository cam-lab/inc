//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef IP_QPIPE_LIB_H
#define IP_QPIPE_LIB_H

#include <QtGlobal>

#ifdef __cplusplus
    #include <cstdint>
#else
    #include <stdint.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

#include "ip_qpipe_def.h"

#ifdef IP_QPIPE_LIB_EXPORT
    //#define IP_QPIPE_DLL_API Q_DECL_EXPORT
    //#define IP_QPIPE_DLL_API __declspec(dllexport)
    #define IP_QPIPE_DLL_API
#else
    //#define IP_QPIPE_DLL_API Q_DECL_IMPORT
    //#define IP_QPIPE_DLL_API __declspec(dllimport)
    #define IP_QPIPE_DLL_API
#endif

namespace IP_QPIPE_LIB
{
        IP_QPIPE_DLL_API IP_QPIPE_LIB::TStatus createPipeViewTx(TPipeTxParams& params);
        IP_QPIPE_DLL_API IP_QPIPE_LIB::TStatus createPipeViewRx(TPipeRxParams& params);
        IP_QPIPE_DLL_API IP_QPIPE_LIB::TStatus sendData(TPipeTxTransfer& txTransfer);
        IP_QPIPE_DLL_API IP_QPIPE_LIB::TStatus readData(TPipeRxTransfer& rxTransfer, int timeout = -1);
}

#ifdef __cplusplus
    }
#endif

#endif /* IP_QPIPE_LIB_H */

