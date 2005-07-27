/*
  $NiH: romcmp.c,v 1.1 2005/07/13 17:42:20 dillo Exp $

  romcmp.c -- compare two ROMs
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <string.h>

#include "rom.h"



state_t
romcmp(const rom_t *r1, const rom_t *r2, int merge)
{
    /* r1 is important */
    /* in match: r1 is from zip, r2 from rom */
    
    if (strcasecmp(rom_name(r1), (merge && rom_merge(r2)
				  ? rom_merge(r2) : rom_name(r2))) == 0) {
	if (rom_size(r2) == 0)
	    return ROM_OK;
	
	if (rom_size(r1) == rom_size(r2)) {
	    if (rom_status(r1) == STATUS_NODUMP
		|| rom_status(r2) == STATUS_NODUMP
		|| hashes_cmp(rom_hashes(r1),
			      rom_hashes(r2)) == HASHES_CMP_MATCH)
		return ROM_OK;
	    else if (hashes_are_crc_complement(rom_hashes(r1),
					       rom_hashes(r2))
		     || rom_status(r1) == STATUS_BADDUMP
		     || rom_status(r2) == STATUS_BADDUMP)
		return ROM_BESTBADDUMP;
	    else
		return ROM_CRCERR;
	}
	else if (rom_size(r1) > rom_size(r2))
	    return ROM_LONG;
	else
	    return ROM_SHORT;
    }
    else if (rom_size(r1) == rom_size(r2)
	     && rom_size(r2) != 0
	     && hashes_cmp(rom_hashes(r1), rom_hashes(r2)) == HASHES_CMP_MATCH)
	return ROM_NAMERR;
    else
	return ROM_UNKNOWN;
}
