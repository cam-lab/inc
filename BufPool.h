#if !defined(BUF_POOL_H)
#define BUF_POOL_H

#include <QMap>

#include "SysUtils.h"
#include "netaddr.h"
#include "msg.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TBufPool
{
    public:
        template <typename T> void insertPool(T* pool) { mBufPool.insert(poolId<T>(), pool); /*qDebug() << "pool Id:" << poolId<T>() << "pool handle:" << pool; */ }
        template <typename T> T* getPool()
        {
            TBufPoolMap::iterator pool = mBufPool.find(SysUtils::TTypeEnumerator<T>::classId());
            if(pool != mBufPool.end()) {
                return static_cast<T*>(pool.value());
            } else {
                return 0;
            }
        }
        template <typename T> bool getBuf(TBaseMsgWrapperPtr& buf)
        {
            typename T* pool = getPool<T>();
			if(pool) {
				return pool->get(buf);
            } else {
                return false;
            }

        }
        ~TBufPool();
		void bufPoolInfo(); // for test information

    private:
        typedef QMap<int,TMsgWrapperPoolQueue*> TBufPoolMap;
        TBufPoolMap mBufPool;
        template<typename T> static int poolId() { return SysUtils::TTypeEnumerator<T>::classId(); }
};

#endif // BUF_POOL_H


