import os, sys, platform

flist = "main.c"
rfs_flist = ''
rfs_flist = "main.c server.c log.c"
cdefs = "-DRFS_UDP_TRANSPORT -DRFS_THREAD_MODE -DMUX_STANDALONE_MODE"
socklib = ''
ptlib = ''
if platform.system() == "Windows":
  cdefs = cdefs + " -DWIN32_BUILD"
  rfs_flist = rfs_flist + " os_io_win32.c serial_win32.c"
  exeprefix = "exe"
  socklib = '-lwsock32'
  ptlib = '-lpthreadGC2'
else:
  rfs_flist = rfs_flist + " os_io_posix.c serial_posix.c"
  exeprefix = "elf"
  socklib = ''
  ptlib = '-pthread'

output = "mux.%s" % exeprefix

rfs_full_files = " " + " ".join( [ "rfs_server/%s" % name for name in rfs_flist.split() ] )
full_files = " " + " ".join( [ "mux/%s" % name for name in flist.split() ] ) + rfs_full_files + " src/remotefs/remotefs.c src/eluarpc.c"
local_include = "-Imux -Irfs_server -Iinc -Iinc/remotefs"

# Compiler/linker options
cccom = "gcc -m32 -O0 -g %s -Wall %s -c $SOURCE -o $TARGET" % ( local_include, cdefs )
linkcom = "gcc -m32 -o $TARGET $SOURCES %s %s" % ( socklib, ptlib )

# Env for building the program
comp = Environment( CCCOM = cccom,
                    LINKCOM = linkcom,
                    ENV = os.environ )
Decider( 'MD5' )                  
Default( comp.Program( output, Split( full_files ) ) )
