public class Pascal  {
	private static int binom(int n, int k) {
		if(k == 0) {
			return 1;
		}
		if(n == k) {
			return 1;
		}
		return binom(n-1, k-1) + binom(n-1, k);
	}


	public static void main(String[] args) {
		int maxn = Integer.parseInt(args[0]);;

		for(int n = 0; n < maxn; n = n + 1) {
			for(int k = 0; k < n+1; k = k + 1) {
				System.out.print(binom(n, k) + " ");
			}
			System.out.println();
		}
	}
}