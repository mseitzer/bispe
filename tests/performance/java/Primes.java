public class Primes  {
	private static void print_prime(int p) {
		if(p % 2 == 0) {
			return;
		}

		for(int i = 3; i * i <= p; i = i + 2) {
			if(p % i == 0) {
				return;
			}
		}
		System.out.println(p);
	}


	public static void main(String[] args) {
		int max_prime = Integer.parseInt(args[0]);
		for(int i = 2; i <= max_prime; i = i + 1) {
			print_prime(i);
		}
	}
}