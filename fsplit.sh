#!/bin/bash
#mv fsplit1.txt patches-all/fsplit1.txt
#mv fsplit2.txt patches-all/fsplit2.txt
#mv fsplit3.txt patches-all/fsplit3.txt
cd /root/machinex/patches-all/
mkdir 1patches
mkdir 2patches
mkdir 3patches
cat fsplit1.txt | xargs mv -t $(pwd)/1patches
cat fsplit2.txt | xargs mv -t $(pwd)/2patches
cat fsplit3.txt | xargs mv -t $(pwd)/3patches