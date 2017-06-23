#if !defined(RAW_BUF_H)
#define RAW_BUF_H

//#include "CfgDefs.h"

#include "msg.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TRawBuf
{
	friend class TMsgDeleted<TRawBuf>;

	public:
		//---------------------------------------------------------------------
		template <typename T> class TCreator
		{
			public:
				explicit TCreator(unsigned bufSize) : mBufSize(bufSize) {}
				TRawBuf* createMsg() { return new TRawBuf(mBufSize,sizeof(T)); }

			private:
				unsigned mBufSize;
		};
		//---------------------------------------------------------------------

		template <typename T> T* getDataBuf() { return reinterpret_cast<T*>(mBuf); }
		unsigned byteBufSize() const { return mByteBufSize; }
		unsigned byteDataLen() const { return mByteDataLen; }
		unsigned elemSize() const { return mElemSize; }
		//void setElemSize(unsigned);
		unsigned nativeBufSize() const { return byteBufSize()/elemSize(); }
		template <typename T> unsigned bufSize() const { return byteBufSize()/sizeof(T); }
		template <typename T> unsigned dataLen() const { return byteDataLen()/sizeof(T); }
		template <typename T> void setDataLen(unsigned dataLen)  { mByteDataLen = dataLen*sizeof(T); }
		template <typename T> void resizeBuf(unsigned bufSize) { resizeBuf(bufSize, sizeof(T)); }
		TRawBuf& operator=(const TRawBuf& right)
		{
			if((byteBufSize() != right.byteBufSize()) || (elemSize() != right.elemSize()))
				return *this;
			std::memcpy(mBuf,right.mBuf,byteBufSize());
			return *this;
		}

	protected:
		static const size_t BufAlignment = 128;

		TRawBuf(unsigned bufSize, unsigned elemSize) : mElemSize(0), mByteBufSize(0), mByteDataLen(0), mBuf(0) { resizeBuf(bufSize,elemSize); }
		virtual ~TRawBuf() { _aligned_free(mBuf); }
		void resizeBuf(unsigned bufSize, unsigned elemSize)
		{
			if(mByteBufSize != bufSize*elemSize) {
				mByteBufSize = bufSize*elemSize;
				_aligned_free(mBuf);
				mBuf = _aligned_malloc(mByteBufSize,BufAlignment);
				mByteDataLen = 0;
			}
			if(elemSize != mElemSize) {
				mElemSize = elemSize;
			}
		}

		unsigned mElemSize;
		unsigned mByteBufSize;
		unsigned mByteDataLen;
		void*    mBuf;
};

typedef TBaseMsgWrapperPtr TRawBufPtr;

#endif // RAW_BUF_H
