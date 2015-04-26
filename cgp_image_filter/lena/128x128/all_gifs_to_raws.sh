#!/bin/sh

# convert all files gif from the current directory into raw
for i in *
do 
  gif2raw lena-128x128.gif > lena-128x128.raw
done
