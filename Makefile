DIR_OBJ = ./obj
DIR_BIN = ./bin

OBJ_C = $(wildcard ${DIR_FONTS}/*.c ${DIR_OBJ}/*.c)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

ENTRY  = main.c
OPTS   = -g -O0 -Wall
LIBS   = -L /usr/local/include -lwiringPi
TARGET = pwm_fan_control

compile:
	gcc ${OPTS} ${ENTRY} ${LIBS} -o ${TARGET}
	chmod +x pwm_fan_control

compile-debug:
	gcc -DDEBUG ${OPTS} ${ENTRY} ${LIBS} -o ${TARGET}
	chmod +x pwm_fan_control

clean:
	rm pwm_fan_control

install:
	gcc ${OPTS} ${ENTRY} ${LIBS} -o ${TARGET}
	chmod +x pwm_fan_control
	cp ${TARGET} /usr/sbin/${TARGET}

	cp ${TARGET}.service /etc/systemd/system/${TARGET}.service
	service ${TARGET} start
	systemctl enable ${TARGET}.service

uninstall:

	systemctl -f stop ${TARGET}.service
	systemctl disable ${TARGET}.service
	systemctl daemon-reload

	rm -f /etc/systemd/system/${TARGET}.service
	rm -f /usr/sbin/${TARGET}
