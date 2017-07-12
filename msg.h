#if !defined(MSG_H)
#define MSG_H

#include "SysUtils.h"
#include "tqueue.h"
#include "netaddr.h"

#define QT_IMPL  0
#define SL_IMPL  1
#define WIN_IMPL 2

#define MSG_SHARED_PTR_IMPL  SL_IMPL
#define MSG_QUEUE_IMPL       SL_IMPL
#if defined(ENA_WIN_API)
    #define MSG_QUEUE_GUARD_IMPL WIN_IMPL
#else
    #define MSG_QUEUE_GUARD_IMPL QT_IMPL
#endif

#if (MSG_SHARED_PTR_IMPL == SL_IMPL)
	#include <memory>
#else
	//#define QT_SHAREDPOINTER_TRACK_POINTERS
	#include <QtCore/QSharedPointer>
	#include <QtCore/QDebug>  /*TEST*/
#endif

//****************************************************************************************
// Msg Deletion Policy
//
// TMsgDeleted    - message deleted when TBaseMsgWrapper deleted
// TMsgNotDeleted - message NOT deleted when TBaseMsgWrapper deleted
//
//****************************************************************************************

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TMsg> class TMsgDeleted
{
	template<typename, template<typename> class, typename, template<typename> class> friend class TMsgWrapper;

	private:
		static void DeleteMsg(TMsg* msg) { delete msg; }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TMsg> class TMsgNotDeleted
{
	template<typename, template<typename> class, typename, template<typename> class> friend class TMsgWrapper;

	private:
		static void DeleteMsg(TMsg*) {}
};

//****************************************************************************************
// Msg Storage Policy (in special pool based at Queue type)
//
// TMsgPoolPolicy   - messages are created and saved in the pool
// TMsgNoPoolPolicy - messages are "free" (no associated pool)
//
//****************************************************************************************

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TWrapper> class TMsgPoolPolicy
{
	template<typename, typename, typename > friend class TMsgPool;

    public:
        //---
        #if (MSG_SHARED_PTR_IMPL == SL_IMPL)
            typedef std::shared_ptr<TWrapper> TBaseMsgWrapperPtr;
        #elif (MSG_SHARED_PTR_IMPL == QT_IMPL)
            typedef QSharedPointer<TWrapper> TBaseMsgWrapperPtr;
        #else
            #error "BAD MSG_SHARED_PTR_IMPL OPTION"
        #endif

        //---
        #if (MSG_QUEUE_GUARD_IMPL == QT_IMPL)
            typedef TQtMutexGuard TGuard;
        #elif (MSG_QUEUE_GUARD_IMPL == WIN_IMPL)
            typedef TWinCsGuard TGuard;
        #else
            #error "BAD MSG_QUEUE_GUARD_IMPL OPTION"
        #endif

        //---
        #if (MSG_QUEUE_IMPL == SL_IMPL)
            typedef TQueue<TBaseMsgWrapperPtr, TQueueSl, TGuard> TMsgWrapperPoolQueue;
        #elif (MSG_QUEUE_IMPL == QT_IMPL)
            typedef TQueue<TBaseMsgWrapperPtr, TQueueQt, TGuard> TMsgWrapperPoolQueue;
        #else
            #error "BAD MSG_QUEUE_IMPL OPTION"
        #endif


        static bool releaseMsg(TBaseMsgWrapperPtr& msgWrapperPtr)
        {
			if(!msgWrapperPtr)
				return false;
            if(msgWrapperPtr->mMsgPool) {
				msgWrapperPtr->clearNetPoints();
                msgWrapperPtr->mMsgPool->put(msgWrapperPtr);
                return true;
            } else {
                return false;
            }
        }

		virtual ~TMsgPoolPolicy() { /* qDebug() << "~MsgPoolPolicy"; */ }
        int msgPoolId() const { return mMsgPoolId; }
		bool msgClone(TBaseMsgWrapperPtr& msgWrapper)
		{
			if(!mMsgPool->get(msgWrapper))
				return false;
			if(msgCloneImpl(msgWrapper)) {
				return true;
			}
			else {
				#if !defined(MSG_SELF_RELEASE)
					releaseMsg(msgWrapper);
				#endif
				return false;
			}
		}

	protected:
		TMsgPoolPolicy() : mMsgPoolId(-1), mMsgPool(0) {}

		void assignPool(int msgPoolId, TMsgWrapperPoolQueue* msgPool, bool* poolDeleted)
		{
			mMsgPoolId   = msgPoolId;
			mMsgPool     = msgPool;
			mPoolDeleted = poolDeleted;
		}

		static TBaseMsgWrapperPtr createPoolWrapper(TWrapper* obj, bool msgSelfRealease)
		{
			if(msgSelfRealease)
				return TBaseMsgWrapperPtr(obj,msgDeletor);
			else
				return TBaseMsgWrapperPtr(obj);
		}

		static TBaseMsgWrapperPtr createPoolWrapper(TWrapper* obj)
		{
			#if defined(MSG_SELF_RELEASE)
				return createPoolWrapper(obj,true);
			#else
				return createPoolWrapper(obj,false);
			#endif
		}

		virtual bool msgCloneImpl(TBaseMsgWrapperPtr) = 0;
		static void msgDeletor(TWrapper* obj)
		{
			if(*(obj->mPoolDeleted)) {
				delete obj;
				//qDebug() << "Deletor: obj deleted";
			} else {
				obj->mMsgPool->put(createPoolWrapper(obj));
				//qDebug() << "Deletor: obj id = " << obj->msgClassId() << "released to pool:" << obj->mMsgPool;
			}
		}

    private:
        int                   mMsgPoolId;
        TMsgWrapperPoolQueue* mMsgPool;
		bool*                 mPoolDeleted;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TWrapper> class TMsgNoPoolPolicy
{
    public:
		typedef TWrapper* TBaseMsgWrapperPtr;
		virtual ~TMsgNoPoolPolicy() {}

	protected:
		virtual bool msgCloneImpl(TBaseMsgWrapperPtr) = 0;
};

//****************************************************************************************
// Routing (Src/Dst) Policy
//****************************************************************************************

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TNoRoutingPolicy
{
	template<typename TWrapper> friend class TMsgPoolPolicy;

	protected:
		void clearNetPoints() {}
		template <typename Tdst, typename Tsrc> void copyNetPoints(Tdst,Tsrc) {}
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TRoutingPolicy
{
	template<typename TWrapper> friend class TMsgPoolPolicy;

	public:
		typedef CfgDefs::TNetAddr TNetAddr;

		TRoutingPolicy() : mNetSrc(CfgDefs::NetNoAddr()), mNetDst(CfgDefs::NetNoAddr()) {}
		~TRoutingPolicy() { /* qDebug() << "~RoutingPolicy"; */ }
		TNetAddr netSrc() const { return mNetSrc; }
		TNetAddr netDst() const { return mNetDst; }
		void setNetSrc(TNetAddr netSrc) { mNetSrc = netSrc; }
		void setNetDst(TNetAddr netDst) { mNetDst = netDst; }
		void setNetPoints(TNetAddr netSrc, TNetAddr netDst) { setNetSrc(netSrc); setNetDst(netDst); }

	protected:
		void clearNetPoints() { setNetPoints(CfgDefs::NetNoAddr(),CfgDefs::NetNoAddr()); }
		template <typename Tdst, typename Tsrc> void copyNetPoints(Tdst dst, Tsrc src) { dst->setNetPoints(src->netSrc(),src->netDst()); }

	private:
		TNetAddr mNetSrc;
		TNetAddr mNetDst;
};


//****************************************************************************************
// TBaseMsgWrapper и TMsgWrapper
//****************************************************************************************

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template
<
	template<typename> class MsgPoolPolicy,
	typename RoutingPolicy = TRoutingPolicy
>
class TBaseMsgWrapper :
						public MsgPoolPolicy<TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>>,
						public RoutingPolicy
{
    public:
		typedef typename MsgPoolPolicy<TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>>::TBaseMsgWrapperPtr TBaseMsgWrapperPtr;

		virtual int msgClassId() const = 0;
		virtual ~TBaseMsgWrapper() { /* qDebug() << "[~BaseMsgWrapper] msgPoolId:" << msgPoolId(); */ }

		template<typename TMsg> static inline TMsg* checkMsg(TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>*);
		template<typename TMsg> static inline TMsg* getMsg(TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>*);

		uint32_t msgId() const { return mMsgId; }
		void setMsgId(uint32_t msgId) { mMsgId = msgId; }

	protected:
		uint32_t mMsgId;
};

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* checkMsg(TBaseMsgWrapper<TMsgPoolPolicy>* msgWrapper);
template<typename TMsg> TMsg* checkMsg(TBaseMsgWrapper<TMsgPoolPolicy>& msgWrapper);

//-----------------------------------------------------------------------------
template
<
    typename TMsg,
	template<typename> class MsgPoolPolicy = TMsgPoolPolicy,
	typename RoutingPolicy                 = TRoutingPolicy,
	template<typename> class MsgDeletor    = TMsgDeleted
>
class TMsgWrapper :
					public TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>,
					public SysUtils::TTypeEnumerator<TMsg>
{
    public:
		typedef typename TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>::TBaseMsgWrapperPtr TBaseMsgWrapperPtr;

        TMsgWrapper(TMsg* msg) : TBaseMsgWrapper<MsgPoolPolicy,RoutingPolicy>(), mMsg(msg) {}
		virtual ~TMsgWrapper() { MsgDeletor<TMsg>::DeleteMsg(mMsg); /* qDebug() << "[~MsgWrapper] msgPoolId:" << msgPoolId() << "classId:" << msgClassId(); */ }
        virtual int msgClassId() const { return SysUtils::TTypeEnumerator<TMsg>::classId(); }
        TMsg* getMsg() const { return mMsg; }

	protected:
		virtual bool msgCloneImpl(TBaseMsgWrapperPtr msgWrapper)
		{
            TMsg* msg = checkMsg<TMsg>(&(*msgWrapper));
			if(msg) {
				copyNetPoints(msgWrapper,this);
                msgWrapper->setMsgId(this->msgId());
				*msg = *mMsg;
				return true;
			}
			else
				return false;
		}

    private:
        TMsg* mMsg;
};

//-----------------------------------------------------------------------------
template<template<typename> class PoolPolicy, typename RoutingPolicy>
template<typename TMsg>
TMsg* TBaseMsgWrapper<PoolPolicy,RoutingPolicy>::checkMsg(TBaseMsgWrapper<PoolPolicy,RoutingPolicy>* msgWrapper)
{
    if(msgWrapper->msgClassId() == TMsgWrapper<TMsg,PoolPolicy,RoutingPolicy>::classId()) {
        return (static_cast<TMsgWrapper<TMsg,PoolPolicy,RoutingPolicy>*>(msgWrapper))->getMsg();
    } else {
        return 0;
    }
}

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* checkMsg(TBaseMsgWrapper<TMsgPoolPolicy>* msgWrapper)
{
    return TBaseMsgWrapper<TMsgPoolPolicy>::checkMsg<TMsg>(msgWrapper);
}

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* checkMsg(TBaseMsgWrapper<TMsgPoolPolicy>& msgWrapper)
{
    return TBaseMsgWrapper<TMsgPoolPolicy>::checkMsg<TMsg>(&msgWrapper);
}

//-----------------------------------------------------------------------------
template<template<typename> class PoolPolicy, typename RoutingPolicy>
template<typename TMsg>
TMsg* TBaseMsgWrapper<PoolPolicy,RoutingPolicy>::getMsg(TBaseMsgWrapper<PoolPolicy,RoutingPolicy>* msgWrapper)
{
	return (static_cast<TMsgWrapper<TMsg,PoolPolicy,RoutingPolicy>*>(msgWrapper))->getMsg();
}

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* getMsg(TBaseMsgWrapper<TMsgPoolPolicy>* msgWrapper)
{
    return TBaseMsgWrapper<TMsgPoolPolicy>::getMsg<TMsg>(msgWrapper);
}

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* getMsg(TBaseMsgWrapper<TMsgPoolPolicy>& msgWrapper)
{
    return TBaseMsgWrapper<TMsgPoolPolicy>::getMsg<TMsg>(&msgWrapper);
}

//****************************************************************************************
// TMsgPool
//****************************************************************************************

typedef TBaseMsgWrapper<TMsgPoolPolicy>::TBaseMsgWrapperPtr   TBaseMsgWrapperPtr;
typedef TBaseMsgWrapper<TMsgPoolPolicy>::TMsgWrapperPoolQueue TMsgWrapperPoolQueue;

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* checkMsg(TBaseMsgWrapperPtr& msgWrapperPtr)
{
	return TBaseMsgWrapper<TMsgPoolPolicy>::checkMsg<TMsg>(&*msgWrapperPtr);
}

//-----------------------------------------------------------------------------
template<typename TMsg> TMsg* getMsg(TBaseMsgWrapperPtr& msgWrapperPtr)
{
	return TBaseMsgWrapper<TMsgPoolPolicy>::getMsg<TMsg>(&*msgWrapperPtr);
}


//-----------------------------------------------------------------------------
/*static bool releaseMsg(TBaseMsgWrapperPtr& msgWrapperPtr)
{
    return TBaseMsgWrapper<TMsgPoolPolicy>::releaseMsg(msgWrapperPtr);
}*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<typename TMsg, typename TMsgBase = TMsg, typename RoutingPolicy = TRoutingPolicy> class TMsgPool : public TMsgWrapperPoolQueue
{
    public:
        typedef typename TMsg::TCreator TMsgCreator;
        typedef /*typename*/ TMsgWrapper<TMsgBase,TMsgPoolPolicy,RoutingPolicy,TMsgDeleted> TMsgPoolWrapper;

    TMsgPool(int poolSize, TMsgCreator msgCreator) :
                                                    TBaseMsgWrapper<TMsgPoolPolicy>::TMsgWrapperPoolQueue(),
													mPoolSize(poolSize),
													mPoolDeleted(false)
    {
        for(int msgPoolId = 0; msgPoolId < poolSize; ++msgPoolId) {
            TMsgPoolWrapper* msgWrapperPtr = new TMsgPoolWrapper(msgCreator.createMsg());
			msgWrapperPtr->assignPool(msgPoolId,this, &mPoolDeleted);
            put(TMsgPoolWrapper::createPoolWrapper(msgWrapperPtr));
        }
    }

	~TMsgPool()
	{
        #if !defined(ENA_FW_QT)
			printf("[~MsgPool] pool handle: %p, poolSize: %d, objects in the pool: %lldd\n",this, mPoolSize, size());
		#else
			qDebug() << "[~MsgPool] " << "pool handle:" << this << "poolSize:" << mPoolSize << "objects in the pool:" << size();
		#endif
		mPoolDeleted = true;
	}
	int poolSize() const { return mPoolSize; }

    private:
        const int mPoolSize;
		bool      mPoolDeleted;
};

#endif // MSG_H
