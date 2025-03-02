#include "return_codes.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	uint8_t sign;
	int32_t exp;
	uint32_t frac;
} ieee754_type;

void print_frac(char *type, uint32_t frac)
{
	int32_t move;
	uint32_t block = 0xF;
	move = (!strcmp(type, "f")) ? 19 : 6;
	while (move > 0)
	{
		printf("%x", ((frac >> move) & block));
		move -= 4;
	}
	uint32_t m = (!strcmp(type, "f")) ? 1 : 2;
	block >>= m;
	printf("%x", (frac & block) << m);
}

uint32_t getNum(char *str)
{
	uint32_t res;
	sscanf(str, "%x", &res);
	return res;
}

uint32_t check_type(char *type)
{
	if (strcmp(type, "f") != 0 && strcmp(type, "h") != 0)
	{
		fprintf(stderr, "Invalid type.");
		return 1;
	}
	return 0;
}

uint32_t check_mode(char *mode)
{
	if (strcmp(mode, "0") != 0 && strcmp(mode, "1") != 0 && strcmp(mode, "2") != 0 && strcmp(mode, "3") != 0)
	{
		fprintf(stderr, "Invalid mode.");
		return 1;
	}
	return 0;
}

uint32_t check_op(char *op)
{
	if (strcmp(op, "+") != 0 && strcmp(op, "-") != 0 && strcmp(op, "*") != 0 && strcmp(op, "/") != 0)
	{
		fprintf(stderr, "Unhandled operation.");
		return 1;
	}
	return 0;
}

uint64_t normalize(uint64_t fracC, uint32_t move, int32_t *expR)
{
	while ((fracC >> move) < 1)
	{
		*expR -= 1;
		fracC <<= 1;
	}

	while ((fracC >> move) > 1)
	{
		*expR += 1;
		fracC >>= 1;
	}
	return fracC;
}

uint8_t get_sign(char *type, uint32_t num)
{
	if (!strcmp(type, "f"))
	{
		return num >> 31;
	}
	return num >> 15;
}

void print_minus(char *type, uint32_t num)
{
	if (get_sign(type, num))
	{
		printf("-");
	}
}

int32_t check_zero(int32_t exp, uint32_t frac)
{
	return (!exp && frac == 0);
}

uint32_t get_check(char *type)
{
	if (!strcmp(type, "f"))
	{
		return 0xFF;
	}
	return 0x1F;
}

int32_t check_nan(char *type, int32_t exp, uint32_t frac)
{
	return (exp == get_check(type) && frac);
}

uint8_t check_inf(char *type, uint32_t exp, uint32_t frac)
{
	return (exp == get_check(type) && !frac);
}

uint32_t get_move(char *type)
{
	if (!strcmp(type, "f"))
	{
		return 23;
	}
	return 10;
}

uint32_t get_exp(char *type, uint32_t num)
{
	return (num >> get_move(type)) & get_check(type);
}

uint32_t get_frac(char *type, uint32_t num)
{
	if (!strcmp(type, "f"))
	{
		return (num & 0x7FFFFF);
	}
	return (num & 0x3FF);
}

int32_t get_slide(char *type)
{
	if (!strcmp(type, "f"))
	{
		return 127;
	}
	return 15;
}

uint32_t get_int_one(char *type)
{
	if (!strcmp(type, "f"))
	{
		return 0x800000;
	}
	return 0x400;
}

ieee754_type get_parts(char *type, uint32_t num)
{
	ieee754_type p;
	p.exp = (int32_t)get_exp(type, num);
	p.frac = get_frac(type, num);
	p.sign = get_sign(type, num);
	return p;
}

void get_result(char *type, ieee754_type a)
{
	if (a.exp)
	{
		a.frac |= get_int_one(type);
	}
	else
	{
		a.exp += 1;
	}
	a.frac = normalize(a.frac, get_move(type), &(a.exp));
	a.exp -= get_slide(type);
	print_minus(type, a.sign);
	printf("0x1.");
	a.frac &= get_int_one(type) - 1;
	print_frac(type, a.frac);
	printf("p");
	if (a.exp >= 0)
	{
		printf("+");
	}
	printf("%i", a.exp);
}

void print_zero(char *type, uint8_t sign)
{
	if (sign)
	{
		printf("-");
	}
	if (!strcmp(type, "f"))
	{
		puts("0x0.000000p+0");
	}
	else
	{
		puts("0x0.000p+0");
	}
}

void print_result(char *type, ieee754_type res)
{
	if (res.sign)
	{
		printf("-");
	}
	printf("0x1.");
	print_frac(type, res.frac);
	printf("p");
	if (res.exp >= 0)
	{
		printf("+");
	}
	printf("%i", res.exp);
}

uint64_t round_mode(char *mode, uint64_t fracR, uint32_t signR, uint64_t round, uint64_t check)
{
	if (!strcmp(mode, "0"))
	{
		return fracR;
	}
	if (((!strcmp(mode, "1")) && ((round > check) || (round == check && (fracR & 1)))) ||
		(!strcmp(mode, "2") && (!signR && round)) || (!strcmp(mode, "3") && (signR && round)))
	{
		fracR += 1;
	}

	return fracR;
}

void reform_values(char *type, ieee754_type *a)
{
	uint32_t int_one = get_int_one(type);
	if ((*a).exp)
	{
		(*a).frac |= int_one;
		(*a).exp -= get_slide(type);
	}
	else
	{
		(*a).exp += 1;
		(*a).exp -= get_slide(type);
	}
}

uint32_t check_zero_nan_inf(char *type, ieee754_type parts)
{
	if (check_zero(parts.exp, parts.frac))
	{
		print_zero(type, parts.sign);
		return 1;
	}

	if (check_nan(type, parts.exp, parts.frac))
	{
		puts("nan");
		return 1;
	}

	if (check_inf(type, parts.exp, parts.frac))
	{
		if (parts.sign)
		{
			printf("-");
		}
		puts("inf");
		return 1;
	}

	return 0;
}

uint32_t normalize_for_check(uint32_t frac, int32_t *exp, int32_t slide)
{
	if (*exp < -slide)
	{
		while (*exp < -slide)
		{
			frac >>= 1;
			*exp += 1;
		}
	}
	else if (*exp > slide)
	{
		while (*exp > slide)
		{
			frac <<= 1;
			*exp -= 1;
		}
	}
	return frac;
}

uint8_t form_result(char *type, uint8_t signR, uint32_t fracR, int32_t expR, ieee754_type *result)
{
	int32_t slide = get_slide(type);
	(*result).sign = signR;
	(*result).frac = get_frac(type, normalize_for_check(fracR, &expR, slide));
	(*result).exp = expR + slide;

	if (check_zero_nan_inf(type, *result))
	{
		return 1;
	}
	(*result).exp = expR;

	(*result).frac = get_frac(type, fracR);
	return 0;
}

void op_add(char *type, char *mode, ieee754_type a, ieee754_type b)
{
	if (check_zero(a.exp, a.frac) && check_zero(b.exp, b.frac))
	{
		print_zero(type, (a.sign == b.sign && a.sign));
		return;
	}

	if (check_inf(type, a.exp, a.frac) && check_inf(type, b.exp, b.frac) && a.sign != b.sign)
	{
		puts("nan");
		return;
	}

	if (check_inf(type, a.exp, a.frac) || check_inf(type, b.exp, b.frac))
	{
		if ((check_inf(type, a.exp, a.frac) && a.sign) || (check_inf(type, b.exp, b.frac) && b.sign))
		{
			printf("-");
		}
		puts("inf");
		return;
	}

	if (check_zero(a.exp, a.frac) || check_zero(b.exp, b.frac))
	{
		ieee754_type num = check_zero(a.exp, a.frac) ? b : a;
		get_result(type, num);
		return;
	}

	reform_values(type, &a);
	reform_values(type, &b);

	a.frac = normalize(a.frac, get_move(type), &a.exp);
	b.frac = normalize(b.frac, get_move(type), &b.exp);
	int64_t frac_a, frac_b;
	uint32_t move = 36;
	frac_a = ((int64_t)a.frac) << move;
	frac_b = ((int64_t)b.frac) << move;
	int32_t expR;
	if (a.exp < b.exp)
	{
		frac_a >>= (b.exp - a.exp);
		expR = b.exp;
	}
	else
	{
		frac_b >>= (a.exp - b.exp);
		expR = a.exp;
	}
	int64_t fracC;
	if (a.sign == b.sign)
	{
		fracC = frac_a + frac_b;
	}
	else
	{
		fracC = frac_a - frac_b;
		if (fracC == 0)
		{
			print_zero(type, 0);
			return;
		}
	}
	uint8_t signR;
	if (a.sign == b.sign)
	{
		signR = a.sign;
	}
	else
	{
		signR = a.sign != 0 == fracC > 0;
	}
	if (fracC < 0)
	{
		fracC <<= 1;
		fracC >>= 1;
	}
	uint64_t frac = (uint64_t)fracC;
	uint32_t fracR;
	uint32_t sub_move = (!strcmp(type, "f")) ? 23 : 10;

	while (1)
	{
		frac = normalize(frac, (move + sub_move), &expR);

		uint64_t round, check;
		fracR = frac >> move;
		round = frac & 0xFFFFFFFFF;
		check = 0x800000000;

		fracR = round_mode(mode, fracR, signR, round, check);

		frac = (int64_t)fracR << move;

		if ((frac >> (move + sub_move)) == 1)
		{
			break;
		}
	}
	ieee754_type result;
	form_result(type, signR, fracR, expR, &result);

	print_result(type, result);
}

void op_subtract(char *type, char *mode, ieee754_type a, ieee754_type b)
{
	b.sign = !b.sign;
	op_add(type, mode, a, b);
}

void op_multiply(char *type, char *mode, ieee754_type a, ieee754_type b)
{
	uint8_t signR = a.sign ^ b.sign;
	if (check_zero(a.exp, a.frac) || check_zero(b.exp, b.frac))
	{
		print_zero(type, signR);
		return;
	}
	if ((check_zero(a.exp, a.frac) && check_inf(type, b.exp, b.frac)) ||
		(check_inf(type, a.exp, a.frac) && check_zero(b.exp, b.frac)))
	{
		puts("nan");
		return;
	}

	if (check_inf(type, a.exp, a.frac) || check_inf(type, b.exp, b.frac))
	{
		if (signR)
		{
			printf("-");
		}
		puts("inf");
		return;
	}

	reform_values(type, &a);
	reform_values(type, &b);

	a.frac = normalize(a.frac, get_move(type), &a.exp);
	b.frac = normalize(b.frac, get_move(type), &b.exp);

	int32_t expR = a.exp + b.exp;
	uint32_t move = !(strcmp(type, "f")) ? 46 : 20;
	uint64_t fracC = (uint64_t)a.frac * (uint64_t)b.frac;
	uint32_t fracR;
	while (1)
	{
		fracC = normalize(fracC, move, &expR);

		fracR = fracC >> (move / 2);

		uint32_t round, check;
		check = (1 << ((move / 2) - 1));
		round = fracC & ((check << 1) - 1);
		fracR = round_mode(mode, fracR, signR, round, check);
		fracC = (uint64_t)fracR << (move / 2);
		if ((fracC >> move) == 1)
		{
			break;
		}
	}

	ieee754_type result;

	form_result(type, signR, fracR, expR, &result);

	print_result(type, result);
}

void op_divide(char *type, char *mode, ieee754_type a, ieee754_type b)
{
	uint8_t signR = a.sign ^ b.sign;
	if ((check_zero(a.exp, a.frac) && check_zero(b.exp, b.frac)) ||
		(check_inf(type, a.exp, a.frac) && check_inf(type, b.exp, b.frac)))
	{
		puts("nan");
		return;
	}
	if (check_zero(a.exp, a.frac) || check_inf(type, b.exp, b.frac))
	{
		print_zero(type, signR);
		return;
	}
	if (check_zero(b.exp, b.frac) || check_inf(type, a.exp, a.frac))
	{
		if (signR)
		{
			printf("-");
		}
		puts("inf");
		return;
	}

	reform_values(type, &a);
	reform_values(type, &b);
	a.frac = normalize(a.frac, get_move(type), &a.exp);
	b.frac = normalize(b.frac, get_move(type), &b.exp);

	int32_t expR = a.exp - b.exp;
	uint64_t frac_a;
	uint32_t move = !(strcmp(type, "f")) ? 36 : 23;
	frac_a = ((uint64_t)a.frac) << move;
	uint64_t fracC = frac_a / (uint64_t)b.frac;
	uint32_t fracR;

	while (1)
	{
		fracC = normalize(fracC, move, &expR);

		uint32_t round, check;

		check = 1 << 13;
		round = fracC & (check - 1);

		fracR = fracC >> 13;

		fracR = round_mode(mode, fracR, signR, round, check);

		fracC = (uint64_t)fracR << 13;

		if ((fracC >> move) == 1)
		{
			break;
		}
	}

	ieee754_type result;
	form_result(type, signR, fracR, expR, &result);

	print_result(type, result);
}

void get_operation_result(char *type, char *mode, char *op, ieee754_type a, ieee754_type b)
{
	if (check_nan(type, a.exp, a.frac) || check_nan(type, b.exp, b.frac))
	{
		puts("nan");
		return;
	}

	if (!strcmp(op, "+"))
	{
		op_add(type, mode, a, b);
	}
	else if (!strcmp(op, "-"))
	{
		op_subtract(type, mode, a, b);
	}
	else if (!strcmp(op, "*"))
	{
		op_multiply(type, mode, a, b);
	}
	else
	{
		op_divide(type, mode, a, b);
	}
}

int main(int argc, char *argv[])
{
	char *type, *mode;
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "Unhandled amount of arguments.");
		return ERROR_ARGUMENTS_INVALID;
	}
	else
	{
		type = argv[1];
		mode = argv[2];
		if (check_type(type) || check_mode(mode))
		{
			return ERROR_ARGUMENTS_INVALID;
		}
		ieee754_type a = get_parts(type, getNum(argv[3]));
		if (argc == 4)
		{
			if (check_zero_nan_inf(type, a))
			{
				return SUCCESS;
			}
			get_result(type, a);
		}
		else
		{
			char *op = argv[4];
			if (check_op(op))
			{
				return ERROR_ARGUMENTS_INVALID;
			}
			ieee754_type b = get_parts(type, getNum(argv[5]));
			get_operation_result(type, mode, op, a, b);
		}
	}

	return SUCCESS;
}
