objects = mygpio_app.o

cc = mips-openwrt-linux-gcc
FLAG=-Wall

mygpio :$(objects)
	$(cc) -o mygpio_app  $(objects)
mygpio.o:mygpio.c
	$(cc) -c mygpio.c
.PHONY : clean
clean:
	rm -rf mygpio_app $(objects)
