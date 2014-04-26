#
# Copyright (c) 2014, Alex Taradov <taradov@gmail.com>
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

USE_LIBCORE = 0

SRCS = \
  main.c \
  config.c \
  soc.c \
  utils.c \
  events.c \
  medium.c \
  noise.c \
  sniffer.c \
  trx.c \
  mem.c \
  sys_ctrl.c \
  sys_timer.c

HEADERS = \
  main.h \
  config.h \
  soc.h \
  utils.h \
  events.h \
  medium.h \
  noise.h \
  sniffer.h \
  trx.h \
  mem.h \
  sys_ctrl.h \
  sys_timer.h

LIBS = -lm

ifeq ($(USE_LIBCORE), 1)
  DEFINES += -DUSE_LIBCORE
  HEADERS += libcore/core.h
  LIBS += -Llibcore -lcore
else
  SRCS += core.c
  HEADERS += core.h
endif

all: netsim

netsim: $(SRCS) $(HEADERS)
	gcc -W -Wall -std=gnu99 -O3 $(DEFINES) $(SRCS) $(LIBS) -o netsim

clean:
	rm -f netsim netsim.exe

