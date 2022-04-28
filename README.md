# jemalloc_micro_benchmark
This program can be used to monitor memory usage with/without jemalloc

step1 setup jemalloc:
```
# wget https://github.com/jemalloc/jemalloc/archive/refs/tags/5.2.1.tar.gz
# cd jemalloc-5.2.1
# ./autogen.sh
# ./configure --prefix=/home/mma/jemalloc-5.2.1-install
# make; make install
# export LD_PRELOAD=/home/mma/jemalloc-5.2.1-install/lib/libjemalloc.so
//check if jemalloc work normally
# lsof -n |grep jemalloc
```

step2 run micro benchmark with/without jemalloc
```
// input thread num, malloc size, bin number
# ./mem_alloc_test 10 1000 100 
```
