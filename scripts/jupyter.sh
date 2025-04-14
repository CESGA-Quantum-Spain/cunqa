#!/bin/bash

export INSTALL_PATH=$STORE/works/cunqa/installation
 
export PATH=$PATH:$INSTALL_PATH/bin
 
export INFO_PATH=$STORE/.cunqa/qpus.json
 
IP=$(ip route get 1.2.3.4 | head -1 | awk '{print $7}')
LD_DEBUG=all jupyter-notebook --ip $IP &> jup.out &
export LD_PRELOAD=""