#!/bin/bash
./rtm32 -d telnet > emulator.log 2>&1 &
PID=$!
sleep 1
echo -e "help\nquit\n" | nc localhost 4444 > debugger.out
kill $PID
