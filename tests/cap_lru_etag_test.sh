#!/bin/sh
cat tests/testget1 | nc -C localhost 12345 # not in cache
cat tests/testget2 | nc -C localhost 12345 # not in cache
cat tests/testget1 | nc -C localhost 12345 # not in cache
cat tests/testget3 | nc -C localhost 12345 # not in cache
cat tests/testget1 | nc -C localhost 12345 # should be in cache
cat tests/testget2 | nc -C localhost 12345 # not in cache
cat tests/testget3 | nc -C localhost 12345 # not in cache
