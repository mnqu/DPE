#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <gsl/gsl_rng.h>
#include "linelib.h"
#include "ransampl.h"

char string_set[MAX_STRING], label_set[MAX_STRING], occur_net[MAX_STRING], syn_net[MAX_STRING], pattern[MAX_STRING], output_string[MAX_STRING], output_pat[MAX_STRING], output_dis[MAX_STRING];
int binary = 0, num_threads = 1, vector_size = 100, negative = 5;
long long samples = 1, edge_count_actual;
real alpha = 0.01, beta = 0.0001, starting_alpha;

const gsl_rng_type * gsl_T;
gsl_rng * gsl_r;

line_node node_w, node_c, node_l;
line_hin hin_wc, hin_ww_syn;
line_trainer_line trainer_wc;
line_trainer_norm trainer_ww_syn;
line_trainer_feature trainer_pat;

double func_rand_num()
{
    return gsl_rng_uniform(gsl_r);
}

void *training_thread(void *id)
{
    long long edge_count = 0, last_edge_count = 0;
    unsigned long long next_random = (long long)id;
    real *error_vec = (real *)calloc(vector_size, sizeof(real));
    
    while (1)
    {
        //judge for exit
        if (edge_count > samples / num_threads + 2) break;
        
        if (edge_count - last_edge_count > 1000)
        {
            edge_count_actual += edge_count - last_edge_count;
            last_edge_count = edge_count;
            printf("%cAlpha: %f Progress: %.3lf%%", 13, alpha, (real)edge_count_actual / (real)(samples + 1) * 100);
            fflush(stdout);
        }
        
        for (int k = 0; k != 100; k++) trainer_wc.train_sample_od3(alpha, negative, error_vec, func_rand_num, next_random);
        trainer_ww_syn.train_sample(alpha, 1, 2, error_vec, func_rand_num);
        trainer_pat.train_sample(alpha, func_rand_num);
        
        edge_count += 102;
    }
    free(error_vec);
    pthread_exit(NULL);
}

void TrainModel()
{
    long a;
    pthread_t *pt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    starting_alpha = alpha;
    
    gsl_rng_env_setup();
    gsl_T = gsl_rng_rand48;
    gsl_r = gsl_rng_alloc(gsl_T);
    gsl_rng_set(gsl_r, 314159265);
    
    node_w.init(string_set, vector_size);
    node_c.init(string_set, vector_size);
    node_l.init(label_set, vector_size);
    
    hin_wc.init(occur_net, &node_w, &node_c, 0);
    hin_ww_syn.init(syn_net, &node_w, &node_w, 0);
    
    trainer_wc.init(&hin_wc, 0);
    trainer_ww_syn.init(&hin_ww_syn, 0);
    trainer_pat.init(pattern, &node_l, &node_w);
    
    clock_t start = clock();
    printf("Training:");
    for (a = 0; a < num_threads; a++) pthread_create(&pt[a], NULL, training_thread, (void *)a);
    for (a = 0; a < num_threads; a++) pthread_join(pt[a], NULL);
    printf("\n");
    clock_t finish = clock();
    printf("Total time: %lf\n", (double)(finish - start) / CLOCKS_PER_SEC);
    
    node_w.output(output_string, binary);
    node_l.output(output_pat, 0);
    trainer_ww_syn.output(output_dis, 0);
}

int ArgPos(char *str, int argc, char **argv) {
    int a;
    for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
        if (a == argc - 1) {
            printf("Argument missing for %s\n", str);
            exit(1);
        }
        return a;
    }
    return -1;
}

int main(int argc, char **argv) {
    int i;
    if (argc == 1) {
        printf("DPE\n\n");
        printf("Options:\n");
        printf("Parameters for training:\n");
        printf("\t-string-set <file>\n");
        printf("\t\tA dictionary of all strings.\n");
        printf("\t-label-set <file>\n");
        printf("\t\tThe set of all pattern labels.\n");
        printf("\t-occur-net <int>\n");
        printf("\t\tThe co-occurren network between strings\n");
        printf("\t-syn-net <int>\n");
        printf("\t\tThe set of all synonymous string pairs used in training.\n");
        printf("\t-pattern <int>\n");
        printf("\t\tThe set of all training patterns.\n");
        printf("\t-output-string <int>\n");
        printf("\t\tOutput file of the string embeddings.\n");
        printf("\t-output-pat <int>\n");
        printf("\t\tOutput file of \n");
        printf("\t-binary <int>\n");
        printf("\t\tSave the resulting vectors in binary moded; default is 0 (off)\n");
        printf("\t-size <int>\n");
        printf("\t\tSet size of word vectors; default is 100\n");
        printf("\t-negative <int>\n");
        printf("\t\tNumber of negative examples; default is 5, common values are 5 - 10 (0 = not used)\n");
        printf("\t-samples <int>\n");
        printf("\t\tSet the number of training samples as <int>Million\n");
        printf("\t-threads <int>\n");
        printf("\t\tUse <int> threads (default 1)\n");
        printf("\t-alpha <float>\n");
        printf("\t\tSet the starting learning rate; default is 0.025\n");
        return 0;
    }
    if ((i = ArgPos((char *)"-string-set", argc, argv)) > 0) strcpy(string_set, argv[i + 1]);
    if ((i = ArgPos((char *)"-label-set", argc, argv)) > 0) strcpy(label_set, argv[i + 1]);
    if ((i = ArgPos((char *)"-occur-net", argc, argv)) > 0) strcpy(occur_net, argv[i + 1]);
    if ((i = ArgPos((char *)"-syn-net", argc, argv)) > 0) strcpy(syn_net, argv[i + 1]);
    if ((i = ArgPos((char *)"-pattern", argc, argv)) > 0) strcpy(pattern, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-string", argc, argv)) > 0) strcpy(output_string, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-pat", argc, argv)) > 0) strcpy(output_pat, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-dis", argc, argv)) > 0) strcpy(output_dis, argv[i + 1]);
    if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-size", argc, argv)) > 0) vector_size = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-negative", argc, argv)) > 0) negative = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-samples", argc, argv)) > 0) samples = (long long)(atof(argv[i + 1])*1000000);
    if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0) alpha = atof(argv[i + 1]);
    if ((i = ArgPos((char *)"-threads", argc, argv)) > 0) num_threads = atoi(argv[i + 1]);
    TrainModel();
    return 0;
}