# === Build config ============================================================
CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -Werror -pedantic -g

INCDIR  := include
SRCDIR  := src
BINDIR  := bin
OBJDIR  := .obj
OBJDIRPIC := .objpic

# === Objects / libs ==========================================================
COMMON_OBJS := $(OBJDIR)/common/util.o $(OBJDIR)/graph/graph.o
LIBCOMMON   := $(BINDIR)/libcommon.a

# === Default target ==========================================================
.PHONY: all clean fmt test asan tsan ubsan
all: $(LIBCOMMON) $(BINDIR)/detect_wfg $(BINDIR)/detect_matrix $(BINDIR)/ddetect $(BINDIR)/demo_deadlock $(BINDIR)/libdd.so

# === Dirs ====================================================================
$(BINDIR) $(OBJDIR) $(OBJDIR)/common $(OBJDIR)/graph $(OBJDIRPIC) $(OBJDIRPIC)/common $(OBJDIRPIC)/graph:
	mkdir -p $@

# === Compile objects (static) ================================================
$(OBJDIR)/common/%.o: $(SRCDIR)/common/%.c | $(OBJDIR) $(OBJDIR)/common
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/graph/%.o: $(SRCDIR)/graph/%.c | $(OBJDIR) $(OBJDIR)/graph
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# === Static lib ==============================================================
$(LIBCOMMON): $(COMMON_OBJS) | $(BINDIR)
	ar rcs $@ $^

# === Binaries ================================================================
$(BINDIR)/detect_wfg: $(SRCDIR)/wfg/detect_wfg.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/detect_matrix: $(SRCDIR)/matrix/detect_matrix.c | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@

$(BINDIR)/ddetect: $(SRCDIR)/cli/ddetect.c | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@

$(BINDIR)/demo_deadlock: $(SRCDIR)/demo/demo_deadlock.c | $(BINDIR)
	$(CC) $(CFLAGS) -pthread -I$(INCDIR) $< -o $@

# Optional tool: quick WFG checker
$(BINDIR)/wfg_check: tools/wfg_check.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

# === PIC objects for shared lib (libdd.so) ===================================
PIC_CFLAGS := -fPIC
PIC_COMMON_OBJS := $(OBJDIRPIC)/common/util.o $(OBJDIRPIC)/graph/graph.o

$(OBJDIRPIC)/common/%.o: $(SRCDIR)/common/%.c | $(OBJDIRPIC) $(OBJDIRPIC)/common
	$(CC) $(CFLAGS) $(PIC_CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIRPIC)/graph/%.o: $(SRCDIR)/graph/%.c | $(OBJDIRPIC) $(OBJDIRPIC)/graph
	$(CC) $(CFLAGS) $(PIC_CFLAGS) -I$(INCDIR) -c $< -o $@

$(BINDIR)/libdd.so: $(SRCDIR)/runtime/libdd.c $(PIC_COMMON_OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -shared -fPIC -I$(INCDIR) $^ -o $@ -ldl -pthread

# === Utilities ===============================================================
fmt:
	@command -v clang-format >/dev/null 2>&1 && \
		clang-format -i $(shell git ls-files '*.c' '*.h') || \
		echo "clang-format not found, skip."

test: all
	./scripts/run_tests.sh

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean all

tsan: CFLAGS += -fsanitize=thread
tsan: clean all

ubsan: CFLAGS += -fsanitize=undefined
ubsan: clean all

clean:
	rm -rf $(OBJDIR) $(OBJDIRPIC) $(BINDIR) demo_deadlock libdd.so
