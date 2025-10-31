#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void usage(const char* prog){
    fprintf(stderr,
        "Usage: %s --mode <wfg|matrix> [--file <path>] [--verbose]\n"
        "Examples:\n"
        "  %s --mode wfg --file tests/wfg/01_cycle.in\n"
        "  %s -m matrix -f tests/matrix/01_ok_simple.in\n",
        prog, prog, prog);
}

int main(int argc, char** argv){
    const char* mode = NULL;
    const char* file = NULL;
    int verbose = 0;

    for (int i=1; i<argc; ++i){
        if (!strcmp(argv[i], "--mode") || !strcmp(argv[i], "-m")){
            if (i+1>=argc){ usage(argv[0]); return 2; }
            mode = argv[++i];
        } else if (!strcmp(argv[i], "--file") || !strcmp(argv[i], "-f")){
            if (i+1>=argc){ usage(argv[0]); return 2; }
            file = argv[++i];
        } else if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")){
            verbose = 1;
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")){
            usage(argv[0]); return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]); return 2;
        }
    }

    if (!mode){ fprintf(stderr, "--mode is required\n"); usage(argv[0]); return 2; }

    const char* bin = NULL;
    if (!strcmp(mode, "wfg"))        bin = "./bin/detect_wfg";
    else if (!strcmp(mode, "matrix")) bin = "./bin/detect_matrix";
    else { fprintf(stderr, "Invalid mode: %s (use wfg|matrix)\n", mode); return 2; }

    if (verbose){
        fprintf(stderr, "[ddetect] mode=%s file=%s\n", mode, file ? file : "<stdin>");
    }

    if (file){
        char* const args[] = { (char*)bin, (char*)file, NULL };
        execv(bin, args);
    } else {
        char* const args[] = { (char*)bin, NULL };
        execv(bin, args);
    }

    perror("execv");
    return 127;
}
