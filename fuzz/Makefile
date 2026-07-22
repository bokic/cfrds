CC = clang
CFLAGS = -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -Wall -Wextra
FUZZER_CXXFLAGS = -fsanitize=fuzzer,address,undefined

SRCS = ../src/cfrds.c ../src/cfrds_http.c
INC = -I../include -I../include/internal -I../build/include -I/usr/include/libxml2 -I/usr/include/json-c
LDFLAGS = -lxml2 -ljson-c

all: fuzz_targets

version:
	@if [ ! -f ../build/include/version.h ]; then \
		mkdir -p ../build/include; \
		echo '#pragma once' > ../build/include/version.h; \
		echo '#define CFRDS_VERSION "0.0.0-fuzz"' >> ../build/include/version.h; \
		echo '#define CFRDS_VERSION_MAJOR 0' >> ../build/include/version.h; \
		echo '#define CFRDS_VERSION_MINOR 0' >> ../build/include/version.h; \
		echo '#define CFRDS_VERSION_PATCH 0' >> ../build/include/version.h; \
		echo '#define CFRDS_VERSION_INT 0' >> ../build/include/version.h; \
	fi

fuzz_targets: fuzz_rds_string

fuzz_rds_string: version $(SRCS)
	rm -f ./fuzz_rds_string
	$(CC) $(FUZZER_CXXFLAGS) $(INC) cfrds_fuzz.c $(SRCS) $(CFLAGS) $(LDFLAGS) -o $@

# ... add others similarly

.PHONY: rebuild
