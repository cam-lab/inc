#if !defined(NETADDR_H)
#define NETADDR_H

#include <cstdint>

namespace CfgDefs {

typedef int64_t NetAddrWordType;
struct NetAddrType
{
	NetAddrType(NetAddrWordType nAddr = -1) : netAddr(nAddr) {}
	operator NetAddrWordType() const { return netAddr; }

	NetAddrWordType netAddr;
};

inline bool operator<(const NetAddrType& el1, const NetAddrType& el2) {
	return el1.netAddr < el2.netAddr;
}

typedef NetAddrType TNetAddr;
static TNetAddr NetNoAddr() { return TNetAddr(-1); }

}

#endif // NETADDR_H


