#!/usr/bin/env python
import sys

def fib(i):
	if i == 1:
		return 1

	if i == 2:
		return 1;
	
	return fib(i-1) + fib(i-2);

def main():
	n = int(sys.argv[1])
	print fib(n)

if __name__ == "__main__":
	main()