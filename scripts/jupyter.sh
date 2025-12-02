#!/bin/bash
export LD_PRELOAD=$HOME/lib64/libzmq.so
IP=$(ip route get 1.2.3.4 | head -1 | awk '{print $7}')
echo $IP
jupyter-notebook --ip $IP &> jup.out &
export LD_PRELOAD=""