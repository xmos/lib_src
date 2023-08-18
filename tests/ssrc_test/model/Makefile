FLAGS = -Os -D__int64=int64_t -I ../../../lib_src/src/multirate_hifi
CC = gcc
SRCS = *.c
BIN = ssrc_golden

all: ${BIN}

${BIN}: ${SRCS}
	cc ${SRCS} ${FLAGS} -o $@
	
clean:
	rm ${BIN}
