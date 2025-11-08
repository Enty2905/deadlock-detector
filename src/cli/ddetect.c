#include <sys/wait.h>
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char* prog){
    fprintf(stderr, "Usage: %s --mode wfg|matrix --file PATH\n", prog);
}

int main(int argc, char**argv){
    const char *mode=NULL, *path=NULL;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-m")||!strcmp(argv[i],"--mode")){
            if(i+1<argc) mode=argv[++i];
        }else if(!strcmp(argv[i],"-f")||!strcmp(argv[i],"--file")){
            if(i+1<argc) path=argv[++i];
        }else if(!strcmp(argv[i],"--verbose")||!strcmp(argv[i],"-v")){
            /* no-op here; detectors print fixed format */
        }else{
            usage(argv[0]); return EX_BAD_INPUT;
        }
    }
    if(!mode || !path){ usage(argv[0]); return EX_BAD_INPUT; }

    char cmd[1024];
    if(!strcmp(mode,"wfg")){
        snprintf(cmd, sizeof(cmd), "./bin/detect_wfg %s", path);
    }else if(!strcmp(mode,"matrix")){
        snprintf(cmd, sizeof(cmd), "./bin/detect_matrix %s", path);
    }else{
        usage(argv[0]); return EX_BAD_INPUT;
    }

    int rc = system(cmd);
    if(rc==-1) return EX_INTERNAL;
    if(WIFEXITED(rc)) return WEXITSTATUS(rc);
    return EX_INTERNAL;
}
