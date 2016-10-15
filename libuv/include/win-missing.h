#ifndef INC_MISSING_H_
#define INC_MISSING_H_

#undef __STRICT_ANSI__
#include <string.h>

//#include <ntddndis.h>
//#include <naptypes.h>
#include <windows.h>
typedef int MIB_TCP_STATE;

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#ifndef UNLEN
#define UNLEN       256 
#endif

#ifndef VOLUME_NAME_DOS
#define VOLUME_NAME_DOS 0
#endif

#ifndef min
#define min(a,b)	((a) <(b) ? (a) : (b))
#endif

#endif
