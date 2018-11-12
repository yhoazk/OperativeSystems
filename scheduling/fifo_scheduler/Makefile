all: clean comp run

clean:
	rm -rf ./*.exe

comp:
	gcc -o fifo_scheduler.exe fifo_scheduler.c


run:
	./fifo_scheduler.exe 

get_ppc:
	gcc -E fifo_scheduler.c > fifo_scheduler.ppc

