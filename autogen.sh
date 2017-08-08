#!/bin/bash
set -e
aclocal
autoconf
automake --add-missing
