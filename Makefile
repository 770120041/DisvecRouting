CC=/usr/bin/gcc
CC_OPTS=-g3
CC_LIBS=
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

# clean is not a file
.PHONY=clean all

all:  vec_router  

vec_router: vec_router.c
	@${CC} ${CC_ARGS} -pthread -o vec_router vec_router.c
	

clean:
	@rm -f  vec_router *.o 
# @rm -r *.dSYM
