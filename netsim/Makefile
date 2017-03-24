#
# Copyright (c) 2014-2017, Alex Taradov <alex@taradov.com>
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
#
###############################################################################

SRCS = \
  main.c \
  config.c \
  soc.c \
  core.c \
  utils.c \
  events.c \
  medium.c \
  noise.c \
  sniffer.c \
  trx.c \
  sys_ctrl.c \
  sys_timer.c

HEADERS = \
  main.h \
  config.h \
  soc.h \
  core.h \
  utils.h \
  events.h \
  medium.h \
  noise.h \
  sniffer.h \
  trx.h \
  io_ops.h \
  sys_ctrl.h \
  sys_timer.h

LIBS = -lm

CFLAGS += -W -Wall -std=gnu11 -O3
CFLAGS += -fgnu89-inline
CFLAGS += -D__STDC_FORMAT_MACROS=1

all: netsim

netsim: $(SRCS) $(HEADERS)
	gcc $(CFLAGS) $(DEFINES) $(SRCS) $(LIBS) -o netsim

clean:
	-rm -f netsim netsim.exe

