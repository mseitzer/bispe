#include <stdlib.h>
#include <stdio.h>

void print_prime(int p) {
	int i;

	if(p % 2 == 0) {
		return;
	}

	for(i = 3; i * i <= p; i = i + 2) {
		if(p % i == 0) {
			return;
		}
	}
	printf("%d\n", p);
}

int main(int argc, char const *argv[]) {
	int i;
	int max_prime = atoi(argv[1]);

	for(i = 2; i <= max_prime; i = i + 1) {
		print_prime(i);
	}

	return 0;
}