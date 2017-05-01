#!/usr/bin/bash
i=0
while true; do
echo -e -n '\x'$i | nc 192.168.1.102 8080
sleep 1
i=$[$i+1]
echo $i
done

