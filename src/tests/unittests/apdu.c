/*
 * apdu.c: Unit tests for APDU encoding
 *
 * Copyright (C) 2026 Andreas Schultz <aschultz@warp10.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <string.h>
#include "libopensc/opensc.h"

/* encode `apdu` for `proto` and compare the bytes with `exp` (`explen` long) */
static int check(const char *name, sc_apdu_t *apdu, unsigned int proto,
		const unsigned char *exp, size_t explen)
{
	unsigned char buf[64];
	size_t len = sc_apdu_get_length(apdu, proto);
	int r = sc_apdu2bytes(NULL, apdu, proto, buf, sizeof buf);

	if (r != SC_SUCCESS) {
		printf("FAIL %s: sc_apdu2bytes returned %d\n", name, r);
		return 1;
	}
	if (len != explen || memcmp(buf, exp, explen) != 0) {
		printf("FAIL %s: got %zu bytes, expected %zu\n", name, len, explen);
		return 1;
	}
	printf("ok   %s\n", name);
	return 0;
}

int main(void)
{
	int fail = 0;
	unsigned char data[2] = { 0x3F, 0x00 };
	/* SELECT 3F00 with response expected: a short case 4 APDU */
	sc_apdu_t apdu;

	memset(&apdu, 0, sizeof apdu);
	apdu.cse = SC_APDU_CASE_4_SHORT;
	apdu.ins = 0xA4;
	apdu.lc = 2;
	apdu.data = data;
	apdu.datalen = 2;
	apdu.le = 256;

	/* T=0: the Le byte is never appended */
	{
		const unsigned char exp[] = { 0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
		fail += check("case 4 short, T=0", &apdu, SC_PROTO_T0, exp, sizeof exp);
	}
	/* T=1: the Le byte follows the data (le 256 -> 0x00) */
	{
		const unsigned char exp[] = { 0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00, 0x00 };
		fail += check("case 4 short, T=1", &apdu, SC_PROTO_T1, exp, sizeof exp);
	}
	/* T=1 with SC_APDU_FLAGS_CASE4_SHORT_NO_LE: no Le byte, as under T=0 */
	{
		const unsigned char exp[] = { 0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
		apdu.flags |= SC_APDU_FLAGS_CASE4_SHORT_NO_LE;
		fail += check("case 4 short, T=1 + NO_LE flag", &apdu, SC_PROTO_T1, exp, sizeof exp);
		apdu.flags &= ~SC_APDU_FLAGS_CASE4_SHORT_NO_LE;
	}

	/* the flag must not change a case 3 APDU (no Le in the first place) */
	{
		const unsigned char exp[] = { 0x00, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
		apdu.cse = SC_APDU_CASE_3_SHORT;
		apdu.flags |= SC_APDU_FLAGS_CASE4_SHORT_NO_LE;
		fail += check("case 3 short, flag ignored", &apdu, SC_PROTO_T1, exp, sizeof exp);
	}

	return fail ? 1 : 0;
}
