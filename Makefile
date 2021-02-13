ENTRY  = main.c
OPTS   = -g -O0 -Wall
LIBS   = -L /usr/local/include -lwiringPi -lcrypt -lpthread -lm -lrt
TARGET = pwm_fan_control

# COMPILE:
compile:
	gcc ${OPTS} ${ENTRY} ${LIBS} -o ${TARGET}
	chmod +x pwm_fan_control

compile-debug:
	gcc -DDEBUG ${OPTS} ${ENTRY} ${LIBS} -o ${TARGET}
	chmod +x pwm_fan_control

clean:
	rm pwm_fan_control

# INSTALL:
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
