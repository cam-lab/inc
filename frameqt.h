#if !defined(FRAME_QT_H)
#define FRAME_QT_H

#include <QtGui/QImage>

#include "frame.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <QImage::Format Format> class TQtImageFormat
{
	public:
		static void initImage(QImage*);
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <> class TQtImageFormat<QImage::Format_Indexed8>
{
	public:
		typedef quint8 TPixel;
		static void initImage(QImage* img) { img->setColorTable(frameColorTable()); }

	private:
		static QVector<QRgb>& frameColorTable();
		static const int ColorNum = 256;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <> class TQtImageFormat<QImage::Format_RGB32>
{
	public:
		typedef quint32 TPixel;
		static void initImage(QImage*) {}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <> class TQtImageFormat<QImage::Format_ARGB32>
{
	public:
		typedef quint32 TPixel;
		static void initImage(QImage*) {}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <QImage::Format Format, typename T2 = uint16_t> class TQtFrameImpl
{
	friend class TFrame<TQtFrameImpl>;

	public:
		typedef typename TQtImageFormat<Format>::TPixel TPixel;
        typedef          T2                             TMetaDataElem;

        bool operator==(const TQtFrameImpl<Format>& right) { return *mImage == *(right.mImage); }

	private:
		//---------------------------------------------------------------------
		class TCreator
		{
			public:
				TCreator(int width, int height) : mWidth(width), mHeight(height) {}
				TFrame<TQtFrameImpl>* createMsg() {	return new TFrame<TQtFrameImpl>(new TQtFrameImpl(mWidth, mHeight));	}

			private:
				int mWidth;
				int mHeight;
		};

		//---------------------------------------------------------------------
		static void frameCleanUp(void* info) { delete [] reinterpret_cast<uchar*>(info); /* qDebug() << "frame cleanup"; */}

		int width() const { return mImage->width(); }
		int height() const { return mImage->height(); }
                int pixelSize() const { return sizeof(TPixel); }
                int colorCount() const { return mImage->colorCount(); }
		bool resizeImg(int width, int height)
		{
			delete mImage;
			uchar* dataBuf = new uchar [width*height*sizeof(TPixel)];
			mImage = new QImage(dataBuf,width,height,Format,frameCleanUp,dataBuf);
			TQtImageFormat<Format>::initImage(mImage);
			return true;
		}
		QImage* getImage() { return mImage; }
        void* getPixelBuf(int pixelSize) { return (pixelSize == sizeof(typename TQtImageFormat<Format>::TPixel)) ? mImage->bits() : 0; }

		//---
		TQtFrameImpl(int width, int height) : mImage(0) { resizeImg(width, height); }
		~TQtFrameImpl() { delete mImage; /*qDebug() << "~TQtFrameImpl"; */}
		TQtFrameImpl<Format>& operator=(const TQtFrameImpl<Format>&) { qDebug() << "TQtFrameImpl<Format>& operator="; return *this; }

		QImage* mImage;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//typedef TFrame<TQtFrameImpl<QImage::Format_Indexed8>> TScreenFrameGray;
typedef TFrame<TQtFrameImpl<QImage::Format_ARGB32>>   TScreenFrame;

template<int Width, int Height, int Id> class ScreenFrame : public TScreenFrame
{
	public:
        /*WORK*/ typedef TMsgPool<ScreenFrame<Width,Height,Id>,TBaseFrame> TFramePool;
        /*TEST*/ //typedef TMsgPool<ScreenFrame<Width,Height,Id>> TFramePool;

        class TCreator : public TScreenFrame::TCreator
		{
			public:
                explicit TCreator() : TScreenFrame::TCreator(Width,Height) {}
                ScreenFrame<Width,Height,Id>* createMsg() { return static_cast<ScreenFrame<Width,Height,Id>*>(TScreenFrame::TCreator::createMsg()); }
		};
};

#endif // FRAME_QT_H
