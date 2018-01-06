#!/usr/bin/env python3

import os,sys
import argparse
import ork.build.utils as obu
import ork.build.common as obc

from pathlib import Path

parser = argparse.ArgumentParser(description='Install MicroOrk into location')

parser.add_argument('--prefix', type=str, help='destination to install into')

args = vars(parser.parse_args())

print(args)

assert(args["prefix"]!=None)

prefix = Path(args["prefix"])
libdir = prefix/"lib"
incdir = prefix/"include"
stagedir = Path(obu.stage_dir)
stginc = stagedir/"include"
stglib = stagedir/"lib"

print(prefix)
print(libdir)
print(incdir)
print(stagedir)
print(stginc)
print(stglib)

os.system("mkdir -p %s" % prefix)
os.system("mkdir -p %s" % Path(incdir/"ork"))
os.system("mkdir -p %s" % libdir)

libs = obc.recursive_glob(str(stglib),"libork.*.so")
dpars = set()
libs_to_copy = set()
for item in libs:
    item_path = Path(item)
    item_name = item_path.name
    item_rela = item_path.relative_to(stglib)
    item_dest = libdir/item_name
    item_dpar = item_dest.parent
    dpars.add(item_dpar)
    if item_path.is_file():
        libs_to_copy.add(item_rela)
    #print(item_path,item_name,item_dest,item_dpar)

incs = obc.recursive_glob(str(stginc/"ork"),"*")
incs_to_copy = set()
for item in incs:
    item_path = Path(item)
    item_name = item_path.name
    item_rela = item_path.relative_to(stginc/"ork")
    item_dest = incdir/"ork"/item_rela
    item_dpar = item_dest.parent
    dpars.add(item_dpar)
    if item_path.is_file():
        incs_to_copy.add(item_rela)
        #print(item_name, item_rela,item_dest,item_dpar)

############################################
# create dest folders first
############################################

#print(dpars)

for item in dpars:
    os.system("mkdir -p %s" % item)

#sys.exit(0)

############################################
# copy items
############################################

for item in incs_to_copy:
    item_src = stginc/"ork"/item
    item_dst = incdir/"ork"/item
    print(item_src,item_dst)    
    os.system("cp %s %s" % (item_src,item_dst))

for item in libs_to_copy:
    item_src = stglib/item
    item_dst = libdir/item
    print(item_src,item_dst)    
    os.system("cp %s %s" % (item_src,item_dst))
#print(incs_to_copy)
#print(libs_to_copy)


