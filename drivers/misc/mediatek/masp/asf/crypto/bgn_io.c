#include "sec_osal_light.h"
#include "sec_cust_struct.h"
#include "bgn_internal.h"

#define MOD "BGN"

/**************************************************************************
 *  INTERNAL DEFINITION
 **************************************************************************/
#define ASCII_MASK                  255
#define ASCII_0                     0x30
#define ASCII_7                     0x37
#define ASCII_9                     0x39
#define ASCII_A                     0x41
#define ASCII_F                     0x46
#define ASCII_W                     0x57
#define ASCII_a                     0x61
#define ASCII_f                     0x66

/**************************************************************************
 *  FUNCTIONS
 **************************************************************************/
static int bgn_get_digit(ulong *d, int radix, char c)
{
	*d = ASCII_MASK;

	if (c >= ASCII_0 && c <= ASCII_9)
		*d = c - ASCII_0;

	if (c >= ASCII_A && c <= ASCII_F)
		*d = c - ASCII_7;

	if (c >= ASCII_a && c <= ASCII_f)
		*d = c - ASCII_W;

	if (*d >= (ulong) radix)
		return E_BGN_INVALID_CHARACTER;

	return 0;
}

int bgn_read_str(bgn *X, int radix, const char *s, int length)
{
	int ret, i, j, n, slen;
	ulong d;
	bgn T;


	if (radix < 2 || radix > 16)
		return E_BGN_BAD_INPUT_DATA;

	bgn_init(&T);
	slen = length;

	if (radix == 16) {
		n = B_T_L(slen << 2);

		ret = bgn_grow(X, n);
		if (0 != ret)
			goto _exit;

		ret = bgn_lset(X, 0);
		if (0 != ret)
			goto _exit;

		for (i = slen - 1, j = 0; i >= 0; i--, j++) {
			if (i == 0 && s[i] == '-') {
				X->s = -1;
				break;
			}

			ret = bgn_get_digit(&d, radix, s[i]);
			if (0 != ret)
				goto _exit;

			X->p[j / (2 * ciL)] |= d << ((j % (2 * ciL)) << 2);
		}
	} else {
		ret = bgn_lset(X, 0);
		if (0 != ret)
			goto _exit;

		for (i = 0; i < slen; i++) {
			if (i == 0 && s[i] == '-') {
				X->s = -1;
				continue;
			}

			ret = bgn_get_digit(&d, radix, s[i]);
			if (0 != ret)
				goto _exit;

			ret = bgn_mul_int(&T, X, radix);
			if (0 != ret)
				goto _exit;

			if (X->s == 1) {
				ret = bgn_add_int(X, &T, d);
				if (0 != ret)
					goto _exit;
			} else {
				ret = bgn_sub_int(X, &T, d);
				if (0 != ret)
					goto _exit;
			}
		}
	}

_exit:

	bgn_free(&T);

	return ret;
}

int bgn_read_bin(bgn *X, const unsigned char *buf, int len)
{
	int ret, i, j, n;

	for (n = 0; n < len; n++) {
		if (buf[n] != 0)
			break;
	}

	ret = bgn_grow(X, C_T_L(len - n));
	if (0 != ret)
		goto _exit;

	ret = bgn_lset(X, 0);
	if (0 != ret)
		goto _exit;

	for (i = len - 1, j = 0; i >= n; i--, j++)
		X->p[j / ciL] |= ((ulong) buf[i]) << ((j % ciL) << 3);

_exit:

	return ret;
}

int bgn_write_bin(const bgn *X, unsigned char *buf, int len)
{
	int i, j, n;

	n = bgn_size(X);

	if (len < n)
		return E_BGN_BUFFER_TOO_SMALL;

	memset(buf, 0, len);

	for (i = len - 1, j = 0; n > 0; i--, j++, n--)
		buf[i] = (unsigned char) (X->p[j / ciL] >> ((j % ciL) << 3));

	return 0;
}
