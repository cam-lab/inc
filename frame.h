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
        static uint32_t MetaInfoObjRawLen() { return 4*sizeof(uint32_t) + MetaBufSize(); }

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
        serializer.write(static_cast<uint32_t>(0));                      // [offset:  0] 'magic number'

        //--- frame container data
        serializer.write(static_cast<uint32_t>(framePtr->msgClassId())); // [offset:  1]
        serializer.write(static_cast<uint32_t>(framePtr->netSrc()));     // [offset:  2]
        serializer.write(static_cast<uint32_t>(framePtr->netDst()));     // [offset:  3]
        serializer.write(static_cast<uint32_t>(framePtr->msgId()));      // [offset:  4] host frame num

        //--- frame data
        serializer.write(static_cast<uint32_t>(frame->pixelSize()));     // [offset:  5]
        serializer.write(static_cast<uint32_t>(frame->height()));        // [offset:  6]
        serializer.write(static_cast<uint32_t>(frame->width()));         // [offset:  7]

        //--- frame metainfo
        frame->metaInfo().serialize(serializer);                         // [offset:  8]

        //--- frame pixel buf
        serializer.write(static_cast<uint8_t*>(frame->getPixelBuf()),frame->byteSize());
        return serializer.streamLen();
	} else {
        return 0;
	}
}

#endif // FRAME_H
