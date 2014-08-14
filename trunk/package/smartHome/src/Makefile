objects = server.o read2.o function.o time.o wifi.o

cc = mips-openwrt-linux-gcc
FLAG=-Wall

smartHome :$(objects)
	$(cc) -o smartHome $(objects) -lpthread
server.o :server.c read.h common.h def.h wifi.h
	$(cc) -c server.c
read2.o :read2.c read.h
	$(cc) -c read2.c
function.o :function.c common.h def.h
	$(cc) -c function.c
time.o :time.c def.h common.h
	$(cc) -c time.c
wifi.o :wifi.c wifi.h def.h
	$(cc) -c wifi.c
.PHONY : clean
clean:
	rm -rf smartHome $(objects)
