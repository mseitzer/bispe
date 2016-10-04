#include <stdlib.h>
#include <stdio.h>

static int fib(int i) {
	if(i == 1) {
		return 1;
	}
	if(i == 2) {
		return 1;
	}
	return fib(i-1) + fib(i-2);
}

int main(int argc, char const *argv[]) {
	int n = atoi(argv[1]);
	printf("%d\n", fib(n));
	return 0;
}