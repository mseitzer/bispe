#!/usr/bin/env python
import sys

def binom(n, k):
	if k == 0:
		return 1

	if n == k:
		return 1
	
	return binom(n-1, k-1) + binom(n-1, k)

def main():
	maxn = int(sys.argv[1])

	for n in xrange(0, maxn):
		for k in xrange(0, n+1):
			print binom(n, k), 
		print ""

if __name__ == "__main__":
	main()