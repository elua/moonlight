import os, sys, platform

flist = "main.cpp cpu_info.cpp usage.cpp nt_service.cpp"
cdefs = "/DWIN32_BUILD"
socklib = 'wsock32'
output = 'pcmonitor.exe'
lib_path = ""
prog_libs = ""
full_files = " " + " ".join( [ "pcmonitor_udp/%s" % name for name in flist.split() ] )
local_include = "pcmonitor_udp"
prog_libs = "pdh wbemuuid psapi advapi32 wsock32 winmm"

# Compiler/linker options
cpp_flags = "/MT /DWIN32 /GR /EHsc /O2 /DNOPCH %s %s" % ( "", cdefs )
ld_flags = "/OPT:REF /MACHINE:X86 /LARGEADDRESSAWARE"

buildprog = Environment( CPPFLAGS = cpp_flags, LINKFLAGS = ld_flags, ENV = os.environ )
Decider( 'MD5' )   
buildprog.Append( CPPPATH = Split( local_include ) )
buildprog.Append( LIBPATH = Split( lib_path ) )
buildprog.Append( LIBS = Split( prog_libs ) )
buildprog.Program( output, Split( full_files ) )