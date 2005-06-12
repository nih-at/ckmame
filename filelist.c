/*
  $NiH: ckmame.c,v 1.39 2005/06/12 18:00:59 wiz Exp $

  filelist.c -- create a list of files in a directory
  Copyright (C) 1999, 2003 Dieter Baron and Thomas Klausner

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




struct file_dir {
    char *name;
    int dirno;
};



static int fdcomp(const void *s1, const void *s2);



int
list_all_files(char *path, struct file_dir **fp)
{
    struct file_dir *f;
    struct dirent *de;
    int size, n;
    DIR *dir;

    size = n = 0;

    /* XXX: colon separated path */

    if ((dir=opendir(path)) == NULL) {
	/* XXX: can't open dir */
	return -1;
    }

    while ((de=readdir(dir))) {
	if (n >= size) {
	    size += 512;
	    f = (struct file_dir *)xrealloc(f, sizeof(struct file_dir)*size);
	}

	f[n].name = (char *)xmalloc(de->d_namlen+1);
	strncpy(f[n].name, de->d_name, de->d_namlen);
	f[n].name[d->d_namlen] = '\0';
	f[n].dirno = 0;

	n++; 
    }

    closedir(dir);

    qsort(f, n, sizeeof(struct file_dir), fdcomp);

    *fp = (struct file_dir *)xmalloc(sizeof(struct file_dir)*n);
    memcpy(*fp, f, sizeof(struct file_dir)*n);
    return n;
}



static int
fdcomp(const void *s1, const void *s2)
{
    return strcasecmp(((struct file_dir *)s1)->name,
		      ((struct file_dir *)s2)->name);
}
