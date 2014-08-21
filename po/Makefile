# Copyright (C) 2006-2010 G.P. Halkes
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
.POSIX:

.PHONY: linguas lingua-install

.SUFFIXES: .mo .po

LINGUAOBJECTS=$(LINGUAS:=.mo)

linguas: $(LINGUAOBJECTS)

.po.mo:
	msgfmt -o $@ $<

lingua-install: $(LINGUAOBJECTS)
	for lingua in $(LINGUAS) ; do \
		if [ ! -f "$${lingua}.mo" ] ; then continue ; fi ;\
		$(INSTALL) -d "$(LOCALEDIR)/$${lingua}/LC_MESSAGES" ;\
		$(INSTALL) -m 644 "$${lingua}.mo" "$(LOCALEDIR)/$${lingua}/LC_MESSAGES/dwdiff.mo" ;\
	done

