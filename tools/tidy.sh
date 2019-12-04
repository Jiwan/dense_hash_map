#!/usr/bin/env sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# The compile_commands.json has relative path from the build directory.
# This confuses clang-tidy fixes...
# So it is better to execute in that directory directly.
pushd $DIR/../build
$DIR/run-clang-tidy.py -p $DIR/../build -header-filter='.*' -fix 
popd
