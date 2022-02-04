/*
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * sf-pcapng.h - pcapng-file-format-specific routines
 *
 * Used to read pcapng savefiles.
 */

#ifndef sf_pcapng_h
#define	sf_pcapng_h

#ifdef __APPLE__
extern pcap_t *pcap_ng_check_header(const uint8_t *magic, FILE *fp,
    u_int precision, char *errbuf, int *err, int isng);

struct block_cursor;
void *
get_from_block_data(struct block_cursor *cursor, size_t chunk_size,
		    char *errbuf);
#else
extern pcap_t *pcap_ng_check_header(const uint8_t *magic, FILE *fp,
    u_int precision, char *errbuf, int *err);
#endif /* __APPLE__ */

#endif