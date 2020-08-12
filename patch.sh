#!/bin/bash

set -ex

repo=sm64-port

# clone sm64 if needed
if [ ! -d $repo ]; then
  git clone https://github.com/Zoltar007/target_osx.git $repo
fi

# clear any changes in working directory
pushd $repo
git checkout -- .
popd

# patch other files
awk -v root="$repo" -f patch.awk patch.diff | ed

# extra patch
if [ ! -z "$1" ]; then
  awk -v root="$repo" -f patch.awk "$1" | ed
fi

