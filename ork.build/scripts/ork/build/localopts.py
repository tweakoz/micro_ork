#!python
###############################################################################
# Orkid SCONS Build System
# Copyright 2010, Michael T. Mayers
# email: michael@tweakoz.com
# The Orkid Build System is published under the GPL 2.0 license
# see http://www.gnu.org/licenses/gpl-2.0.html
###############################################################################
# Orkid Build Machine Local Options
# feel free to edit localopts.py, but NOT localopts.py.template
###############################################################################

import os
import imp
import ork.build.common as common

def IsOsx():
	return common.IsOsx

def IsWindows():
	return os.name == "nt"

if IsWindows():
	import win32api

def IsIx():
	return common.IsIx

def is_verbose():
    return ("ORKDOTBUILD_VERBOSE" in os.environ)

#print "os.name<%s>" % os.name

################################################################
__all__ = [ "XCODEDIR", "VST_SDK_DIR", "VST_INST_DIR", "CXX", "AQSISDIR", "ARCH", "ConfigFileName", "ConfigData", "dump" ]
################################################################

if IsOsx():
   os.environ["VST_INST_DIR"]="~/.vst"
   os.environ["XCODEDIR"]="/Applications/Xcode.app"
   os.environ["ARCH"]="x86_64"
   os.environ["CXX"]="clang++"
elif IsIx():
   os.environ["CXX"]="clang++"
   os.environ["STD"]="c++11"
   os.environ["VST_SDK_DIR"]="/sdk/vstsdk2.4"

################################################################

def GetEnv( sect, varname ):
	#print "/////////////////////"
	#print "sect<%s> varname<%s>" % (sect,varname)
	ret = os.environ[varname]
	#if False==os.path.isdir(ret):
	#	print "<localopts.py> Warning: path<%s> <ret %s> does not exist" % (varname,ret) 
	#print "/////////////////////"
	return os.path.normpath(ret)

################################################################

def XCODEDIR():
 return GetEnv( "PATHS", "XCODEDIR" )
def VST_INST_DIR(): 
 return GetEnv( "PATHS", "VST_INST_DIR" )
def VST_SDK_DIR(): 
 return GetEnv( "PATHS", "VST_SDK_DIR" )
def AQSISDIR():
 return GetEnv( "PATHS", "AQSISDIR" )
def ARCH():
 return GetEnv( "CONFIG", "ARCH" )
def CXX():
 return GetEnv("CONFIG","CXX")
def STD():
 return GetEnv("CONFIG","STD")
################################################################

def dump():
    return None
    #    print "XCODEDIR<%s>" % XCODEDIR()
