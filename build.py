#!/usr/bin/env python
#
# build.py
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

bin_folder = 'bin'
folders = ["compiler", "frontend", "backend"]

def make():
	clean()

	for folder in folders:
		os.system("cd " + folder + " && make bin")

def clean():
	for folder in folders:
		os.system("cd " + folder + " && make clean")


if not os.path.exists(bin_folder):
	os.mkdir(bin_folder)

if len(sys.argv) == 2 and sys.argv[1] == "clean":
	clean()
	if os.path.exists(bin_folder):
		os.rmdir(bin_folder)
else:
	make()
