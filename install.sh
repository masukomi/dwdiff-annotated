#!/bin/sh
# Copyright (C) 2009-2010 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Set zsh to emulate sh (otherwise all kinds of eval's wont work)
[ -n "${ZSH_VERSION}" ] && emulate sh

unset directory sources dest mode owner group

# Provide an alternative to !
not() {
	if "$@" ; then
		false
	else
		true
	fi
}

# Default mode for install is rwxr-xr-x
mode="0755"

while [ $# -gt 0 ]
do
	case "$1" in
		-d)
			directory="1"
			;;
		-m)
			mode="$2"
			shift
			;;
		-m*)
			mode=`echo "$1" | sed 's/^-m//'`
			;;
		-o)
			owner="$2"
			shift
			;;
		-o*)
			owner=`echo "$1" | sed 's/^-o//'`
			;;
		-g)
			group="$2"
			shift
			;;
		-g*)
			group=`echo "$1" | sed 's/^-g//'`
			;;
		*)
			break
			;;
	esac
	shift
done

if [ "1" = "${directory}" ] ; then
	if [ $# -eq 0 ] ; then
		echo "No directory names supplied" >&2
		exit 1
	fi

	# Create directories
	while [ $# -gt 0 ]
	do
		if [ ! -d "$1" ] ; then
			# Find name of dir to apply chown/chgrp -R to, i.e., the first part
			# of the path that does not yet exist.
			firstDir="$1"
			testDir=`dirname "$1"`
			while [ ! -d "${testDir}" ] ; do
				firstDir="${testDir}"
				testDir=`dirname "${firstDir}"`
			done

			# Make the directory
			if not mkdir -p "$1" ; then
				exit 1
			fi

			if not chmod -R "${mode}" "${firstDir}" ; then
				exit 1
			fi

			# Set owner and group, if specified
			if [ -n "${owner}" ] ; then
				if not chown -R "${owner}" "${firstDir}" ; then
					exit 1
				fi
			fi
			if [ -n "${group}" ] ; then
				if not chgrp -R "${group}" "${firstDir}" ; then
					exit 1
				fi
			fi
		fi
		shift
	done
	exit
fi

if [ $# -lt 2 ] ; then
	echo "No destination and/or source(s) supplied" >&2
	exit 1
fi

install_file() {
	if [ -d "$2" ] ; then
		echo "Cannot install $2: directory in the way" >&2
		exit 1
	fi

	# Copy file
	if not cp "$1" "$2" ; then
		exit 1
	fi

	# Set attributes
	if [ -n "${mode}" ] ; then
		if not chmod "${mode}" "$2" ; then
			exit 1
		fi
	fi
	if [ -n "${owner}" ] ; then
		if not chown "${owner}" "$2" ; then
			exit 1
		fi
	fi
	if [ -n "${group}" ] ; then
		if not chgrp "${group}" "$2" ; then
			exit 1
		fi
	fi
}

eval dest="\"\${$#}\""

if [ -d "${dest}" ] ; then

	# Loop over all sources, but do not process destination as source
	while [ $# -gt 1 ]
	do
		install_file "$1" "${dest}/`basename \"$1\"`"
		shift
	done
elif [ $# -eq 2 ] ; then
	install_file "$1" "${dest}"
else
	echo "Multiple sources specified, but destination is not a directory" >&2
	exit 1
fi
