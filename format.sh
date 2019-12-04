#!/usr/bin/env bash

for folder in include/ tests/
do 
find $folder -regex '.*\.\(cpp\|hpp\|cu\|c\|h\)' -exec clang-format -style=file -i {} \;
done
