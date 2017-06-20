#if !defined(SYS_UTILS_H)
#define SYS_UTILS_H

#include <cmath>
#include <cfloat>

namespace SysUtils {
	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	inline bool isFuzzyEqual(double x1, double x2, double tol = 0.02) {
        if (fabs(x1 - x2) / (x2 + DBL_EPSILON) > tol)
			return false;
		else
			return true;
	}

	//-----------------------------------------------------------------------------
	template<typename T> class TypeTraits
	{
	private:
		template <typename U> struct PointerTraits
		{
			enum { result = false };
		};
		template <typename U> struct PointerTraits<U*>
		{
			enum { result = true };
		};

	public:
		enum { isPointer = PointerTraits<T>::result };
	};


	//****************************************************************************************
	// Enumerator - generate unique classId for class type
	//
	//
	//              The special class (Enumerator family) for this action is need because
	//              classId's must be the same for all TMsgWrappers which are differenced
	//              only by MsgDeletor type
	//****************************************************************************************

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	class TBaseTypeEnumerator
	{
	protected:
		static int generateClassId() { static int classId; return ++classId; }
	};

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	template<typename T> class TTypeEnumerator : public TBaseTypeEnumerator
	{
	public:
		static int classId() { static const int classId = TBaseTypeEnumerator::generateClassId(); return classId; }
	};
}

#endif // SYS_UTILS_H


