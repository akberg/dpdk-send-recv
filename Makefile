# binary name
APP = helloworld

# all source are stored in SRCS-y
SRCS-y := \
	src/main.cpp \
	src/pkt_display.cpp \
	src/sender.cpp \
	src/receiver.cpp \
	src/fizzbuzz.cpp \
	src/forwarder.cpp

PKG_CONFIG_PATH = /usr/local/lib64/pkgconfig

PKGCONF ?= PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk) -I./include
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

CFLAGS += -DALLOW_EXPERIMENTAL_API

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

LCORES ?=4

# Run sender
run: send
valgrind: valgrind-send

send: build/$(APP)-shared
	sudo PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) LD_LIBRARY_PATH=/usr/local/lib64 build/$(APP) -l 0-$(LCORES) -a 0000:18:00.0 -- -S
# Run receiver
recv: build/$(APP)-shared
	sudo PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) LD_LIBRARY_PATH=/usr/local/lib64 build/$(APP) -l 0-$(LCORES) -- -R
# Run forwarder
fwrd: build/$(APP)-shared
	sudo PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) LD_LIBRARY_PATH=/usr/local/lib64 build/$(APP) -l 0-$(LCORES) -- -F

# Clone sources to receiver host
OTHER_HOST = doca@15.15.15.7
clone:
	scp Makefile 	$(OTHER_HOST):~/send-recv/Makefile
	scp -r src/  	$(OTHER_HOST):~/send-recv/
	scp -r include/ $(OTHER_HOST):~/send-recv/
NIC = ubuntu@128.141.206.239
clone-bf:
	scp Makefile 	$(NIC):~/dpdk-fizzbuzz/Makefile
	scp -r src/  	$(NIC):~/dpdk-fizzbuzz/
	scp -r include/ $(NIC):~/dpdk-fizzbuzz/


.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true
