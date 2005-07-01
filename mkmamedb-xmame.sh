#!/bin/sh

#  $NiH: mkmamedb-xmame.sh,v 1.3 2005/06/12 14:56:31 dillo Exp $
#
#  mkmamedb-xmame.sh -- create mamedb by calling xmame
#  Copyright (C) 2004, 2005 Dieter Baron and Thomas Klausner
#
#  This file is part of ckmame, a program to check rom sets for MAME.
#  The authors can be contacted at <nih@giga.or.at>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License, version 2, as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

PROG_NAME=xmame
PROG_VERSION=`xmame --version 2>/dev/null | sed 's/.* version \([^ ]*\).*/\1/'`

if [ -z "$PROG_VERSION" ]
then
    echo "$0: cannot determine xmame version" >&2
    exit 1
fi

if [ "$PROG_VERSION" '<' '0.84.1' ]
then
    LIST=-li
else
    LIST=-lx
fi

xmame $LIST 2>/dev/null \
   | mkmamedb --prog-name "$PROG_NAME" --prog-version "$PROG_VERSION" "$@"
