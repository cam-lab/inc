#if !defined(DRV_BUF_H)
#define DRV_BUF_H

//#include "CfgDefs.h"

#include "RawBuf.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename T, int BufSize> class DrvBuf : public TRawBuf
{
    public:
        /*WORK*/ typedef TMsgPool<DrvBuf<T,BufSize>,TRawBuf> TDrvBufPool;
        /*TEST*/ //typedef TMsgPool<DrvBuf<T,BufSize>> TDrvBufPool;

        class TCreator : public TRawBuf::TCreator<T>
        {
            public:
                explicit TCreator() : TRawBuf::TCreator<T>(BufSize) {}
                DrvBuf<T,BufSize>* createMsg() { return static_cast<DrvBuf<T,BufSize>*>(TRawBuf::TCreator<T>::createMsg()); }

        };
};

typedef TBaseMsgWrapperPtr                        TDrvBufPtr;

#endif // DRV_BUF_H


