#include <stdint.h>
#include <stdio.h>

void print_spaces(uint16_t len)
{
	while (len > 0)
	{
		printf(" ");
		len--;
	}
}
uint16_t two_divisiable(uint16_t n)
{
	return (n & 1) == 1;
}

void print_sepline(uint16_t len1, uint16_t len2)
{
	printf("+");
	while (len1 > 0)
	{
		printf("-");
		len1--;
	}
	printf("+");
	while (len2 > 0)
	{
		printf("-");
		len2--;
	}
	puts("+");
}

void print_header(uint16_t len1, uint16_t len2, int16_t align)
{
	print_sepline(len1, len2);
	if (align == -1)
	{
		printf("|%-*s|", len1, " n ");
		printf("%-*s|\n", len2, " n! ");
	}
	else if (align == 1)
	{
		printf("|%*s|", len1, " n ");
		printf("%*s|\n", len2, " n! ");
	}
	else
	{
		printf("|");
		if (two_divisiable(len1 - 1))
			print_spaces((len1 - 1) / 2 + 1);
		else
			print_spaces((len1 - 1) / 2);

		printf("n");
		print_spaces((len1 - 1) / 2);
		printf("|");
		if (two_divisiable(len2 - 2))
			print_spaces((len2 - 2) / 2 + 1);
		else
			print_spaces((len2 - 2) / 2);
		printf("n!");
		print_spaces((len2 - 2) / 2);
		puts("|");
	}
	print_sepline(len1, len2);
}

uint16_t get_len(uint32_t n)
{
	uint16_t len = 1;
	while (n >= 10)
	{
		n /= 10;
		len++;
	}
	return len;
}

void print_line(uint16_t n, uint32_t factorial, uint16_t len1, uint16_t len2, int16_t align)
{
	if (align == -1)
	{
		printf("|% -*i| ", len1, n);
		printf("%-*u|\n", len2 - 1, factorial);
	}
	else if (align == 1)
	{
		printf("|%*i |", len1 - 1, n);
		printf("%*u |\n", len2 - 1, factorial);
	}
	else
	{
		printf("|");
		int len_l = get_len(n);
		int len_r = get_len(factorial);
		if (two_divisiable(len1 - len_l))
			print_spaces((len1 - len_l) / 2 + 1);
		else
			print_spaces((len1 - len_l) / 2);
		printf("%i", n);
		print_spaces((len1 - len_l) / 2);
		printf("|");
		if (two_divisiable(len2 - len_r))
			print_spaces((len2 - len_r) / 2 + 1);
		else
			print_spaces((len2 - len_r) / 2);
		printf("%u", factorial);
		print_spaces((len2 - len_r) / 2);
		puts("|");
	}
}

uint32_t factorial(uint32_t n)
{
	uint32_t mersen_31 = 2147483647;
	uint64_t result = 1;
	for (uint32_t i = 2; i <= n; i++)
	{
		result %= mersen_31;
		result *= i;
	}

	return result % mersen_31;
}

uint32_t get_max_factorial(int32_t n_start, int32_t n_end)
{
	uint32_t result = 0;
	for (int32_t i = n_start; i <= n_end; i++)
	{
		uint32_t curr_result = factorial(i);
		if (curr_result > result)
		{
			result = curr_result;
		}
	}
	return result;
}

void print_main_body(int32_t n_start, int32_t n_end, uint16_t len1, uint16_t len2, int16_t align)
{
	for (int32_t i = n_start; i <= n_end; i++)
		print_line(i, factorial(i), len1, len2, align);
}

int main()
{
	int32_t n_start, n_end;
	int16_t align;

	scanf("%i %i %hi", &n_start, &n_end, &align);

	if (n_start >= 0 && n_end >= 0)
	{
		if (n_start <= n_end)
		{
			uint16_t len1 = get_len(n_end) + 2;
			uint32_t result_for_len = get_max_factorial(n_start, n_end);
			uint16_t len2 = get_len(result_for_len) + 2 > 3 ? get_len(result_for_len) + 2 : 4;
			print_header(len1, len2, align);
			print_main_body(n_start, n_end, len1, len2, align);
			print_sepline(len1, len2);
		}
		else
		{
			uint16_t len1 = get_len(n_start) + 2;
			uint32_t res1 = get_max_factorial(0, n_end);
			uint32_t res2 = get_max_factorial(n_start, 65535);
			uint32_t result_for_len = res1 > res2 ? res1 : res2;
			uint16_t len2 = get_len(result_for_len) + 2 > 3 ? get_len(result_for_len) + 2 : 4;
			print_header(len1, len2, align);
			print_main_body(n_start, 65535, len1, len2, align);
			print_main_body(0, n_end, len1, len2, align);
			print_sepline(len1, len2);
		}
	}
	else if (n_start < 0)
	{
		fprintf(stderr, "n_start must be > 0");
		return 1;
	}
	else
	{
		fprintf(stderr, "n_end must be > 0");
		return 1;
	}

	return 0;
}
