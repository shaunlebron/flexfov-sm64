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

# dev patch
#awk -v root="$repo" -f patch.awk scratch/patchdev.diff | ed

