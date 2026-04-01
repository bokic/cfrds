CC = clang
CFLAGS = -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -Wall -Wextra
FUZZER_CXXFLAGS = -fsanitize=fuzzer,address,undefined

SRCS = ../src/cfrds_buffer.c ../src/wddx.c ../src/cfrds.c ../src/cfrds_http.c
INC = -I../include -I../include/internal -I/usr/include/libxml2 -I/usr/include/json-c
LDFLAGS = -lxml2 -ljson-c

all: fuzz_targets

fuzz_targets: fuzz_rds_string

fuzz_rds_string: $(SRCS)
	rm -f ./fuzz_rds_string
	$(CC) $(FUZZER_CXXFLAGS) $(INC) cfrds_fuzz.c $(SRCS) $(CFLAGS) $(LDFLAGS) -o $@

# ... add others similarly

.PHONY: rebuild
