/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "sec_osal_light.h"
#include "sec_osal.h"
#include "sec_cust_struct.h"
#include "bgn_internal.h"

#define MOD "BGN"

/**************************************************************************
 *  FUNCTIONS
 **************************************************************************/
int bgn_grow(bgn *P_X, int nbl)
{
	ulong *p;

	if (P_X->n < nbl) {
		p = (ulong *) osal_kmalloc(nbl * ciL);
		if (p == NULL)
			return 1;

		memset(p, 0, nbl * ciL);

		if (P_X->p != NULL) {
			memcpy(p, P_X->p, P_X->n * ciL);
			memset(P_X->p, 0, P_X->n * ciL);
			osal_kfree(P_X->p);
		}

		P_X->n = nbl;
		P_X->p = p;
	}

	return 0;
}

int bgn_copy(bgn *P_X, const bgn *Y)
{
	int ret, i;

	if (P_X == Y)
		return 0;

	for (i = Y->n - 1; i > 0; i--)
		if (Y->p[i] != 0)
			break;
	i++;

	P_X->s = Y->s;

	ret = bgn_grow(P_X, i);
	if (0 != ret)
		goto _exit;

	memset(P_X->p, 0, P_X->n * ciL);
	memcpy(P_X->p, Y->p, i * ciL);

_exit:

	return ret;
}

void bgn_swap(bgn *P_X, bgn *Y)
{
	bgn T;

	memcpy(&T, P_X, sizeof(bgn));
	memcpy(P_X, Y, sizeof(bgn));
	memcpy(Y, &T, sizeof(bgn));
}

int bgn_lset(bgn *P_X, int z)
{
	int ret;

	ret = bgn_grow(P_X, 1);
	if (0 != ret)
		goto _exit;

	memset(P_X->p, 0, P_X->n * ciL);

	P_X->p[0] = (z < 0) ? -z : z;
	P_X->s = (z < 0) ? -1 : 1;

_exit:
	return ret;
}

int bgn_lsb(const bgn *P_X)
{
	int i, j, count = 0;

	for (i = 0; i < P_X->n; i++)
		for (j = 0; j < (int)biL; j++, count++)
			if (((P_X->p[i] >> j) & 1) != 0)
				return count;

	return 0;
}

int bgn_msb(const bgn *P_X)
{
	int i, j;

	for (i = P_X->n - 1; i > 0; i--)
		if (P_X->p[i] != 0)
			break;

	for (j = biL - 1; j >= 0; j--)
		if (((P_X->p[i] >> j) & 1) != 0)
			break;

	return (i * biL) + j + 1;
}

int bgn_size(const bgn *P_X)
{
	return (bgn_msb(P_X) + 7) >> 3;
}

int bgn_shift_l(bgn *P_X, int count)
{
	int ret, i, v0, t1;
	ulong r0 = 0, r1;

	v0 = count / (biL);
	t1 = count & (biL - 1);

	i = bgn_msb(P_X) + count;

	if (P_X->n * (int)biL < i) {
		ret = bgn_grow(P_X, B_T_L(i));
		if (0 != ret)
			goto _exit;
	}

	ret = 0;

	if (v0 > 0) {
		for (i = P_X->n - 1; i >= v0; i--)
			P_X->p[i] = P_X->p[i - v0];

		for (; i >= 0; i--)
			P_X->p[i] = 0;
	}

	if (t1 > 0) {
		for (i = v0; i < P_X->n; i++) {
			r1 = P_X->p[i] >> (biL - t1);
			P_X->p[i] <<= t1;
			P_X->p[i] |= r0;
			r0 = r1;
		}
	}

_exit:

	return ret;
}

int bgn_shift_r(bgn *P_X, int count)
{
	int i, v0, v1;
	ulong r0 = 0, r1;

	v0 = count / biL;
	v1 = count & (biL - 1);

	if (v0 > 0) {
		for (i = 0; i < P_X->n - v0; i++)
			P_X->p[i] = P_X->p[i + v0];

		for (; i < P_X->n; i++)
			P_X->p[i] = 0;
	}

	if (v1 > 0) {
		for (i = P_X->n - 1; i >= 0; i--) {
			r1 = P_X->p[i] << (biL - v1);
			P_X->p[i] >>= v1;
			P_X->p[i] |= r0;
			r0 = r1;
		}
	}

	return 0;
}

int bgn_cmp_abs(const bgn *P_X, const bgn *Y)
{
	int i, j;

	for (i = P_X->n - 1; i >= 0; i--) {
		if (P_X->p[i] != 0)
			break;
	}

	for (j = Y->n - 1; j >= 0; j--) {
		if (Y->p[j] != 0)
			break;
	}

	if (i < 0 && j < 0)
		return 0;

	if (i > j)
		return 1;

	if (j > i)
		return -1;

	for (; i >= 0; i--) {
		if (P_X->p[i] > Y->p[i])
			return 1;

		if (P_X->p[i] < Y->p[i])
			return -1;
	}

	return 0;
}

int bgn_cmp_num(const bgn *P_X, const bgn *Y)
{
	int i, j;

	for (i = P_X->n - 1; i >= 0; i--) {
		if (P_X->p[i] != 0)
			break;
	}

	for (j = Y->n - 1; j >= 0; j--) {
		if (Y->p[j] != 0)
			break;
	}

	if (i < 0 && j < 0)
		return 0;

	if (i > j)
		return P_X->s;
	if (j > i)
		return -P_X->s;

	if (P_X->s > 0 && Y->s < 0)
		return 1;

	if (Y->s > 0 && P_X->s < 0)
		return -1;

	for (; i >= 0; i--) {
		if (P_X->p[i] > Y->p[i])
			return P_X->s;
		if (P_X->p[i] < Y->p[i])
			return -P_X->s;
	}

	return 0;
}

int bgn_cmp_int(const bgn *P_X, int z)
{
	bgn Y;
	ulong p[1];

	*p = (z < 0) ? -z : z;
	Y.s = (z < 0) ? -1 : 1;
	Y.n = 1;
	Y.p = p;

	return bgn_cmp_num(P_X, &Y);
}

int bgn_add_abs(bgn *P_X, const bgn *P_A, const bgn *P_B)
{
	int ret, i, j;
	ulong *o, *p, c;

	if (P_X == P_B) {
		const bgn *T = P_A;

		P_A = P_X;
		P_B = T;
	}

	if (P_X != P_A) {
		ret = bgn_copy(P_X, P_A);
		if (0 != ret)
			goto _exit;
	}

	P_X->s = 1;

	for (j = P_B->n - 1; j >= 0; j--) {
		if (P_B->p[j] != 0)
			break;
	}

	ret = bgn_grow(P_X, j + 1);
	if (0 != ret)
		goto _exit;

	o = P_B->p;
	p = P_X->p;
	c = 0;

	for (i = 0; i <= j; i++, o++, p++) {
		*p += c;
		c = (*p < c);
		*p += *o;
		c += (*p < *o);
	}

	while (c != 0) {
		if (i >= P_X->n) {
			ret = bgn_grow(P_X, i + 1);
			if (0 != ret)
				goto _exit;

			p = P_X->p + i;
		}

		*p += c;
		c = (*p < c);
		i++;
	}
_exit:

	return ret;
}

void bgn_sub_hlp(int n, ulong *s, ulong *d)
{
	int i;
	ulong c, z;

	for (i = c = 0; i < n; i++, s++, d++) {
		z = (*d < c);
		*d -= c;
		c = (*d < *s) + z;
		*d -= *s;
	}

	while (c != 0) {
		z = (*d < c);
		*d -= c;
		c = z;
		i++;
		d++;
	}
}

int bgn_sub_abs(bgn *P_X, const bgn *P_A, const bgn *P_B)
{
	bgn TB;
	int ret, n;

	if (bgn_cmp_abs(P_A, P_B) < 0)
		return E_BGN_NEGATIVE_VALUE;

	bgn_init(&TB);

	if (P_X == P_B) {
		ret = bgn_copy(&TB, P_B);
		if (0 != ret)
			goto _exit;
		P_B = &TB;
	}

	if (P_X != P_A) {
		ret = bgn_copy(P_X, P_A);
		if (0 != ret)
			goto _exit;
	}

	P_X->s = 1;

	ret = 0;

	for (n = P_B->n - 1; n >= 0; n--) {
		if (P_B->p[n] != 0)
			break;
	}

	bgn_sub_hlp(n + 1, P_B->p, P_X->p);

_exit:

	bgn_free(&TB);

	return ret;
}

int bgn_add_bgn(bgn *P_X, const bgn *P_A, const bgn *P_B)
{
	int ret, s = P_A->s;

	if (P_A->s * P_B->s < 0) {
		if (bgn_cmp_abs(P_A, P_B) >= 0) {
			ret = bgn_sub_abs(P_X, P_A, P_B);
			if (0 != ret)
				goto _exit;

			P_X->s = s;
		} else {
			ret = bgn_sub_abs(P_X, P_B, P_A);
			if (0 != ret)
				goto _exit;

			P_X->s = -s;
		}
	} else {
		ret = bgn_add_abs(P_X, P_A, P_B);
		if (0 != ret)
			goto _exit;
		P_X->s = s;
	}

_exit:

	return ret;
}

int bgn_sub_bgn(bgn *P_X, const bgn *P_A, const bgn *P_B)
{
	int ret, s = P_A->s;

	if (P_A->s * P_B->s > 0) {
		if (bgn_cmp_abs(P_A, P_B) >= 0) {
			ret = bgn_sub_abs(P_X, P_A, P_B);
			if (0 != ret)
				goto _exit;

			P_X->s = s;
		} else {
			ret = bgn_sub_abs(P_X, P_B, P_A);
			if (0 != ret)
				goto _exit;

			P_X->s = -s;
		}
	} else {
		ret = bgn_add_abs(P_X, P_A, P_B);
		if (0 != ret)
			goto _exit;

		P_X->s = s;
	}
_exit:

	return ret;
}

int bgn_add_int(bgn *P_X, const bgn *P_A, int b)
{
	bgn _B;
	ulong p[1];

	p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p = p;

	return bgn_add_bgn(P_X, P_A, &_B);
}

int bgn_sub_int(bgn *P_X, const bgn *P_A, int b)
{
	bgn _B;
	ulong p[1];

	p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p = p;

	return bgn_sub_bgn(P_X, P_A, &_B);
}

void bgn_alu(ulong **s, ulong **d, ulong b, ulong c, unsigned int loop_count)
{
	unsigned int i;
	unsigned long s_0, s_1, b_0, b_1;
	unsigned long r_0, r_1, r_x, r_y;

	b_0 = (b << biH) >> biH;
	b_1 = (b >> biH);

	for (i = 0; i < loop_count; i++) {
		s_0 = (**s << biH) >> biH;
		s_1 = (**s >> biH); (*s)++;
		r_x = s_0 * b_1; r_0 = s_0 * b_0;
		r_y = s_1 * b_0; r_1 = s_1 * b_1;
		r_1 += (r_x >> biH);
		r_1 += (r_y >> biH);
		r_x <<= biH; r_y <<= biH;
		r_0 += r_x; r_1 += (r_0 < r_x);
		r_0 += r_y; r_1 += (r_0 < r_y);
		r_0 +=  c; r_1 += (r_0 <  c);
		r_0 += (unsigned long)*d;
		r_1 += (r_0 < (unsigned long) *d);
		c = r_1; *((*d)++) = r_0;
	}

}

void bgn_mul_hlp(int i, ulong *s, ulong *d, ulong b)
{
	ulong c = 0, t = 0;

	for (; i >= 16; i -= 16)
		bgn_alu(&s, &d, b, c, 16);

	for (; i >= 8; i -= 8)
		bgn_alu(&s, &d, b, c, 8);

	for (; i > 0; i--)
		bgn_alu(&s, &d, b, c, 1);

	t++;

	do {
		*d += c;
		c = (*d < c);
		d++;
	} while (c != 0);
}

int bgn_mul_bgn(bgn *P_X, const bgn *P_A, const bgn *P_B)
{
	int ret, i, j;
	bgn TA, TB;

	bgn_init(&TA);
	bgn_init(&TB);

	if (P_X == P_A) {
		ret = bgn_copy(&TA, P_A);
		if (0 != ret)
			goto _exit;

		P_A = &TA;
	}

	if (P_X == P_B) {
		ret = bgn_copy(&TB, P_B);
		if (0 != ret)
			goto _exit;

		P_B = &TB;
	}

	for (i = P_A->n - 1; i >= 0; i--) {
		if (P_A->p[i] != 0)
			break;
	}

	for (j = P_B->n - 1; j >= 0; j--) {
		if (P_B->p[j] != 0)
			break;
	}

	ret = bgn_grow(P_X, i + j + 2);
	if (0 != ret)
		goto _exit;

	ret = bgn_lset(P_X, 0);
	if (0 != ret)
		goto _exit;

	for (i++; j >= 0; j--)
		bgn_mul_hlp(i, P_A->p, P_X->p + j, P_B->p[j]);

	P_X->s = P_A->s * P_B->s;

_exit:

	bgn_free(&TB);
	bgn_free(&TA);

	return ret;
}

int bgn_mul_int(bgn *P_X, const bgn *P_A, ulong b)
{
	bgn _B;
	ulong p[1];

	_B.s = 1;
	_B.n = 1;
	_B.p = p;
	p[0] = b;

	return bgn_mul_bgn(P_X, P_A, &_B);
}

int bgn_div_bgn(bgn *P_Q, bgn *P_R, const bgn *P_A, const bgn *P_B)
{
	int ret, i, n, t, k;
	bgn X, Y, Z, T1, T2;

	if (bgn_cmp_int(P_B, 0) == 0)
		return E_BGN_DIVISION_BY_ZERO;

	bgn_init(&X);
	bgn_init(&Y);
	bgn_init(&Z);
	bgn_init(&T1);
	bgn_init(&T2);

	if (bgn_cmp_abs(P_A, P_B) < 0) {
		if (P_Q != NULL) {
			ret = bgn_lset(P_Q, 0);
			if (0 != ret)
				goto _exit;
		}

		if (P_R != NULL) {
			ret = bgn_copy(P_R, P_A);
			if (0 != ret)
				goto _exit;
		}

		return 0;
	}

	ret = bgn_copy(&X, P_A);
	if (0 != ret)
		goto _exit;

	ret = bgn_copy(&Y, P_B);
	if (0 != ret)
		goto _exit;

	X.s = Y.s = 1;

	ret = bgn_grow(&Z, P_A->n + 2);
	if (0 != ret)
		goto _exit;

	ret = bgn_lset(&Z, 0);
	if (0 != ret)
		goto _exit;

	ret = bgn_grow(&T1, 2);
	if (0 != ret)
		goto _exit;

	ret = bgn_grow(&T2, 3);
	if (0 != ret)
		goto _exit;

	k = bgn_msb(&Y) % biL;

	if (k < (int)biL - 1) {
		k = biL - 1 - k;

		ret = bgn_shift_l(&X, k);
		if (0 != ret)
			goto _exit;

		ret = bgn_shift_l(&Y, k);
		if (0 != ret)
			goto _exit;
	} else {
		k = 0;
	}

	n = X.n - 1;
	t = Y.n - 1;
	bgn_shift_l(&Y, biL * (n - t));

	while (bgn_cmp_num(&X, &Y) >= 0) {
		Z.p[n - t]++;
		bgn_sub_bgn(&X, &X, &Y);
	}

	bgn_shift_r(&Y, biL * (n - t));

	for (i = n; i > t; i--) {
		if (X.p[i] >= Y.p[t]) {
			Z.p[i - t - 1] = ~0;
		} else {
			ulong q0, q1, r0, r1;
			ulong d0, d1, d, m;

			d = Y.p[t];
			d0 = (d << biH) >> biH;
			d1 = (d >> biH);

			q1 = X.p[i] / d1;
			r1 = X.p[i] - d1 * q1;
			r1 <<= biH;
			r1 |= (X.p[i - 1] >> biH);

			m = q1 * d0;
			if (r1 < m) {
				q1--, r1 += d;
				while (r1 >= d && r1 < m)
					q1--, r1 += d;
			}
			r1 -= m;

			q0 = r1 / d1;
			r0 = r1 - d1 * q0;
			r0 <<= biH;
			r0 |= (X.p[i - 1] << biH) >> biH;

			m = q0 * d0;
			if (r0 < m) {
				q0--, r0 += d;
				while (r0 >= d && r0 < m)
					q0--, r0 += d;
			}
			r0 -= m;

			Z.p[i - t - 1] = (q1 << biH) | q0;
		}

		Z.p[i - t - 1]++;

		do {
			Z.p[i - t - 1]--;

			ret = bgn_lset(&T1, 0);
			if (0 != ret)
				goto _exit;

			T1.p[0] = (t < 1) ? 0 : Y.p[t - 1];
			T1.p[1] = Y.p[t];

			ret = bgn_mul_int(&T1, &T1, Z.p[i - t - 1]);
			if (0 != ret)
				goto _exit;

			ret = bgn_lset(&T2, 0);
			if (0 != ret)
				goto _exit;

			T2.p[0] = (i < 2) ? 0 : X.p[i - 2];
			T2.p[1] = (i < 1) ? 0 : X.p[i - 1];
			T2.p[2] = X.p[i];
		} while (bgn_cmp_num(&T1, &T2) > 0);

		ret = bgn_mul_int(&T1, &Y, Z.p[i - t - 1]);
		if (0 != ret)
			goto _exit;

		ret = bgn_shift_l(&T1, biL * (i - t - 1));
		if (0 != ret)
			goto _exit;

		ret = bgn_sub_bgn(&X, &X, &T1);
		if (0 != ret)
			goto _exit;

		if (bgn_cmp_int(&X, 0) < 0) {
			ret = bgn_copy(&T1, &Y);
			if (0 != ret)
				goto _exit;

			ret = bgn_shift_l(&T1, biL * (i - t - 1));
			if (0 != ret)
				goto _exit;

			ret = bgn_add_bgn(&X, &X, &T1);
			if (0 != ret)
				goto _exit;

			Z.p[i - t - 1]--;
		}
	}

	if (P_Q != NULL) {
		bgn_copy(P_Q, &Z);
		P_Q->s = P_A->s * P_B->s;
	}

	if (P_R != NULL) {
		bgn_shift_r(&X, k);
		bgn_copy(P_R, &X);

		P_R->s = P_A->s;

		if (bgn_cmp_int(P_R, 0) == 0)
			P_R->s = 1;
	}

_exit:

	bgn_free(&X);
	bgn_free(&Y);
	bgn_free(&Z);
	bgn_free(&T1);
	bgn_free(&T2);

	return ret;
}

int bgn_div_int(bgn *P_Q, bgn *P_R, const bgn *P_A, int b)
{
	bgn _B;
	ulong p[1];

	p[0] = (b < 0) ? -b : b;
	_B.s = (b < 0) ? -1 : 1;
	_B.n = 1;
	_B.p = p;

	return bgn_div_bgn(P_Q, P_R, P_A, &_B);
}

int bgn_mod_bgn(bgn *P_R, const bgn *P_A, const bgn *P_B)
{
	int ret;

	if (bgn_cmp_int(P_B, 0) < 0)
		return E_BGN_NEGATIVE_VALUE;

	ret = bgn_div_bgn(NULL, P_R, P_A, P_B);
	if (0 != ret)
		goto _exit;

	while (bgn_cmp_int(P_R, 0) < 0) {
		ret = bgn_add_bgn(P_R, P_R, P_B);
		if (0 != ret)
			goto _exit;
	}

	while (bgn_cmp_num(P_R, P_B) >= 0) {
		ret = bgn_sub_bgn(P_R, P_R, P_B);
		if (0 != ret)
			goto _exit;
	}

_exit:
	return ret;
}

int bgn_mod_int(ulong *r, const bgn *P_A, int b)
{
	int i;
	ulong x, y, z;

	if (b == 0)
		return E_BGN_DIVISION_BY_ZERO;

	if (b < 0)
		return E_BGN_NEGATIVE_VALUE;

	if (b == 1) {
		*r = 0;
		return 0;
	}

	if (b == 2) {
		*r = P_A->p[0] & 1;
		return 0;
	}

	for (i = P_A->n - 1, y = 0; i >= 0; i--) {
		x = P_A->p[i];
		y = (y << biH) | (x >> biH);
		z = y / b;
		y -= z * b;

		x <<= biH;
		y = (y << biH) | (x >> biH);
		z = y / b;
		y -= z * b;
	}

	if (P_A->s < 0 && y != 0)
		y = b - y;

	*r = y;

	return 0;
}
