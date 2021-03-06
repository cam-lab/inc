#if !defined(FRAME_H)
#define FRAME_H

#include <cstdint>

#include "msg.h"
#include "rawbuf.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
class TSerializer
{
    public:
        TSerializer(void* streamBuf, uint32_t maxLen) : MaxLen(maxLen), mStream(streamBuf), mStreamLen(0), mBufOverrun(false) {}
        bool isOk() const { return !mBufOverrun; }
        uint32_t streamLen() const { return isOk() ? mStreamLen : 0; }
        void* streamPtr() const { return isOk() ? mStream : 0; }

        //---
        template<typename T> uint32_t write(T var)
        {
            //---
            if(!isOk()) {
                return 0;
            }

            //---
            if((mStreamLen + sizeof(T)) > MaxLen) {
                mBufOverrun = true;
                return 0;
            }
            T* streamBuf = reinterpret_cast<T*>(mStream);
            *streamBuf++ = var;
            mStreamLen += sizeof(T);
            mStream = streamBuf;
            return mStreamLen;
        }

        //---
        template<typename T> uint32_t write(T* array, unsigned arrayLen)
        {
            //---
            if(!isOk()) {
                return 0;
            }

            //---
            const unsigned ArrayByteLen = sizeof(T)*arrayLen;
            if((mStreamLen + ArrayByteLen) > MaxLen) {
                mBufOverrun = true;
                return 0;
            }

            T* streamBuf = reinterpret_cast<T*>(mStream);
            mStreamLen += ArrayByteLen;
            std::memcpy(mStream,array,ArrayByteLen);
            streamBuf += arrayLen;
            mStream = streamBuf;
            return mStreamLen;
        }

    private:
        const uint32_t MaxLen;
        void*          mStream;
        uint32_t       mStreamLen;
        bool           mBufOverrun;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TMetaInfo
{
    public:
        TMetaInfo() : mWriteIdx(0), mAppendInfoSize(0) {}
        void reset() { mWriteIdx = 0; }
        static uint32_t MetaBufSize() { return MetaBufByteSize; }
        virtual uint32_t metaBufSize() const { return MetaBufSize(); }
        virtual uint32_t metaElemSize() const = 0;
        virtual bool write(void* data, uint32_t dataLen) = 0;
        virtual bool read(void* data, uint32_t beginIdx, uint32_t dataLen) = 0;
        uint32_t metaInfoSize() const { return mWriteIdx; }
        uint32_t metaInfoByteSize() const { return mWriteIdx*metaElemSize(); }
        uint32_t metaBufByteSize() const { return MetaBufSize(); }
        uint32_t metaAppendInfoSize() const { return mAppendInfoSize; }
        uint32_t metaAppendInfoByteSize() const { return mAppendInfoSize*metaElemSize(); }
        const void* getMetaInfoBuf() const { return mMetaBuf; }

        //---
        bool serialize(TSerializer& serializer)
        {
            if(!serializer.isOk()) {
                return false;
            }
            serializer.write(MetaBufSize());
            serializer.write(metaElemSize());
            serializer.write(metaInfoByteSize());
            serializer.write(metaAppendInfoByteSize());
            serializer.write(mMetaBuf,MetaBufSize());
            return serializer.isOk();
        }

        //---
        static void* deserialize(void* src, TMetaInfo& obj)
        {
            uint32_t* srcPtr32 = static_cast<uint32_t*>(src);
            if(obj.MetaBufSize() != *srcPtr32++)                    // [offset:  0] MetaBufSize() check
                return 0;
            if(obj.metaElemSize() != *srcPtr32++)                   // [offset:  1] metaElemSize() check
                return 0;
            obj.mWriteIdx       = (*srcPtr32++)/obj.metaElemSize(); // [offset:  2] metaInfoByteSize()
            obj.mAppendInfoSize = (*srcPtr32++)/obj.metaElemSize(); // [offset:  3] metaAppendInfoByteSize()

            std::memcpy(obj.mMetaBuf,srcPtr32,obj.MetaBufSize());   // [offset:  4] mMetaBuf
            srcPtr32 += (obj.MetaBufSize()/sizeof(uint32_t));
            return srcPtr32;
        }

        //---
        TMetaInfo& operator=(const TMetaInfo& right)
        {
            //qDebug() << "TMetaInfo::operator=";
            if(metaElemSize() != right.metaElemSize()) {
                return *this;
            }
            mWriteIdx       = right.mWriteIdx;
            mAppendInfoSize = right.mAppendInfoSize;
            std::memcpy(mMetaBuf,right.mMetaBuf,mWriteIdx*metaElemSize());
            return *this;
        }

        //---
        bool operator==(const TMetaInfo& right)
        {
            //qDebug() << "TMetaInfo::operator==";
            if(metaElemSize() != right.metaElemSize())
                return false;
            if(mWriteIdx != right.mWriteIdx)
                return false;
            return (std::memcmp(mMetaBuf,right.mMetaBuf,mWriteIdx*metaElemSize()) == 0);
        }

        bool operator!=(const TMetaInfo& right) { return !(*this == right); }

    protected:
        static const uint32_t MetaBufByteSize = 1024;

        uint8_t  mMetaBuf[MetaBufByteSize];
        uint32_t mWriteIdx;
        uint32_t mAppendInfoSize;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T> class TMetaInfoImpl : public TMetaInfo
{
    public:
        TMetaInfoImpl() : TMetaInfo() {}
        virtual uint32_t metaBufSize() const { return TMetaInfo::metaBufSize()/sizeof(T); }
        virtual uint32_t metaElemSize() const { return sizeof(T); }

        //---
        virtual bool write(void* data, uint32_t dataLen)
        {
            if(dataLen + mWriteIdx > metaBufSize())
                return false;
            T* bufPtr = reinterpret_cast<T*>(mMetaBuf + mWriteIdx*metaElemSize());
            std::memcpy(bufPtr,data,dataLen*metaElemSize());
            mWriteIdx += dataLen;
            return true;
        }

        //---
        virtual bool read(void* data, uint32_t beginIdx, uint32_t dataLen)
        {
            if((beginIdx + dataLen) > metaInfoSize())
                return false;
            T* bufPtr = reinterpret_cast<T*>(mMetaBuf + beginIdx*metaElemSize());
            std::memcpy(data,bufPtr,dataLen*metaElemSize());
            return true;
        }

        //---
        T operator[](uint32_t index) const { return *(reinterpret_cast<const T*>(mMetaBuf+index*metaElemSize())); }
        T& operator[](uint32_t index) { return *(reinterpret_cast<T*>(mMetaBuf+index*metaElemSize())); }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class QImage;
class TBaseFrame
{
	friend class TMsgDeleted<TBaseFrame>;

	public:
		virtual int width() const = 0;
		virtual int height() const = 0;
        virtual int pixelSize() const = 0;
        virtual int size() const { return width()*height(); }
        virtual int byteSize() const { return size()*pixelSize(); }
        virtual int colorCount() const = 0;
		virtual bool resizeImg(int width, int height) = 0;

		virtual QImage* getImage() = 0;
        template <typename T> typename T::TPixel* getPixelBuf() { return reinterpret_cast<typename T::TPixel*>(getPixelBuf(sizeof(typename T::TPixel))); }
        virtual void* getPixelBuf() = 0;

        //---
        virtual TBaseFrame& operator=(const TBaseFrame&) = 0;
        virtual bool operator==(const TBaseFrame&) = 0;
        bool operator!=(const TBaseFrame& right) { return !(*this == right); }
        virtual TMetaInfo& metaInfo() = 0;
        template <typename T> T& castMetaInfo() { return static_cast<T&>(metaInfo());}

	protected:
		virtual ~TBaseFrame() {}
		virtual void* getPixelBuf(int) = 0;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename TFrameImpl> class TFrame : public TBaseFrame
{
	public:
        typedef typename TFrameImpl::TCreator      TCreator;
        typedef typename TFrameImpl::TPixel        TPixel;
        typedef typename TFrameImpl::TMetaDataElem TMetaDataElem;

		explicit TFrame(TFrameImpl* frameImpl) : mFrameImpl(frameImpl) { /* qDebug() << "Frame"; */}

		virtual int width() const { return mFrameImpl->width(); }
		virtual int height() const { return mFrameImpl->height(); }
        virtual int pixelSize() const { return mFrameImpl->pixelSize(); }
        virtual int colorCount() const { return mFrameImpl->colorCount(); }
		virtual bool resizeImg(int width, int height) {  return mFrameImpl->resizeImg(width, height); }

		virtual QImage* getImage() { return mFrameImpl->getImage(); }
		virtual void* getPixelBuf(int pixelSize) { return mFrameImpl->getPixelBuf(pixelSize); }
        virtual void* getPixelBuf() { return mFrameImpl->getPixelBuf(sizeof(TPixel)); }

        //---
		virtual TBaseFrame& operator=(const TBaseFrame& baseRight)
		{
			const TFrame<TFrameImpl>& derivedRight = static_cast<const TFrame<TFrameImpl>&>(baseRight);
			if((width() != derivedRight.width()) || (height() != derivedRight.height()))
				return *this;
			*mFrameImpl = *derivedRight.mFrameImpl;
            mMetaInfo   = derivedRight.mMetaInfo;
            // not need to use TBaseFrame::operator=(baseRight);
            //qDebug() << "TBaseFrame& TFrame<TFrameImpl>::operator=";
			return *this;
		}

        //---
        virtual bool operator==(const TBaseFrame& baseRight)
        {
            const TFrame<TFrameImpl>& derivedRight = static_cast<const TFrame<TFrameImpl>&>(baseRight);
            if((width() != derivedRight.width()) || (height() != derivedRight.height()))
                return false;
            return (*mFrameImpl == *(derivedRight.mFrameImpl)) && (mMetaInfo == derivedRight.mMetaInfo);
        }

        //---
        virtual TMetaInfo& metaInfo() { return mMetaInfo; }

	protected:
		virtual ~TFrame() { delete mFrameImpl; /* qDebug() << "~Frame"; */ }

        TFrameImpl*                  mFrameImpl;
        TMetaInfoImpl<TMetaDataElem> mMetaInfo;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T, typename T2 = uint16_t> class TRawFrameImpl : public TRawBuf
{
	friend class TFrame<TRawFrameImpl>;

	public:
        typedef /*typename*/ T  TPixel;
        typedef              T2 TMetaDataElem;

	private:
		//---------------------------------------------------------------------
		class TCreator : public TRawBuf::TCreator<TPixel>
		{
			public:
                TCreator(int width, int height) : TRawBuf::TCreator<T>(width*height), mWidth(width), mHeight(height)  {}
				TFrame<TRawFrameImpl>* createMsg() { return new TFrame<TRawFrameImpl>(new TRawFrameImpl(mWidth, mHeight));	}

			private:
				int mWidth;
				int mHeight;
		};

		int width() const { return mWidth; }
		int height() const { return mHeight; }
        int pixelSize() const { return sizeof(TPixel); }
        int colorCount() const { return 0; }
		QImage* getImage() { return 0; }
		bool resizeImg(int width, int height)
		{
			mWidth = width;
			mHeight = height;
			TRawBuf::resizeBuf<TPixel>(mWidth*mHeight);
			return true;
		}
		void* getPixelBuf(int pixelSize) { return (pixelSize == sizeof(TPixel)) ? TRawBuf::getDataBuf<TPixel>() : 0; }

		//---
		TRawFrameImpl(int width, int height) : TRawBuf(width*height,sizeof(TPixel)), mWidth(width), mHeight(height)  {  /* qDebug() << "TRawFrameImpl"; */  }
		~TRawFrameImpl() { /* qDebug() << "~TRawFrameImpl"; */ }
		TRawFrameImpl<T>& operator=(const TRawFrameImpl<T>& right)
		{
			TRawBuf::operator =(right);
			//qDebug() << "TRawFrameImpl<T>& operator=";
			return *this;
		}

		int mWidth;
		int mHeight;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef TFrame<TRawFrameImpl<quint8>> TScreenFrameGray;
typedef TBaseMsgWrapperPtr	          TScreenFramePtr;

template<int Width, int Height, int Id> class ScreenFrameGray : public TScreenFrameGray
{
        public:
        /*WORK*/ typedef TMsgPool<ScreenFrameGray<Width,Height,Id>,TBaseFrame> TFramePool;

        class TCreator : public TScreenFrameGray::TCreator
                {
                    public:
                        explicit TCreator() : TScreenFrameGray::TCreator(Width,Height) {}
                        ScreenFrameGray<Width,Height,Id>* createMsg() { return static_cast<ScreenFrameGray<Width,Height,Id>*>(TScreenFrameGray::TCreator::createMsg()); }
                };
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef TFrame<TRawFrameImpl<quint16>> TRawFrame;
typedef TBaseMsgWrapperPtr	           TRawFramePtr;

template<int Width, int Height, int Id> class RawFrame : public TRawFrame
{
	public:
		typedef TMsgPool<RawFrame<Width,Height,Id>,TBaseFrame> TFramePool;

		class TCreator : public TRawFrame::TCreator
		{
			public:
				explicit TCreator() : TRawFrame::TCreator(Width,Height) {}
				RawFrame<Width,Height,Id>* createMsg() { return static_cast<RawFrame<Width,Height,Id>*>(TRawFrame::TCreator::createMsg()); }
		};
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template<typename T> uint32_t serializeFrame(TRawFramePtr framePtr, uint8_t* dst, uint32_t maxLen)
{
    T* frame;

    if(framePtr && (frame = checkMsg<T>(framePtr))) {
        TSerializer serializer(dst,maxLen);

        //--- test data
        serializer.write(static_cast<uint32_t>(0));                      // [offset:    0] 'magic number'

        //--- frame container data
        serializer.write(static_cast<uint32_t>(framePtr->msgClassId())); // [offset:    1]
        CfgDefs::TNetAddr netSrc = framePtr->netSrc();
        serializer.write(static_cast<uint32_t>(netSrc >> 32));           // [offset:    2]
        serializer.write(static_cast<uint32_t>(netSrc));                 // [offset:    3]
        CfgDefs::TNetAddr netDst = framePtr->netDst();
        serializer.write(static_cast<uint32_t>(netDst >> 32));           // [offset:    4]
        serializer.write(static_cast<uint32_t>(netDst));                 // [offset:    5]
        serializer.write(static_cast<uint32_t>(framePtr->msgId()));      // [offset:    6] usually used as host frame num

        //--- frame data
        serializer.write(static_cast<uint32_t>(frame->pixelSize()));     // [offset:    7]
        serializer.write(static_cast<uint32_t>(frame->height()));        // [offset:    8]
        serializer.write(static_cast<uint32_t>(frame->width()));         // [offset:    9]

        //--- frame metainfo
        frame->metaInfo().serialize(serializer);                         // [offset:   10]

        //--- frame pixel buf
        serializer.write(static_cast<uint8_t*>(frame->getPixelBuf()),frame->byteSize());
        return serializer.streamLen();
	} else {
        return 0;
	}
}

//-----------------------------------------------------------------------------
template<typename T> bool deserializeFrame(TRawFramePtr framePtr, void* src, uint32_t srcLen)
{
    T* frame;
    if(framePtr && (frame = checkMsg<T>(framePtr))) {
        uint32_t* srcPtr32 = static_cast<uint32_t*>(src);

        //---
        if(*srcPtr32++ != 0)                   // [offset:    0] 'magic number' check
            return false;

        //--- frame container data
        ++srcPtr32;                            // [offset:    1] msgClassId - existed in serialized frame but not used

        //---
        uint32_t netSrcHi = *srcPtr32++;       // [offset:    2] netSrc (hi)
        uint32_t netSrcLo = *srcPtr32++;       // [offset:    3] netSrc (lo)
        CfgDefs::TNetAddr netSrc = (static_cast<CfgDefs::TNetAddr>(netSrcHi) << 32) | netSrcLo;
        framePtr->setNetSrc(netSrc);

        //---
        uint32_t netDstHi = *srcPtr32++;       // [offset:    4] netDst (hi)
        uint32_t netDstLo = *srcPtr32++;       // [offset:    5] netDst (lo)
        CfgDefs::TNetAddr netDst = (static_cast<CfgDefs::TNetAddr>(netDstHi) << 32) | netDstLo;
        framePtr->setNetDst(netDst);

        //---
        framePtr->setMsgId(*srcPtr32++);       // [offset:    6] msgId      - usually used as host frame num
        //--- msgPoolId - not existed in serialized frame and not used

        //--- frame compatibility check
        if(static_cast<uint32_t>(frame->pixelSize()) != *srcPtr32++)  // [offset:    7] pixelSize
            return false;
        if(static_cast<uint32_t>(frame->height()) != *srcPtr32++)     // [offset:    8] frameHeight
            return false;
        if(static_cast<uint32_t>(frame->width()) != *srcPtr32++)      // [offset:    9] frameWidth
            return false;

        //--- frame metainfo
        void* pixelBuf = TMetaInfo::deserialize(srcPtr32, frame->metaInfo());
        if(!pixelBuf)
            return false;

        //--- frame data
        std::memcpy(frame->getPixelBuf(),pixelBuf,frame->byteSize());

        //--- size check (optional)
        if(((static_cast<uint8_t*>(pixelBuf) - static_cast<uint8_t*>(src)) + static_cast<uint32_t>(frame->byteSize())) != srcLen)
            return false;
        return true;
    } else {
        return false;
    }
}

#endif // FRAME_H
