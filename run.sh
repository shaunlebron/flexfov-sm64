#!/bin/bash

set -ex

make all

pushd sm64-port
gmake && build/us_pc/sm64.us.f3dex2e
popd
