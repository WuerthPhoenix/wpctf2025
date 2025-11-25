SOURCES = main.c info.c
FLAGS = -DFLAG='"${FLAG}"' -DPASSWORD='"${PASSWORD}"'
TARGET = system_information

${TARGET}: ${SOURCES}
	clang -o ${TARGET} ${SOURCES} ${FLAGS}
