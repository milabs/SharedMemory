#pragma once

#ifndef WIN32
 #define WIN32
#endif

#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif

#include <tchar.h>
#include <windows.h>

#include <string>
#include <sstream>
#include <iostream>

typedef std::basic_string<TCHAR> tstring;
typedef std::basic_stringstream<TCHAR> tstringstream;

#ifndef SHMEM_MSG_SIZE
 #define SHMEM_MSG_SIZE 0x1000
#endif

#ifndef SHMEM_PIPE_SIZE
 #define SHMEM_PIPE_SIZE 0x100000
#endif

#ifndef SHMEM_PIPE_TIMEOUT
 #define SHMEM_PIPE_TIMEOUT (5*1000)
#endif

#include "CSmMsg.h"
#include "CSmShared.h"
#include "CSmSharedMsg.h"
#include "CSmSharedPipe.h"

#include "CSmSocket.h"
