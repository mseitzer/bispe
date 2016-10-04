#!/usr/bin/env python
#
# helper.py
#
# Copyright (C) 2014-2016	Max Seitzer <maximilian.seitzer@fau.de>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA 02111-1307 USA.
import os, sys, shutil

name = "bispe_km"

def usage():
	print("Usage: ./helper.py [start|stop|copy]")
	sys.exit(-1)

#if os.geteuid() != 0:
#	print("This needs root to run.")
#	usage()

if len(sys.argv) == 2:
	if sys.argv[1] == "start":
		os.system("sudo rmmod " + name)
		os.system("make clean")
		os.system("make")
		os.system("sudo insmod " + name + ".ko")
	elif sys.argv[1] == "stop":
		os.system("sudo rmmod " + name)
	elif sys.argv[1] == "copy":
		os.system("make clean")
		os.system("make")
		os.system("scp bispe_km.ko user@localhost:/home/user/")
		os.system("cd ../frontend && make clean")
		os.system("cd ../frontend && make")
		os.system("cd ../frontend && scp bispe user@localhost:/home/user/")
		os.system("cd ../compiler && make clean")
		os.system("cd ../compiler && make")
		os.system("cd ../compiler && scp compiler user@localhost:/home/user/")
		os.system("cd ../examples && scp hello_world.scll user@localhost:/home/user/")
		os.system("cd ../examples && scp fib.scll user@localhost:/home/user/")
	else:
		usage()
else:
	usage()
