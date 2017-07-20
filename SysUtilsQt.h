#if !defined(SYS_UTILS_QT_H)
#define SYS_UTILS_QT_H

#include "SysUtils.h"

#include <QtCore/QThread>
#include <QtCore/QString>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSystemSemaphore>
#include <QtCore/QSharedMemory>

namespace SysUtils {

static const int ClassNameJustify = 30;
static const int JobNameJustify   = 16;

//-----------------------------------------------------------------------------
inline const char* getThreadId()
{
    return qPrintable(QString("thread 0x%1").arg(int64_t(QThread::currentThread()),16,16,QLatin1Char('0')));
    //return qPrintable(QString("thread 0x%1").arg(int(QThread::currentThread()),8,16,QLatin1Char('0')));
}

//-----------------------------------------------------------------------------
inline QString getElapsedTime(const QElapsedTimer& timer)
{
    const int TimeGroupNum = 3;
    const int TimeGroupLen = 3;

    QString timeStr = QString("%1").arg(timer.nsecsElapsed()/1000,TimeGroupNum*TimeGroupLen,10,QLatin1Char('0'));
    for(int n = TimeGroupNum-1; n; --n)
        timeStr.insert(n*TimeGroupLen,':');
	timeStr += " us";
    return timeStr;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TAppSingleton
{
	public:
        TAppSingleton(QString name) : mAppSem(name + "_SEM", 1), mSharedMem(name + "_MEM"), mIsRunning(false)
		{
			mAppSem.acquire();

			{
                QSharedMemory sharedMem(name + "_MEM");
				sharedMem.attach();
			}
			if(mSharedMem.attach()) {
				mIsRunning = true;
			}
			else {
				mSharedMem.create(1);
				mIsRunning = false;
			}
			mAppSem.release();
		}
		~TAppSingleton() {}
		bool isRunning() const { return mIsRunning; }

	private:
        QSystemSemaphore	mAppSem;
		QSharedMemory		mSharedMem;
        bool				mIsRunning;
};

}

#endif // SYS_UTILS_QT_H


