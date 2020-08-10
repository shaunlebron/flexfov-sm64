
sm64-port/src/game/flexfov.c: flexfov.c
	cp flexfov.c sm64-port/src/game

sm64-port/src/game/flexfov.h: flexfov.h
	cp flexfov.h sm64-port/src/game

sm64-port/src/game/flexfov_shaders.c: quad.vert quad.frag
	./quad.sh > sm64-port/src/game/flexfov_shaders.c


.PHONY: all
all: sm64-port/src/game/flexfov.c sm64-port/src/game/flexfov.h sm64-port/src/game/flexfov_shaders.c
