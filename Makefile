CFLAGS ?= -Wall -O2
TARGETS = efiextract

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
