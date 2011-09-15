CFLAGS=-Wall -g -c -fPIC -DLINUX
CXXFLAGS=-Wall -g -c -fPIC -DLINUX
CC=gcc
CXX=g++


COBJS=
ALLEXE=dense
DENSEOBJS=dense_main.o sparse_matrix.o
DENSEHDRS=sparse_matrix.h
CXXOBJS=$(DENSEOBJS)

.PHONY: dense clean

all: $(ALLEXE)

dense: $(DENSEOBJS) $(DENSEHDRS)
	g++ -g -o dense \
		-lboost_filesystem \
		$(DENSEOBJS)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CXXOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	-@ rm -r *.o
	-@ rm $(ALLEXE)
.PHONY: clean

