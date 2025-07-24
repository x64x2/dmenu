#!/bin/bash

# torssh - Makes a SSH conection through TOR
#
# Copyright (C) 2015 David Moreno <dmc.coder@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

NARGS=1
PNAME=`basename $0`

#Error codes.
ERROR_OK=0
ERROR_USER=1
ERROR_HOSTNAME=2
ERROR_URL=3

#Error messages.
ERR_MSG=(
""
"ERROR: Missing username"
"ERROR: Missing hostname"
"ERROR: Invalid URL"
)

#The classic help message.
function show_help {
	help="$PNAME - Makes a SSH conection through TOR\n"\
"Usage: $PNAME [USR@]URL\n\n"\
"\tUSR\tLogin name (optional)\n"\
"\tURL\tAddress to connect (mandatory)"

	echo -e $help
}

#Displays an error message and exits with an error code.
function error_exit {
	echo "$PNAME: "${ERR_MSG[$1]}
	exit $1
}

#Handles the argument.
[ $# -ne $NARGS ] && { show_help; exit $ERROR_OK; }
[[ $1 =~ ^@ ]] && error_exit $ERROR_USER
[[ $1 =~ @$ ]] && error_exit $ERROR_HOSTNAME
[[ $1 =~ .+@.+ ]] && {
	USR=`echo "$1"|cut -f1 -d'@'`
	URL=`echo "$1"|cut -f2 -d'@'`
} || {
	USR=`whoami`
	URL=$1
}

#Avoids DNS leaking.
IP=`tor-resolve $URL 2>/dev/null`
[ $? -gt 0 ] && error_exit $ERROR_URL

#Connects through Tor.
proxychains ssh $USR@$IP
exit $ERROR_OK
