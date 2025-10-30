CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -Werror -pedantic -g
demo_deadlock: src/demo/demo_deadlock.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
INCDIR  := include
SRCDIR  := src
BINDIR  := bin
OBJDIR  := .obj

COMMON_OBJS := $(OBJDIR)/common/util.o $(OBJDIR)/graph/graph.o
LIBCOMMON  := $(BINDIR)/libcommon.a

.PHONY: all clean fmt asan tsan ubsan
#
all: $(LIBCOMMON) $(BINDIR)/detect_wfg $(BINDIR)/detect_matrix

$(BINDIR) $(OBJDIR) $(OBJDIR)/common $(OBJDIR)/graph:
	mkdir -p $@

$(OBJDIR)/common/%.o: $(SRCDIR)/common/%.c | $(OBJDIR) $(OBJDIR)/common
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/graph/%.o: $(SRCDIR)/graph/%.c | $(OBJDIR) $(OBJDIR)/graph
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(LIBCOMMON): $(COMMON_OBJS) | $(BINDIR)
	ar rcs $@ $^

# Binaries (sẽ dùng ở Day 2/3/B)
$(BINDIR)/wfg_check: tools/wfg_check.c $(LIBCOMMON)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/detect_wfg: $(SRCDIR)/wfg/detect_wfg.c $(LIBCOMMON)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/detect_matrix: $(SRCDIR)/matrix/detect_matrix.c
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@

fmt:
	@command -v clang-format >/dev/null 2>&1 && clang-format -i $(shell git ls-files '*.c' '*.h') || echo "clang-format not found"

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean all
tsan: CFLAGS += -fsanitize=thread
tsan: clean all
ubsan: CFLAGS += -fsanitize=undefined
ubsan: clean all

clean:
	rm -rf $(OBJDIR) $(BINDIR)
