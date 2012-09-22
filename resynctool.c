/*
 * resynctool.c
 * find the counter value for trid event-based hotp tokens
 * prints a state file entry to stdout
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/hmac.h>
#include <unistd.h>
#include "extern.h"

extern char *optarg;

static void
usage(const char *pname)
{
  fprintf(stderr, "usage: %s -1 <otp1> -2 <otp2> [-u <user>] [-k <key>] [-c <challenge>] [-i <counter>] [-f <counter>] [-d]\n", pname);
  exit(1);
}

/*
 * Convert an ASCII keystring to a keyblock.
 * Returns keylen on success, -1 otherwise.
 */
static int
keystring2keyblock(const char *keystring, unsigned char keyblock[])
{
  size_t l = strlen(keystring);

  /* up to 256-bit key with optional line ending */
  if ((l & ~1) > 64)
    return -1;

  return a2x(keystring, keyblock);
}

static void
c2c(uint64_t counter, unsigned char challenge[8])
{
  challenge[0] = counter >> 56;
  challenge[1] = counter >> 48;
  challenge[2] = counter >> 40;
  challenge[3] = counter >> 32;
  challenge[4] = counter >> 24;
  challenge[5] = counter >> 16;
  challenge[6] = counter >> 8;
  challenge[7] = counter;
}

int
main(int argc, char *argv[])
{
  char *username = "username";
  char *keystring = "0000000000000000000000000000000000000000";
  char *pass1 = NULL;
  char *pass2 = NULL;
  unsigned char challenge[8];
  char response[10];
  unsigned char keyblock[32];
  uint64_t counter;
  uint64_t initial = 0;
  uint64_t final = 0;
  int keylen, len1, len2;

  int debug = 0;

  {
    int c;

    while ((c = getopt(argc, argv, ":d1:2:u:k:c:i:f:")) != -1) {
      switch (c) {
        case 'd': debug = 1; break;
        case '1': pass1 = optarg; break;
        case '2': pass2 = optarg; break;
        case 'u': username = optarg; break;
        case 'k': keystring = optarg; break;
        case 'c': initial = strtoull(optarg, NULL, 16); break;
        case 'i': initial = strtoull(optarg, NULL, 10); break;
        case 'f':
          if (optarg[0] == '+')	/* NOTE: must be given AFTER -i */
            final = initial + strtoull(&optarg[1], NULL, 10);
          else
            final = strtoull(optarg, NULL, 10);
          break;
        default:
          (void) fprintf(stderr, "%s: unknown option\n", argv[0]);
          exit(1);
      }
    }
  }
  if (!pass1 || !pass2)
    usage(argv[0]);

  len1 = strlen(pass1);
  len2 = strlen(pass2);

  if (len1 != len2) {
    (void) fprintf(stderr, "%s: passcode lengths differ\n", argv[0]);
    exit(1);
  }

  if (len1 < 6 || len2 < 6 || len1 > 9 || len2 > 9) {
    (void) fprintf(stderr, "%s: bad HOTP passcode length\n", argv[0]);
    exit(1);
  }

  if (!final)
    final = initial + 65536;

  keylen = keystring2keyblock(keystring, keyblock);
  if (keylen == -1) {
    (void) fprintf(stderr, "%s: key is invalid\n", argv[0]);
    exit(1);
  }
  (void) memset(challenge, 0, 8);

  for (counter = initial; counter < final; ++counter) {
    c2c(counter, challenge);
    hotp_mac(challenge, response, keyblock, keylen, len1);
   
    if (debug == 1)
       (void) printf("%" PRIu64 ": %s\n", counter, response);

    if (!strcmp((char *) response, pass1)) {
      if (debug)
        (void) printf("matched %s at counter=%" PRIu64 "\n", pass1, counter);

      /* matched first pass, look for 2nd */
      c2c(++counter, challenge);
      hotp_mac(challenge, response, keyblock, keylen, len2);
      if (!strcmp((char *) response, pass2)) {
        char s[17];

        /* print the new state containing the last used challenge */
        (void) printf("5:%s:%s:::0:0:0:\n", username,
		      x2a(challenge, sizeof(challenge),
			  s, x2a_hex_conversion));
        exit(0);

      } else if (debug) {
        (void) fprintf(stderr,
                       "mismatch for counter=%" PRIu64" , wanted: %s got: %s\n",
                       counter, pass2, response);
        (void) fprintf(stderr, "continuing search\n");
      } /* else (pass2 does not match) */
    } /* if (pass1 matches) */
  } /* for (each event counter) */

  (void) fprintf(stderr, "%s: counter value not found\n", argv[0]);
  exit(1);
}
