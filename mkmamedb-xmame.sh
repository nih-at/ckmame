#!/bin/sh

#  $NiH: mkmamedb.c,v 1.23 2004/04/22 11:21:44 dillo Exp $
#
#  mkmamedb-xmame.sh -- create mamedb by calling xmame
#  Copyright (C) 2004 Dieter Baron and Thomas Klausner
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

(xmame --version 2>/dev/null \
	| sed 's/\([^ ]*\).*version \([^ ]*\).*/emulator (@       name \1@        version \2@)@/' \
	| tr @ '\012'; \
 xmame -li 2>/dev/null) \
	| mkmamedb "$@"
