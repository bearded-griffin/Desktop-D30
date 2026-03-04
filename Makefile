#   This file is part of Desktop-D30
#   Copyright (C) 2026 bearded-griffin
# 
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation version 3 of the License.
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Makefile wrapper for Python Invoke tasks

.PHONY: all setup build clean run test release appimage cross-windows coverage install metadata

all: build

setup:
	pip install invoke

build:
	inv build

clean:
	inv clean

run:
	inv run

test:
	inv test

release:
	inv release

appimage:
	inv appimage

cross-windows:
	inv cross-windows

coverage:
	inv coverage

install:
	sudo inv install

metadata:
	inv metadata
