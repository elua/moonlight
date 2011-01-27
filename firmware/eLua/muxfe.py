# SCons build script for muxfe
import os, sys, platform
debug = int( ARGUMENTS.get( 'debug', 0 ) )

# Modify this to reflect your Win32 wxWidgets installation directory
BASE_WX = r"c:\software\wxWidgets-2.8.10"

output = "muxfe.exe"

# List of include dirs
include_dirs = "-I. -Imux -Imux/frontend -Irfs_server -Iinc/remotefs -Iinc"
wx_lib_dir = "-L%s/lib/gcc_lib" % BASE_WX
if debug:
  wx_include_dirs = "-I%s/include -I%s/lib/gcc_lib/mswd" % ( BASE_WX, BASE_WX )
else:
  wx_include_dirs = "-I%s/include -I%s/lib/gcc_lib/msw" % ( BASE_WX, BASE_WX )  

# List of source files
source_files = """mux/main.c mux/frontend/main.cpp mux/frontend/MuxFrontendDialog.cpp mux/frontend/listports_win32.c
                  rfs_server/main.c rfs_server/server.c rfs_server/os_io_win32.c src/eluarpc.c
                  rfs_server/serial_win32.c src/remotefs/remotefs.c rfs_server/log.c""" 

# CPP flags
if debug:
  cpp_flags = "-DWIN32 -D__WXDEBUG__ -D__WXMSW__ -D_WINDOWS -g -D_DEBUG -O0 -DMUX_THREAD_MODE -DWIN32_BUILD -DRFS_UDP_TRANSPORT -DRFS_THREAD_MODE -DLOG_IN_TEXT_CTRL"
else:
  cpp_flags = "-DWIN32 -D__WXMSW__ -D_WINDOWS -O2 -DNDEBUG -DMUX_THREAD_MODE -DWIN32_BUILD -DRFS_UDP_TRANSPORT -DRFS_THREAD_MODE -DLOG_IN_TEXT_CTRL"

# LINK flags
lib_path = "%s/lib/gcc_lib" % BASE_WX
if debug:
  prog_libs = "-lwxmsw28d -lwxtiffd -lwxjpegd -lwxpngd -lwxzlibd -lwxregexd -lwxexpatd" 
else:
  prog_libs = "-lwxmsw28 -lwxtiff -lwxjpeg -lwxpng -lwxzlib -lwxregex -lwxexpat"
sys_libs = "-lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm -lshell32 -lcomctl32 -lole32 -loleaut32 -luuid -lrpcrt4 -lwsock32 -lodbc32 -loleacc -ladvapi32 -lsetupapi -lpthreadGC2"

cccom = "gcc -m32 -fdata-sections -ffunction-sections -Wall %s %s -c $SOURCE -o $TARGET" % ( include_dirs + " " + wx_include_dirs, cpp_flags )
cxxcom = "g++ -m32 -fdata-sections -ffunction-sections -Wall %s %s -c $SOURCE -o $TARGET" % ( include_dirs + " " + wx_include_dirs, cpp_flags )
linkcom = "g++ -m32 -Wl,--gc-sections -Wl,-static -o $TARGET $SOURCES %s %s %s" % ( wx_lib_dir, '', prog_libs + " " + sys_libs )
              
# Env for building the program
comp = Environment( CCCOM = cccom,
                    CXXCOM = cxxcom,
                    LINKCOM = linkcom,
                    ENV = os.environ )
Decider( 'MD5' )                  
Default( comp.Program( output, Split( source_files ) ) )
