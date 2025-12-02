.DEFAULT_GOAL := all

CC       = cc
CFLAGS   = -std=c11 -O2 -Wall -Wextra -Werror -pedantic -g
INCDIR   = include
SRCDIR   = src
OBJDIR   = .obj
OBJDIRPIC= .objpic
BINDIR   = bin

LIBCOMMON = $(BINDIR)/libcommon.a

all: $(LIBCOMMON) \
     $(BINDIR)/detect_wfg $(BINDIR)/detect_matrix $(BINDIR)/ddetect \
     $(BINDIR)/demo_deadlock $(BINDIR)/demo_deadlock3 $(BINDIR)/demo_nocycle \
     $(BINDIR)/wfg_to_dot $(BINDIR)/wfg_check \
     $(BINDIR)/libdd.so

# --- dirs ---
$(BINDIR) $(OBJDIR)/common $(OBJDIR)/graph $(OBJDIRPIC)/common $(OBJDIRPIC)/graph:
	@mkdir -p $@

# --- libcommon.a ---
$(OBJDIR)/common/util.o: $(SRCDIR)/common/util.c | $(OBJDIR)/common
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/graph/graph.o: $(SRCDIR)/graph/graph.c | $(OBJDIR)/graph
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(LIBCOMMON): | $(BINDIR) $(OBJDIR)/common/util.o $(OBJDIR)/graph/graph.o
	ar rcs $@ $(OBJDIR)/common/util.o $(OBJDIR)/graph/graph.o

# --- apps ---
$(BINDIR)/detect_wfg: $(SRCDIR)/wfg/detect_wfg.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/detect_matrix: $(SRCDIR)/matrix/detect_matrix.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/ddetect: $(SRCDIR)/cli/ddetect.c | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@

$(BINDIR)/demo_deadlock: $(SRCDIR)/demo/demo_deadlock.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -pthread -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/demo_deadlock3: $(SRCDIR)/demo/demo_deadlock3.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -pthread -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/demo_nocycle: $(SRCDIR)/demo/demo_nocycle.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -pthread -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/wfg_to_dot: tools/wfg_to_dot.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

$(BINDIR)/wfg_check: tools/wfg_check.c $(LIBCOMMON) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $< -o $@ $(LIBCOMMON)

# --- runtime lib ---
$(OBJDIRPIC)/common/util.o: $(SRCDIR)/common/util.c | $(OBJDIRPIC)/common
	$(CC) $(CFLAGS) -fPIC -I$(INCDIR) -c $< -o $@

$(OBJDIRPIC)/graph/graph.o: $(SRCDIR)/graph/graph.c | $(OBJDIRPIC)/graph
	$(CC) $(CFLAGS) -fPIC -I$(INCDIR) -c $< -o $@

$(BINDIR)/libdd.so: $(SRCDIR)/runtime/libdd.c $(OBJDIRPIC)/common/util.o $(OBJDIRPIC)/graph/graph.o | $(BINDIR)
	$(CC) $(CFLAGS) -shared -fPIC -I$(INCDIR) $^ -o $@ -ldl -pthread

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(OBJDIRPIC) $(BINDIR) demo_deadlock libdd.so
