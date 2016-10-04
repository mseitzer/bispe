#include <stdlib.h>
#include <stdio.h>

int binom(int n, int k) {
	if(k == 0) {
		return 1;
	}

	if(n == k) {
		return 1;
	}
	
	return binom(n-1, k-1) + binom(n-1, k);
}

int main(int argc, char const *argv[]) {
	int n, k;
	int maxn = atoi(argv[1]);

	for(n = 0; n < maxn; n = n + 1) {
		for(k = 0; k < n+1; k = k + 1) {
			printf("%d ", binom(n, k));
		}
		printf("\n");
	}

	return 0;
}