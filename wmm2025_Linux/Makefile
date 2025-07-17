###################################################
# Setting of commands and file extension from different OS
#####################################################
CLEANUP = rm -f
CLEANUP_DIR = rm -rf
MKDIR = mkdir -p

ifeq ($(OSTYPE),cygwin)
	TARGET_EXTENSION=.out
else ifeq ($(OSTYPE),msys)
	TARGET_EXTENSION=.exe
else ifeq ($(OS),Windows_NT)
	TARGET_EXTENSION=.exe
else
	TARGET_EXTENSION=.out
endif
#####################################################
#C Compiler
#####################################################

CC=gcc
ifeq ($(shell uname -s), Darwin)
	CC=clang
endif

#####################################################
# C Flags
#####################################################

CFLAGS=-std=c99
CFLAGS += -Wall
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -W

#####################################################
# C Library
#####################################################

LDFLAGS =
LIBS = -lm

#####################################################
# PATHS
#####################################################

PATH_MAIN = main
PATH_APP = src
PATH_OUT = build
PATH_DATA = data
PATH_TEST = tests
PATH_UNITY = Unity

#####################################################
# Include Unit Test Directories
#####################################################

INC_DIRS = -Isrc -I$(PATH_UNITY)/src -I$(PATH_UNITY)/extras/fixture/src
SYMBOLS= -DUNITY_FIXTURE_NO_EXTRAS -DUNITY_INCLUDE_CONFIG_H

#####################################################
# SOURCE CODE
#####################################################

SRC_CODE = $(wildcard $(PATH_APP)/*.c)

SRC_OBJ = $(wildcard $(PATH_OUT)/*.o)
SRC_TEST = $(wildcard $(PATH_TEST)/*.c)
SRC_TEST := $(filter-out $(PATH_TEST)/verify.c, $(SRC_TEST))
SRC_TEST_RUNNER = $(wildcard $(PATH_TEST)/test_runners/*.c)


EXE_PT_SET =\
	$(SRC_CODE) \
	$(PATH_MAIN)/wmm_point.c

EXE_FILE_SET =\
	$(SRC_CODE) \
	$(PATH_MAIN)/wmm_file.c

EXE_GRID_SET =\
	$(SRC_CODE) \
	$(PATH_MAIN)/wmm_grid.c

EXE_APP_SET =\
	$(SRC_CODE) \
	$(PATH_MAIN)/app.c

UNIT_TEST_SET =\
	$(SRC_CODE) \
	$(PATH_UNITY)/src/unity.c \
    $(PATH_UNITY)/extras/fixture/src/unity_fixture.c \
    $(SRC_TEST) \
    $(SRC_TEST_RUNNER) \

TEST_EXE_FILENAME = all_tests
TEST_EXE = $(PATH_OUT)/$(TEST_EXE_FILENAME)$(TARGET_EXETENSION)

HRPT_EXE_NAME = wmmhr_point
HRPT_EXE = $(PATH_OUT)/$(HRPT_EXE_NAME)$(TARGET_EXETENSION)

HRFILE_EXE_NAME = wmmhr_file
HRFILE_EXE = $(PATH_OUT)/$(HRFILE_EXE_NAME)$(TARGET_EXETENSION)

HRGRID_EXE_NAME = wmmhr_grid
HRGRID_EXE = $(PATH_OUT)/$(HRGRID_EXE_NAME)$(TARGET_EXETENSION)

PT_EXE_NAME = wmm_point
PT_EXE = $(PATH_OUT)/$(PT_EXE_NAME)$(TARGET_EXETENSION)

FILE_EXE_NAME = wmm_file
FILE_EXE = $(PATH_OUT)/$(FILE_EXE_NAME)$(TARGET_EXETENSION)

GRID_EXE_NAME = wmm_grid
GRID_EXE = $(PATH_OUT)/$(GRID_EXE_NAME)$(TARGET_EXETENSION)

APP_EXE_NAME = app
APP_EXE = $(PATH_OUT)/$(APP_EXE_NAME)$(TARGET_EXETENSION)

#####################################################
# COPY FILES
#####################################################
SRC_TXT = $(wildcard $(PATH_DATA)/*.txt)
SRC_COF = $(wildcard $(PATH_DATA)/*.COF)
coord_txts = $(patsubst $(PATH_DATA)/%, $(PATH_OUT)/%, $(SRC_TXT))
cofs = $(patsubst $(PATH_DATA)/%, $(PATH_OUT)/%, $(SRC_COF))

COPY_FILES = $(coord_txts) $(cofs)


copy_files: $(COPY_FILES)

$(PATH_OUT)/%.txt: $(PATH_DATA)/%.txt
	cp -f $< $@
$(PATH_OUT)/%.COF: $(PATH_DATA)/%.COF
	cp -f $< $@

#####################################################
# MAKE EXE File
#####################################################

wmmhr: $(PATH_OUT) wmmhr_point wmmhr_file wmmhr_grid
wmm: $(PATH_OUT) wmm_point wmm_file wmm_grid app


wmmhr_point: $(PATH_OUT) copy_files $(HRPT_EXE)
wmmhr_file: $(PATH_OUT) copy_files $(HRFILE_EXE)
wmmhr_grid:$(PATH_OUT) copy_files $(HRGRID_EXE)
wmm_point: $(PATH_OUT) copy_files $(PT_EXE)
wmm_file: $(PATH_OUT) copy_files $(FILE_EXE)
wmm_grid:$(PATH_OUT) copy_files $(GRID_EXE)
app:$(PATH_OUT) copy_files $(APP_EXE)

test: $(PATH_OUT) copy_files $(TEST_EXE)

clean:
	$(CLEANUP_DIR) $(PATH_OUT)

$(PATH_OUT):
	$(MKDIR) $(PATH_OUT)


$(PT_EXE):
	${CC} ${CFLAGS} $(INC_DIRS) ${EXE_PT_SET} -o $(PT_EXE) ${LIBS}

$(FILE_EXE):
	${CC} ${CFLAGS} $(INC_DIRS) ${EXE_FILE_SET} -o $(FILE_EXE) ${LIBS}

$(GRID_EXE):
	${CC} ${CFLAGS} $(INC_DIRS) ${EXE_GRID_SET} -o $(GRID_EXE) ${LIBS}

$(APP_EXE):
	${CC} ${CFLAGS} $(INC_DIRS) ${EXE_APP_SET} -o $(APP_EXE) ${LIBS}

$(HRPT_EXE):
	${CC} -DWMMHR ${CFLAGS} $(INC_DIRS) ${EXE_PT_SET} -o $(HRPT_EXE) ${LIBS}

$(HRFILE_EXE):
	${CC} -DWMMHR ${CFLAGS} $(INC_DIRS) ${EXE_FILE_SET} -o $(HRFILE_EXE) ${LIBS}

$(HRGRID_EXE):
	${CC} -DWMMHR ${CFLAGS} $(INC_DIRS) ${EXE_GRID_SET} -o $(HRGRID_EXE) ${LIBS}


$(TEST_EXE):
	${CC} ${CFLAGS} $(INC_DIRS) $(SYMBOLS) ${UNIT_TEST_SET} -o $(TEST_EXE)
	- ./$(TEST_EXE) -v


