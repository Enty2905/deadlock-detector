$(BINDIR)/gen_wfg: tools/gen_wfg.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/gen_matrix: tools/gen_matrix.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -o $@
