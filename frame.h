#if !defined(FRAME_H)
#define FRAME_H

#include "msg.h"
#include "rawbuf.h"

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

		virtual TBaseFrame& operator=(const TBaseFrame&) = 0;

	protected:
		virtual ~TBaseFrame() {}
		virtual void* getPixelBuf(int) = 0;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename TFrameImpl> class TFrame : public TBaseFrame
{
	public:
		typedef typename TFrameImpl::TCreator TCreator;
		typedef typename TFrameImpl::TPixel TPixel;

		explicit TFrame(TFrameImpl* frameImpl) : mFrameImpl(frameImpl) { /* qDebug() << "Frame"; */}

		virtual int width() const { return mFrameImpl->width(); }
		virtual int height() const { return mFrameImpl->height(); }
                virtual int pixelSize() const { return mFrameImpl->pixelSize(); }
                virtual int colorCount() const { return mFrameImpl->colorCount(); }
		virtual bool resizeImg(int width, int height) {  return mFrameImpl->resizeImg(width, height); }

		virtual QImage* getImage() { return mFrameImpl->getImage(); }
		virtual void* getPixelBuf(int pixelSize) { return mFrameImpl->getPixelBuf(pixelSize); }
                virtual void* getPixelBuf() { return mFrameImpl->getPixelBuf(sizeof(TPixel)); }

		virtual TBaseFrame& operator=(const TBaseFrame& baseRight)
		{
			const TFrame<TFrameImpl>& derivedRight = static_cast<const TFrame<TFrameImpl>&>(baseRight);
			if((width() != derivedRight.width()) || (height() != derivedRight.height()))
				return *this;
			*mFrameImpl = *derivedRight.mFrameImpl;
			//qDebug() << "TBaseFrame& TFrame<TFrameImpl>::operator=";
			return *this;
		}

	protected:
		virtual ~TFrame() { delete mFrameImpl; /* qDebug() << "~Frame"; */ }

		TFrameImpl* mFrameImpl;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T> class TRawFrameImpl : public TRawBuf
{
	friend class TFrame<TRawFrameImpl>;

	public:
        typedef /*typename*/ T TPixel;

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
typedef TBaseMsgWrapperPtr	      TScreenFramePtr;

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
typedef TBaseMsgWrapperPtr	       TRawFramePtr;

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
class TSerializer
{
	public:
		TSerializer(void* streamBuf) : mStream(streamBuf), mStreamLen(0) {}
		template<typename T> void* write(T var)
		{
			T* streamBuf = reinterpret_cast<T*>(mStream);
			*streamBuf++ = var;
			mStreamLen += sizeof(T);
			mStream = streamBuf;
			return mStream;
		}
		template<typename T> void* write(T* array, unsigned arrayLen)
		{
			const unsigned ArrayByteLen = sizeof(T)*arrayLen;
			T* streamBuf = reinterpret_cast<T*>(mStream);
			mStreamLen += ArrayByteLen;
			std::memcpy(mStream,array,ArrayByteLen);
			streamBuf += arrayLen;
			mStream = streamBuf;
			return mStream;
		}

		uint32_t streamLen() const { return mStreamLen; }
		void* streamPtr() const { return mStream; }

	private:
		void*  mStream;
		uint32_t mStreamLen;
};

//-----------------------------------------------------------------------------
template<typename T1, typename T2> bool serializeFrame(TRawFramePtr framePtr, uint8_t* dst, uint32_t* streamLen)
{
    T1* frame;
    typename T2::TPixel* frameBuf;

    if(framePtr && (frame = checkMsg<T1>(framePtr)) && (frameBuf = frame->template getPixelBuf<T2>())) {
		TSerializer serializer(dst);
        const uint32_t ClassId    = 0; // only for start-up
        const uint32_t MsgClassId = framePtr->msgClassId();
        const uint32_t NetSrc     = framePtr->netSrc();
        const uint32_t NetDst     = framePtr->netDst();
        const uint32_t FrameNum   = framePtr->msgId();
        const uint32_t PixelSize  = sizeof(T2::TPixel);
        const uint32_t Height     = frame->height();
        const uint32_t Width      = frame->width();

        serializer.write(ClassId);
        serializer.write(MsgClassId);
        serializer.write(NetSrc);
        serializer.write(NetDst);
        serializer.write(FrameNum);
        serializer.write(PixelSize);
        serializer.write(Height);
        serializer.write(Width);

		serializer.write(frameBuf,frame->size());
		*streamLen = serializer.streamLen();
		return true;
	} else {
		*streamLen  = 0;
		return false;
	}
}

#endif // FRAME_H
