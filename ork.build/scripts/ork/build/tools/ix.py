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

from SCons.Script.SConscript import SConsEnvironment

print("Using Ix Build Env")

cxx_comp = localopts.CXX()
cxx_std = localopts.STD()
c_comp = localopts.CXX()

###############################################################################
# Python Module Export Declaration

__all__ = [ "DefaultBuildEnv" ]
__version__ = "1.0"

#############################################

def find_boost_system():
    if "ORKDOTBUILD_PREFIX" in os.environ:
        srch = os.path.join(os.environ["ORKDOTBUILD_PREFIX"],"lib")
        files = ork.build.common.recursive_glob(srch,"libboost_system*.so")
        if len(files)==1:
            file = files[0]
            beg = file.find("libboost_system")
            file = file[beg+3:]
            file = file.replace(".so","")
            return file
    return "boost_system"

###############################################################################
# Basic Build Environment


USE_DEBUG_CXX = False

def DefaultBuildEnv( env, prj ):
    ##
    DEFS = ' IX GCC '
    if USE_DEBUG_CXX:
        DEFS += ' _GLIBCXX_DEBUG '
    if prj.IsLinux:
        DEFS += "LINUX ORK_LINUX"
    CCFLG = ' '
    CXXFLG = ' '
    LIBS = "m rt pthread"
    LIBPATH = ' . '
    if USE_DEBUG_CXX:
        LIBPATH += ' /usr/lib/x86_64-linux-gnu/debug '
    LINK = ''

    boost_sys = find_boost_system()
    #prj.AddLibs( boost_sys )

    ##
    env.Replace( CXX = cxx_comp, CC = c_comp )
    env.Replace( LINK = cxx_comp )
    env.Replace( CPPDEFINES = DEFS.split(" ") )
    env.Replace( CCFLAGS = CCFLG.split(" ") )
    env.Replace( CXXFLAGS = CXXFLG.split(" ") )
    env.Replace( CPPPATH  = [ '.' ] )
    env.Replace( LINKFLAGS=LINK.split(" ") )
    env.Replace( LIBS=LIBS.split(" " ) )
    env.Replace( LIBPATH=LIBPATH.split(" ") )

    if "ORKDOTBUILD_PREFIX" in os.environ:
        pfx = os.environ["ORKDOTBUILD_PREFIX"]
        inc = os.path.join(pfx,"include") 
        lib = os.path.join(pfx,"lib")
        if os.path.exists(inc):
            prj.PostIncludePaths += [inc]
        if os.path.exists(lib):
            env.Append( LIBPATH=[lib] )

    CxFLG = '-ffast-math -mavx -fPIC -fno-common -fno-strict-aliasing -g -Wno-switch-enum -Wno-c++11-narrowing'
    prj.XCCFLG += CxFLG
    prj.XCXXFLG += CxFLG + " --std=%s -fexceptions " % cxx_std

    prj.CompilerType = 'gcc'

    prj.XLINK = '-v -g -Wl,-rpath,/projects/redux/stage/lib'


