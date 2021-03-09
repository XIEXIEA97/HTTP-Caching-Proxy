#!/bin/sh
cat tests/testget1 | nc -C localhost 12345
cat tests/testpost | nc -C localhost 12345
cat tests/testconnect | nc -q 1 localhost 12345
cat tests/testinvalid | nc -C localhost 12345
cat tests/testchunk_send | nc -C localhost 12345
cat tests/testchunk_recv | nc -C localhost 12345
