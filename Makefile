all:
	gcc mem_alloc_test.c -lpthread -o mem_alloc_test
clean:
	rm -rf mem_alloc_test
