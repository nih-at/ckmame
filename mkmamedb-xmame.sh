#!/bin/sh

(xmame --version 2>/dev/null \
	| sed 's/\([^ ]*\).*version \([^ ]*\).*/emulator (@       name \1@        version \2@)@/' \
	| tr @ '\012'; \
 xmame -li 2>/dev/null) \
	| mkmamedb "$@"
