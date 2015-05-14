#!/bin/sh

mkdir -p build
touch build/tup.config.in
autoreconf -fi
