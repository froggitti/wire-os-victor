#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh \
		-c Release \
        -DANKI_RESOURCE_ESCAPEPOD=1 \
		-DANKI_DISABLE_ALEXA=0 \
        "$@"