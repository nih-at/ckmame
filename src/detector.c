/*
  $NiH: detector.c,v 1.1 2007/04/10 16:26:46 dillo Exp $

  detector.c -- alloc/free detector
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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



#include <stdlib.h>

#include "detector.h"
#include "xmalloc.h"



void
detector_free(detector_t *d)
{
    free(d->name);
    free(d->author);
    free(d->version);
    array_free(d->rules, detector_rule_finalize);
    free(d->buf);
}



detector_t *
detector_new(void)
{
    detector_t *d;

    d = xmalloc(sizeof(*d));

    d->name = NULL;
    d->author = NULL;
    d->version = NULL;
    d->rules = array_new(sizeof(detector_rule_t));
    d->buf = NULL;
    d->buf_size = 0;

    return d;
}



void
detector_rule_finalize(detector_rule_t *dr)
{
    array_free(dr->tests, detector_test_finalize);
}



void
detector_test_finalize(detector_test_t *dt)
{
    free(dt->mask);
    free(dt->value);
}
