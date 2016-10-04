#!/usr/bin/env python
import sys

def print_prime(p):
	if p % 2 == 0: 
		return

	i = 3
	while i*i < p:
		if p % i == 0:
			return
		i += 2
		
	print p

def main():
	max_prime = int(sys.argv[1])
	for i in xrange(2, max_prime):
		print_prime(i)

if __name__ == "__main__":
	main()