#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graph.h"

static void make_label(int pid, char* buf, size_t n){ snprintf(buf, n, "P%d", pid); }
static int  label_to_pid(const char* lab){ return (lab && lab[0]=='P') ? atoi(lab+1) : -1; }

//Xóa mấy phần tử liền nhau
static size_t dedup_consecutive(const int* a, size_t n, int* out){
    if(n==0) return 0;
    size_t m=0; out[m++]=a[0];
    for(size_t i=1;i<n;i++) if(a[i]!=out[m-1]) out[m++]=a[i];
    return m;
}
//cắt ra đoạn chu trình
static size_t extract_simple_cycle_window(const int* q, size_t m, int* cyc){
    if(m==0) return 0;
    //nếu start với end chung 1 node thì bỏ vòng lặp cuối 
    if(m>=2 && q[m-1]==q[0]){ 
        for(size_t i=0;i<m-1;i++) cyc[i]=q[i];
        return m-1;
    }
    size_t s_idx = 0, e_idx = 0;
    bool found = false;
    //tìm cặp (s,e) sao cho q[s]==q[e] với e > s
    for(size_t s=0; s<m; s++){
        for(size_t e=m-1; e>s; e--){
            if(q[s]==q[e]){
                s_idx = s; e_idx = e; found = true;
                goto got_pair;
            }
        }
    }
got_pair:
    if(found){
        size_t L=0;
        for(size_t i=s_idx;i<e_idx;i++) cyc[L++]=q[i];
        return L;
    }
    //không tìm thấy cặp nào, trả về toàn bộ
    for(size_t i=0;i<m;i++) cyc[i]=q[i];
    return m;
}
//xoay mảng để phần tử nhỏ nhất lên đầu
static void rotate_min(int* a, size_t n){
    if(n==0) return;
    size_t s=0;
    for(size_t i=1;i<n;i++) if(a[i]<a[s]) s=i;
    if(s==0) return;
    int* tmp=(int*)malloc(sizeof(int)*n);
    for(size_t k=0;k<n;k++) tmp[k]=a[(s+k)%n];
    for(size_t k=0;k<n;k++) a[k]=tmp[k];
    free(tmp);
} 

int main(int argc, char** argv){
    //Đọc đồ thị từ file
    if(argc<2){ fprintf(stderr,"Usage: %s <input>\n", argv[0]); return 2; }
    FILE* f = fopen(argv[1],"r");
    if(!f){ perror("fopen"); return 2; }

    //Đọc N , E với mấy cạnh
    int N,E;
    if(fscanf(f,"%d %d",&N,&E)!=2){ fprintf(stderr,"Bad header\n"); fclose(f); return 2; }

    //Cấp phát u ,v để lưu u và v
    int* U=(int*)malloc(sizeof(int)*E);
    int* V=(int*)malloc(sizeof(int)*E);
    for(int i=0;i<E;i++){
        if(fscanf(f,"%d %d",&U[i],&V[i])!=2){
            fprintf(stderr,"Bad edge at %d\n", i);
            fclose(f); free(U); free(V); return 2;
        }
    }
    fclose(f);
    //Xây đồ thị
    graph_t* g = graph_create();
    if(!g){ fprintf(stderr,"graph_create failed\n"); free(U); free(V); return 2; }
    //Thêm lable
    for(int i=0;i<N;i++){ char lab[32]; make_label(i,lab,sizeof(lab)); graph_get_or_add_node(g, lab); }
    for(int i=0;i<E;i++){
        char lu[32], lv[32];
        //Tạo lable cho "P<Uid>", "P<Vid>" rồi lấy id ứng
        make_label(U[i],lu,sizeof(lu)); make_label(V[i],lv,sizeof(lv));
        int uid = graph_get_or_add_node(g, lu);
        int vid = graph_get_or_add_node(g, lv);
        //Thêm cạnh có hướng 
        graph_add_edge(g, uid, vid);
    }

    //Tìm cycle xong cyc_ids trỏ tới mảng id trong graph 
    int* cyc_ids=NULL; size_t L_ids=0;
    if(!graph_find_cycle(g, &cyc_ids, &L_ids)){
        puts("NO DEADLOCK");
        graph_free(g); free(U); free(V);
        return 0;
    }

    int* raw = (int*)malloc(sizeof(int)*L_ids);
    for(size_t i=0;i<L_ids;i++){
        const char* lab = graph_node_label(g, cyc_ids[i]);
        raw[i] = label_to_pid(lab);
    }
    free(cyc_ids);
    //bỏ maasy pid trùng liền nhau
    int* q = (int*)malloc(sizeof(int)*L_ids);
    size_t qn = dedup_consecutive(raw, L_ids, q);
    int* cyc = (int*)malloc(sizeof(int)*qn);
    //Cắt ra một đoạn con thể hiện chu trình đóng
    size_t L  = extract_simple_cycle_window(q, qn, cyc);
    L = dedup_consecutive(cyc, L, cyc);
    free(raw); free(q);

    if(L==0){
        puts("DEADLOCK");
        graph_free(g); free(U); free(V); free(cyc);
        return 0;
    }
    rotate_min(cyc, L);
    // nếu true i nằm trong vùng cycle -> loại bỏ cycle khỏi đồi thị rồi check xem phần còn lại có khác không
    bool* inC = (bool*)calloc((size_t)N, sizeof(bool));
    for(size_t i=0;i<L;i++) if(cyc[i]>=0 && cyc[i]<N) inC[cyc[i]] = true;

    //Tạo để chứa mấy node không thuộc cycle
    graph_t* g2 = graph_create();
    for(int i=0;i<N;i++) if(!inC[i]){ char lab[32]; make_label(i,lab,sizeof(lab)); graph_get_or_add_node(g2, lab); }
    for(int i=0;i<E;i++){
        if(!inC[U[i]] && !inC[V[i]]){
            char lu[32], lv[32]; make_label(U[i],lu,sizeof(lu)); make_label(V[i],lv,sizeof(lv));
            int uid = graph_get_or_add_node(g2, lu);
            int vid = graph_get_or_add_node(g2, lv);
            graph_add_edge(g2, uid, vid);
        }
    }
    //kiểm tra cycle trong g2 xem còn thừa không
    int* cyc2=NULL; size_t L2=0;
    bool has_disjoint = graph_find_cycle(g2, &cyc2, &L2);
    if(cyc2) free(cyc2);
    graph_free(g2);

    if(has_disjoint){
        puts("DEADLOCK");
    }else{
        printf("DEADLOCK cycle: ");
        for(size_t i=0;i<L;i++){
            printf("P%d", cyc[i]);
            putchar(' ');
        }
        printf("P%d\n", cyc[0]);
    }

    free(cyc); free(inC);
    graph_free(g); free(U); free(V);
    return 0;
}
