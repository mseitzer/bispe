public class Fib {
	private static int fib(int i) {
		if(i == 1) {
			return 1;
		}
		if(i == 2) {
			return 1;
		}
		return fib(i-1) + fib(i-2);
	}	

	public static void main(String[] args) {
		int n = Integer.parseInt(args[0]);
		System.out.println(fib(n));
	}
}