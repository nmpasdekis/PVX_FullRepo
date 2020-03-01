#ifndef _MEMORY_LEAK_DETECT_
#define _MEMORY_LEAK_DETECT_
#ifdef _DEBUG

#ifndef _CRTDBG_MAP_ALLOC 
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>


#ifndef DEBUG_NEW
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


#endif

#define MemoryLeak _CrtDumpMemoryLeaks();
#else
#define MemoryLeak
#endif

#ifdef _DEBUG
// 	_CrtMemState cp;
#define LeakStart() {_CrtMemState cp;_CrtMemCheckpoint(&cp);{
#define LeakEnd() }_CrtMemDumpAllObjectsSince(&cp);}
#else
#define LeakStart()
#define LeakEnd()
#endif



#endif
// 
// _NORMAL_BLOCK