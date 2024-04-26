# projects

DOARR_TUNING = ..

DOARR = doarr
NOARR_STRUCTURES = noarr-structures
SNAKETONGS = snaketongs


# flags

CXXFLAGS = -O3 -Wall -Wextra -pedantic
CXXFLAGS_ = -std=c++20 -I${DOARR_TUNING}/include -I${DOARR}/include -I${NOARR_STRUCTURES}/include -I${SNAKETONGS}/include ${CXXFLAGS}
DOARR_LIBS = ${DOARR}/build/doarr.o
SNAKETONGS_LIBS = ${SNAKETONGS}/subproc.o
LIBS_ = ${DOARR_LIBS} ${SNAKETONGS_LIBS} ${LIBS}


# tools

PYTHON = python3
DCC = ${DOARR}/build/dcc


# build steps

program: program.cpp kernel.o ${LIBS_}
	${CXX} ${CXXFLAGS_} program.cpp kernel.o ${LIBS_} -o $@

kernel.o: kernel.cpp ${DCC}
	${DCC} ${CXXFLAGS_} -c kernel.cpp


# dependencies

${DOARR_LIBS}: doarr ;

${SNAKETONGS_LIBS}: snaketongs ;

doarr:
	make -C ${DOARR}

snaketongs:
	make -C ${SNAKETONGS}

.PHONY: doarr snaketongs


# test run

ENV = env
PROGRAM_ARGS = matrices 512

run: program ${ENV}/pyvenv.cfg matrices
	. ${ENV}/bin/activate && ./program ${PROGRAM_ARGS}

${ENV}/pyvenv.cfg:
	${PYTHON} -m venv ${ENV}
	${ENV}/bin/pip install opentuner

matrices: gen-matrices.py
	./gen-matrices.py matrices 512
