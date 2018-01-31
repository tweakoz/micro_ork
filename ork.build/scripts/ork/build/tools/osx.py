###############################################################################
# Orkid SCONS Build System
# Copyright 2010, Michael T. Mayers
# email: michael@tweakoz.com
# The Orkid Build System is published under the GPL 2.0 license
# see http://www.gnu.org/licenses/gpl-2.0.html
###############################################################################

import glob
import re
import string
import os
import sys
import ork.build.slnprj
import ork.build.localopts as localopts
import ork.build.spawntools
import ork.build.common

from SCons.Script.SConscript import SConsEnvironment

def is_verbose():
    return ("ORKDOTBUILD_VERBOSE" in os.environ)

if is_verbose():
	print("Using Osx Build Env")

###############################################################################
# Python Module Export Declaration
###############################################################################

__all__ = [ "DefaultBuildEnv" ]
__version__ = "1.0"

###############################################################################
# Basic Build Environment
###############################################################################

XcodeDir = localopts.XCODEDIR()
AqsisDir = localopts.AQSISDIR()
Arch = localopts.ARCH()

if is_verbose():
	print("OSX: using arch<%s>" % Arch)
	print("OSX: using xcode<%s>" % XcodeDir)

USE_DEBUG_CXX = False

#############################################

def find_boost_system():
	if "ORKDOTBUILD_PREFIX" in os.environ:
		srch = os.path.join(os.environ["ORKDOTBUILD_PREFIX"],"lib")
		files = ork.build.common.recursive_glob(srch,"libboost_system*.dylib")
		if len(files)==1:
			file = files[0]
			beg = file.find("libboost_system")
			file = file[beg+3:]
			file = file.replace(".dylib","")
			return file
	return "boost_system"

#############################################

class ClangToolChain:
  def __init__(self,env, prj):
    bindir = "%s/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin" % XcodeDir
    c_compiler = "%s/clang"%bindir
    cpp_compiler = "%s/clang++"%bindir
    env.Replace( CXX = cpp_compiler, CC = c_compiler )
    env.Replace( LINK = cpp_compiler )

    boost_sys = find_boost_system()

    prj.AddLibs( ' m c c++ curl wthttp wt wttest' )
    prj.AddLibs( boost_sys )

    prj.CompilerType = 'gcc'
    prj.XCFLG += "-DOSX -arch %s " % Arch
    prj.XCFLG += '-fno-common -fno-strict-aliasing -g -Wno-switch-enum -Wno-deprecated-declarations '
    prj.XCFLG += '-ffast-math '
    prj.XCXXFLG += '-std=c++11 -stdlib=libc++ ' + prj.XCFLG
    prj.XCXXFLG += '-F%s/Contents/Resources/include -Wno-c++11-narrowing ' % AqsisDir
    #######################
    # see answer to:
    # https://stackoverflow.com/questions/35242099/stdterminate-linker-error-on-a-small-clang-project
    #######################
    prj.XLINK = '-L/usr/lib '
    prj.XLINK = '-rpath @executable_path/../lib '
    # todo injectable rpaths
    #prj.XLINK = '-rpath /usr/local/lib ' # homebrew
    #######################
    prj.XLINK += '-std=c++11 -stdlib=libc++ -v -g -F/Library/Frameworks -arch %s '%Arch
    prj.XLINK += '-F/System/Library/Frameworks/Quartz.framework/Frameworks '
#############################################
def DefaultBuildEnv( env, prj ):
	##
	DEFS = ' IX GCC ORK_OSX _DARWIN'
	#if USE_DEBUG_CXX:
	#	DEFS += ' _GLIBCXX_DEBUG '
	#if prj.IsLinux:
	#	DEFS += "LINUX "
	CCFLG = ' '
	CXXFLG = ' '
	LIBS = "m pthread"
	LIBPATH = ' '#/opt/local/lib '
	#if USE_DEBUG_CXX:
	#	LIBPATH += ' /usr/lib/x86_64-linux-gnu/debug '
	LINK = ''
	##
	
	##########################
	#toolchain = MacPortsToolChain(env,prj)
	toolchain = ClangToolChain(env,prj)
	#toolchain = HpcToolChain(env,prj)
	##########################
	
	env.Replace( CPPDEFINES = DEFS.split(" ") )
	env.Replace( CCFLAGS = CCFLG.split(" ") )
	env.Replace( CXXFLAGS = CXXFLG.split(" ") )
#	env.Replace( CPPPATH  = [ ' /opt/local/include' ] )
	env.Replace( LINKFLAGS=LINK.split(" ") )
	env.Replace( LIBS=LIBS.split(" ") )
	env.Replace( LIBPATH=LIBPATH.split(" ") )
	env.Replace( RANLIB = 'ranlib' )	
	env.Append( FRAMEWORKS = [ 'QtGui', 'QtCore', 'OpenGL', 'CoreMIDI', 'CoreAudio', 'AudioUnit', 'AudioToolbox' ] )
	env.Append( FRAMEWORKS = [ 'Carbon', 'Foundation', 'QuartzComposer' ] )
	env.Append( FRAMEWORKS = [ 'ApplicationServices', 'AppKit' ] )
	env.Append( FRAMEWORKS = [ 'MultitouchSupport', 'Cg' ] )

	if "ORKDOTBUILD_PREFIX" in os.environ:
		pfx = os.environ["ORKDOTBUILD_PREFIX"]
		inc = os.path.join(pfx,"include") 
		lib = os.path.join(pfx,"lib")
		if os.path.exists(inc):
			prj.PostIncludePaths += [inc]
		if os.path.exists(lib):
			env.Append( LIBPATH=[lib] )

	env.Replace( AR="libtool" )
	env.Replace( ARFLAGS="-static -c -v -arch_only %s" % Arch )
	env.Replace( ARCOM="$AR $ARFLAGS -o $TARGET $SOURCES" )
	env.Replace( RANLIBCOM="" )

	env.Tool('osxbundle')

	prj.AddLibs( ' bz2' )

	prj.PostIncludePaths += ['/opt/local/include']
