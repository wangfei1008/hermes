#include "log.h"

void InitialLog(const char* path)
{
    log4cplus_file_configure(reinterpret_cast<const log4cplus_char_t*>(path));
}

void ReleaseLog()
{
	log4cplus_shutdown();
}


