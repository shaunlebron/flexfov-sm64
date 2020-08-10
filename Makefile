
sm64-port/src/game/flexfov.c: flexfov.c
	cp $< $@

sm64-port/src/game/flexfov.h: flexfov.h
	cp $< $@

sm64-port/src/game/flexfov.frag: flexfov.frag
	glslangValidator $<
	awk '{ print "\"" $$0 "\\n\"" }' $< > $@


.PHONY: all
all: sm64-port/src/game/flexfov.c sm64-port/src/game/flexfov.h sm64-port/src/game/flexfov.frag
