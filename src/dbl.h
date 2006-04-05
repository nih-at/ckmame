#ifndef _HAD_DBL_H
#define _HAD_DBL_H

/*
  $NiH: dbl.h,v 1.4 2006/03/17 10:59:27 dillo Exp $

  dbl.h -- generic low level data base routines
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include DDB_INCLUDE

#define DDB_READ	0x0	/* open readonly */
#define DDB_WRITE	0x1	/* open for writing */

#define DDB_FORMAT_VERSION	11 /* version of ckmame database format */

#define DDB_KEY_DAT		"/dat"
#define DDB_KEY_DB_VERSION	"/ckmame"
#define DDB_KEY_HASH_TYPES	"/hashtypes"
#define DDB_KEY_LIST_DISK	"/list/disk"
#define DDB_KEY_LIST_GAME	"/list/game"
#define DDB_KEY_LIST_SAMPLE	"/list/sample"

#define DDB_DEFAULT_DB_NAME	"mame.db"



int ddb_check_version(DB *, int);
int ddb_close(DB *);
const char *ddb_error(void);
const char *ddb_error_l(void);
int ddb_foreach(DB *, int (*)(const DBT *, const DBT *, void *), void *);
int ddb_init_db(DB *);
int ddb_insert(DB *, const char *, const DBT *);	/* API version */
int ddb_insert_l(DB *, DBT *, const DBT *);		/* backend version */
int ddb_lookup(DB *, const char *, DBT *);		/* API version */
int ddb_lookup_l(DB *, DBT *, DBT *);			/* backend version */
DB* ddb_open(const char *, int);

#endif /* dbl.h */
