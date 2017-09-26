//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef IP_QPIPE_DEF_H
#define IP_QPIPE_DEF_H

#ifdef __cplusplus
    #include <cstdint>
#else
    #include <stdint.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

namespace IP_QPIPE_LIB
{
    //---
    typedef enum
    {
        TxPipeNotPresent    =  1,  // attempt to read from rx pipe, but tx pipe not present
        Ok                  =  0,  // all ok
        NotInit             = -1,  // not initialized
        PipeExistError      = -3,  // tx/rx pipe exist, attempt to open another one
        CreateError         = -4,  // create error
        DataAccessError     = -5,  // data access error
        AttachError         = -6,  // attach error
        AttachTxParamsError = -7,  // previously created pipe params != current attached pipe params (actual only for 'writer')
        AttachTxExistError  = -8,  // attemt to attach as 'writer' when another one writer exist now
        AttachRxExistError  = -9,  // attemt to attach as 'reader' but number of existed 'readers' over limit
        PipeNotExistError   = -10, // pipe not exist
        RxNotPresentError   = -11, // attempt to send data when 'rxMustBePresent' set to 'true' and no any rx pipe views
        DataParamError      = -12, // bad data size(rx/tx) or bad buf ptr (tx)
        TimeoutError        = -13, // rx
        NoRxDataError       = -14, // "unpossible" error - no really data to read but we have info "data present"
        RxDataLenError      = -15  // chunk data len is zero or less than rx buf len

    } TStatus;

    //---
    typedef enum
    {
        TxError             = -1,
        TxTransfer          =  0,
        TxConnected         =  1,
        TxDisconnected      =  2
    } TTxEvent;


    //---
    struct TPipeInfo
    {
        static const int MaxRxNum = 4;

        uint32_t chunkSize;
        uint32_t chunkNum;
        uint32_t txReady;
        uint32_t rxReady[MaxRxNum];
    };

    //---
    typedef void (*PipeRxNotifyFunc)(unsigned pipeKey, TTxEvent txEvent, int pipeId, const TPipeInfo& pipeInfo);

    //---
    struct TPipeTxParams
    {
        unsigned  pipeKey;
        bool      isCreated;
        TPipeInfo pipeInfo;
    };

    //---
    struct TPipeTxTransfer
    {
        unsigned pipeKey;
        //uint8_t* dataBuf;
        void*    dataBuf;
        uint32_t dataLen;
        bool     rxMustBePresent;

        //--- DEBUG
        uint32_t txBufIdx;
        uint32_t txGblIdx;
    };

    //---
    typedef uint32_t (*TxTransferFunc)(void* obj, uint8_t* dst, uint32_t maxLen);
    struct TPipeTxTransferFuncObj
    {
        unsigned       pipeKey;
        TxTransferFunc transferFunc;
        void*          obj;
        bool           rxMustBePresent;

        //--- DEBUG
        uint32_t       txBufIdx;
        uint32_t       txGblIdx;
        uint32_t       dataLen;
    };


    //---
    struct TPipeRxParams
    {
        unsigned         pipeKey;
        bool             isCreated;
        int              pipeId;
        TPipeInfo        pipeInfo;
        PipeRxNotifyFunc pipeRxNotifyFunc;
    };

    //---
    struct TPipeRxTransfer
    {
        unsigned pipeKey;
        //uint8_t* dataBuf;
        void*    dataBuf;
        uint32_t dataLen;
    };

}

#ifdef __cplusplus
    }
#endif

#endif /* IP_QPIPE_DEF_H */

