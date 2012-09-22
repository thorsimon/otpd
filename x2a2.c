/*
 * $Id: xfunc.c 56 2010-08-24 23:10:52Z frank $
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *  For alternative licensing terms, contact licensing@tri-dsystems.com.
 *
 * Copyright 2005-2007 TRI-D Systems, Inc.
 */

#include "ident.h"
RCSID("$Id: xfunc.c 56 2010-08-24 23:10:52Z frank $")

#include <string.h>

#include "extern.h"

/* ascii to hex; returns HEXlen on success or -1 on error */
ssize_t
a2x(const char *s, unsigned char x[])
{
  return a2nx(s, x, strlen(s) / 2);
}

/* bounded ascii to hex; returns HEXlen on success or -1 on error */
ssize_t
a2nx(const char *s, unsigned char x[], size_t l)
{
  unsigned i;

  /*
   * We could just use sscanf, but we do this a lot, and have very
   * specific needs, and it's easy to implement, so let's go for it!
   */
  for (i = 0; i < l; ++i) {
    unsigned int n[2];
    int j;

    /* extract 2 nibbles */
    n[0] = *s++;
    n[1] = *s++;

    /* verify range */
    for (j = 0; j < 2; ++j) {
      if ((n[j] >= '0' && n[j] <= '9') ||
          (n[j] >= 'A' && n[j] <= 'F') ||
          (n[j] >= 'a' && n[j] <= 'f'))
        continue;
      return -1;
    }

    /* convert ASCII hex digits to numeric values */
    n[0] -= '0';
    n[1] -= '0';
    if (n[0] > 9) {
      if (n[0] > 'F' - '0')
        n[0] -= 'a' - '9' - 1;
      else
        n[0] -= 'A' - '9' - 1;
    }
    if (n[1] > 9) {
      if (n[1] > 'F' - '0')
        n[1] -= 'a' - '9' - 1;
      else
        n[1] -= 'A' - '9' - 1;
    }

    /* store as octets */
    x[i]  = n[0] << 4;
    x[i] += n[1];
  } /* for (each octet) */

  return i;
}

/* Character maps for generic hex and vendor specific decimal modes */
const char x2a_hex_conversion[]         = "0123456789abcdef";
const char x2a_cc_dec_conversion[]      = "0123456789012345";
const char x2a_snk_dec_conversion[]     = "0123456789222333";
const char x2a_sc_friendly_conversion[] = "0123456789ahcpef";

/*
 * hex to ascii
 * Fills in s, which must point to at least len*2+1 bytes of space.
 */
char *
x2a(const unsigned char *x, size_t len, char *s, const char conversion[16])
{
  unsigned i;

  for (i = 0; i < len; ++i) {
    unsigned n[2];

    n[0] = (x[i] >> 4) & 0x0f;
    n[1] = x[i] & 0x0f;
    s[2 * i + 0] = conversion[n[0]];
    s[2 * i + 1] = conversion[n[1]];
  }
  s[2 * len] = '\0';

  return s;
}
