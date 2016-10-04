#!/usr/bin/env python
import os, sys, shutil

languages = {
	"scll" : (".scll", ".scle"),
	"java" : (".java", ".class"),
	"c" : (".c", "")
}

compiling = {
	"scll" : "./compiler {}.scll", 
	"java" : "javac {}.java",
	"c" : "gcc -std=c99 -Werror -Wall -o {0} {0}.c",
}

programs = ["fib", "primes", "pascal"]

def make():
	for folder, build in compiling.iteritems():
		for program in programs:
			if folder == "java":
				program = program.title()
			os.system("cd " + folder + " && " + build.format(program))

def clean():
	for folder, ext in languages.iteritems():
		for program in programs:
			if folder == "java":
				program = program.title()
			os.system("cd " + folder + " && rm -f " + program + ext[1])


if len(sys.argv) == 2 and sys.argv[1] == "clean":
	clean()
else:
	make()