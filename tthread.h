#if !defined(TTHREAD_H)
#define TTHREAD_H

#include <QThread>
#include <QDebug>

//*****************************************************************************

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TThread : public QThread
{
 Q_OBJECT

	private:
		volatile bool mExit;
		std::wstring mThreadName;
        virtual void run() Q_DECL_OVERRIDE	{ while(!onExec()) { } }

	protected:
        TThread(const std::wstring& threadName) : QThread(), mExit(false), mThreadName(threadName) {}
		~TThread()
		{
			if(!threadExit())
				threadFinish();
		}
		void setThreadExit() { mExit = true; }
		virtual bool onExec() = 0;
        bool threadFinish()
		{
			setThreadExit();
            bool status = wait(5000);
			if(!status) {
				qDebug() << "[ERROR] TThread" << QString::fromStdWString(threadName()) << "bad exit";
			} else {
				qDebug() << "[INFO] TThread" << QString::fromStdWString(threadName()) << "normal exit";
			}
            return status;
		}


	public:
		bool threadExit() const { return mExit; }
		const std::wstring& threadName() const { return mThreadName; }
};

#endif // TTHREAD_H


