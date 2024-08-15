src_files := $(wildcard ./src/*.c)

run: $(src_files)
	gcc -o SCC main.c $(src_files)

clean:
	rm -f $(wildcard *.o) SCC