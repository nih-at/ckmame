/*
  file_compare.c -- compare various parts of two files
  Copyright (C) 2004-2008 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <string.h>

#include "file.h"

bool
file_compare_m(const file_t *fg, const file_t *fa)
{
    return strcmp(file_merge(fg) ? file_merge(fg) : file_name(fg),
		  file_name(fa)) == 0;
}



bool
file_compare_msc(const file_t *fg, const file_t *fa)
{
    return file_compare_m(fg, fa) && file_compare_sc(fg, fa);
}



bool
file_compare_nsc(const file_t *fg, const file_t *fa)
{
    return file_compare_n(fg, fa) && file_compare_sc(fg, fa);
}



bool
file_compare_sc(const file_t *fg, const file_t *fa)
{
    int i;

    for (i=0; i<FILE_SH_MAX; i++) {
	if (!file_sh_is_set(fa, i) && i != FILE_SH_FULL)
	    continue;
	
	if (file_size_known(fg) && file_size_xxx_known(fa, i)
	    && file_size(fg) != file_size_xxx(fa, i))
	    continue;

	if (hashes_types(file_hashes(fg)) == 0)
	    return true;

	/* XXX: don't hardcode CRC, doesn't work for disks */
	if ((hashes_types(file_hashes(fg))
	     & hashes_types(file_hashes_xxx(fa, i)) & HASHES_TYPE_CRC)
	    && (hashes_crc(file_hashes(fg))
		== hashes_crc(file_hashes_xxx(fa, i))))
	    return true;
    }

    return false;
}



bool
file_sh_is_set(const file_t *f, int i)
{
    return file_size_xxx_known(f, i)
	&& hashes_types(file_hashes_xxx(f, i)) != 0;
}
