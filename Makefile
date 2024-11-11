CC = /usr/bin/gcc
C_FLAGS = -fdiagnostics-color=always

TEST_APP := massagequeue

ifeq($(TEST_APP), pipe)
C_SRCS =  main.c
O_SRCS =  main
else ifeq ($(TEST_APP) , massagequeue)
C_SRCS = pipe/pipe
O_SRCS = /
C_SRCS += sharedMem/reader/main.c
C_SRCS += sharedMem/writer/main.c
C_SRCS += massagequeue/reader/main.c
C_SRCS += massagequeue/writer/main.c

/usr/bin/gcc -fdiagnostics-color=always -g C_SRCS -o O_SRCS