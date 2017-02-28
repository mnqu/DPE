#include "linelib.h"

line_node::line_node() : vec(NULL, 0, 0)
{
    node = NULL;
    node_size = 0;
    node_max_size = 1000;
    vector_size = 0;
    node_file[0] = 0;
    node_hash = NULL;
    _vec = NULL;
}

line_node::~line_node()
{
    if (node != NULL) {free(node); node = NULL;}
    node_size = 0;
    node_max_size = 0;
    vector_size = 0;
    node_file[0] = 0;
    if (node_hash != NULL) {free(node_hash); node_hash = NULL;}
    if (_vec != NULL) {free(_vec); _vec = NULL;}
    new (&vec) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

int line_node::get_hash(char *word)
{
    unsigned long long a, hash = 0;
    for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
    hash = hash % hash_table_size;
    return hash;
}

int line_node::search(char *word)
{
    unsigned int hash = get_hash(word);
    while (1) {
        if (node_hash[hash] == -1) return -1;
        if (!strcmp(word, node[node_hash[hash]].word)) return node_hash[hash];
        hash = (hash + 1) % hash_table_size;
    }
    return -1;
}

int line_node::add_node(char *word)
{
    unsigned int hash, length = strlen(word) + 1;
    if (length > MAX_STRING) length = MAX_STRING;
    node[node_size].word = (char *)calloc(length, sizeof(char));
    strcpy(node[node_size].word, word);
    node_size++;
    // Reallocate memory if needed
    if (node_size + 2 >= node_max_size) {
        node_max_size += 1000;
        node = (struct struct_node *)realloc(node, node_max_size * sizeof(struct struct_node));
    }
    hash = get_hash(word);
    while (node_hash[hash] != -1) hash = (hash + 1) % hash_table_size;
    node_hash[hash] = node_size - 1;
    return node_size - 1;
}

void line_node::init(const char *file_name, int vector_dim)
{
    strcpy(node_file, file_name);
    vector_size = vector_dim;
    
    node = (struct struct_node *)calloc(node_max_size, sizeof(struct struct_node));
    node_hash = (int *)calloc(hash_table_size, sizeof(int));
    for (int k = 0; k != hash_table_size; k++) node_hash[k] = -1;
    
    FILE *fi = fopen(node_file, "rb");
    if (fi == NULL)
    {
        printf("ERROR: node file not found!\n");
        printf("%s\n", node_file);
        exit(1);
    }
    
    char word[MAX_STRING];
    node_size = 0;
    while (1)
    {
        if (fscanf(fi, "%s", word) != 1) break;
        add_node(word);
    }
    fclose(fi);
    
    long long a, b;
    a = posix_memalign((void **)&_vec, 128, (long long)node_size * vector_size * sizeof(real));
    if (_vec == NULL) { printf("Memory allocation failed\n"); exit(1); }
    for (b = 0; b < vector_size; b++) for (a = 0; a < node_size; a++)
        _vec[a * vector_size + b] = (rand() / (real)RAND_MAX - 0.5) / vector_size;
    new (&vec) Eigen::Map<BLPMatrix>(_vec, node_size, vector_size);
    
    printf("Reading nodes from file: %s, DONE!\n", node_file);
    printf("Node size: %d\n", node_size);
    printf("Node dims: %d\n", vector_size);
}

void line_node::output(const char *file_name, int binary)
{
    FILE *fo = fopen(file_name, "wb");
    fprintf(fo, "%d %d\n", node_size, vector_size);
    for (int a = 0; a != node_size; a++)
    {
        fprintf(fo, "%s ", node[a].word);
        if (binary) for (int b = 0; b != vector_size; b++) fwrite(&_vec[a * vector_size + b], sizeof(real), 1, fo);
        else for (int b = 0; b != vector_size; b++) fprintf(fo, "%lf ", _vec[a * vector_size + b]);
        fprintf(fo, "\n");
    }
    fclose(fo);
}

int line_node::get_node_size()
{
    return node_size;
}

real *line_node::get_vec()
{
    return _vec;
}

line_hin::line_hin()
{
    hin_file[0] = 0;
    node_u = NULL;
    node_v = NULL;
    hin = NULL;
    hin_size = 0;
}

line_hin::~line_hin()
{
    hin_file[0] = 0;
    node_u = NULL;
    node_v = NULL;
    if (hin != NULL) {delete [] hin; hin = NULL;}
    hin_size = 0;
}

void line_hin::init(const char *file_name, line_node *p_u, line_node *p_v, bool with_type)
{
    strcpy(hin_file, file_name);
    
    node_u = p_u;
    node_v = p_v;
    
    int node_size = node_u->node_size;
    hin = new std::vector<hin_nb>[node_size];
    
    FILE *fi = fopen(hin_file, "rb");
    if (fi == NULL)
    {
        printf("ERROR: hin file not found!\n");
        printf("%s\n", hin_file);
        exit(1);
    }
    char word1[MAX_STRING], word2[MAX_STRING], tp;
    int u, v;
    double w;
    hin_nb curnb;
    if (with_type)
    {
        while (fscanf(fi, "%s %s %lf %c", word1, word2, &w, &tp) == 4)
        {
            if (hin_size % 10000 == 0)
            {
                printf("%lldK%c", hin_size / 1000, 13);
                fflush(stdout);
            }
            
            u = node_u->search(word1);
            v = node_v->search(word2);
            
            if (u != -1 && v != -1)
            {
                curnb.nb_id = v;
                curnb.eg_tp = tp;
                curnb.eg_wei = w;
                hin[u].push_back(curnb);
                hin_size++;
            }
        }
    }
    else
    {
        while (fscanf(fi, "%s %s %lf", word1, word2, &w) == 3)
        {
            if (hin_size % 10000 == 0)
            {
                printf("%lldK%c", hin_size / 1000, 13);
                fflush(stdout);
            }
            
            u = node_u->search(word1);
            v = node_v->search(word2);
            tp = 0;
            
            if (u != -1 && v != -1)
            {
                curnb.nb_id = v;
                curnb.eg_tp = tp;
                curnb.eg_wei = w;
                hin[u].push_back(curnb);
                hin_size++;
            }
        }
    }
    fclose(fi);
    
    printf("Reading edges from file: %s, DONE!\n", hin_file);
    printf("Edge size: %lld\n", hin_size);
}

line_adjacency::line_adjacency()
{
    adjmode = 1;
    edge_tp = 0;
    u_wei = NULL;
    smp_u = NULL;
    u_nb_cnt = NULL;
    u_nb_id = NULL;
    u_nb_wei = NULL;
    smp_u_nb = NULL;
    v_nb_cnt = NULL;
    v_nb_id = NULL;
    v_nb_wei = NULL;
    smp_v_nb = NULL;
}

line_adjacency::~line_adjacency()
{
    adjmode = 1;
    edge_tp = 0;
    if (u_wei != NULL) {free(u_wei); u_wei = NULL;}
    if (u_nb_cnt != NULL) {free(u_nb_cnt); u_nb_cnt = NULL;}
    if (u_nb_id != NULL) {free(u_nb_id); u_nb_id = NULL;}
    if (u_nb_wei != NULL) {free(u_nb_wei); u_nb_wei = NULL;}
    if (v_nb_cnt != NULL) {free(v_nb_cnt); v_nb_cnt = NULL;}
    if (v_nb_id != NULL) {free(v_nb_id); v_nb_id = NULL;}
    if (v_nb_wei != NULL) {free(v_nb_wei); v_nb_wei = NULL;}
    if (smp_u_nb != NULL)
    {
        free(smp_u_nb);
        smp_u_nb = NULL;
    }
    if (smp_v_nb != NULL)
    {
        free(smp_u_nb);
        smp_u_nb = NULL;
    }
}

void line_adjacency::init(line_hin *p_hin, char edge_type, int mode)
{
    phin = p_hin;
    adjmode = mode;
    edge_tp = edge_type;
    
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    long long adj_size = 0;
    
    // compute the degree of vertices
    u_wei = (double *)calloc(node_u->node_size, sizeof(double));
    u_nb_cnt = (int *)calloc(node_u->node_size, sizeof(int));
    v_nb_cnt = (int *)calloc(node_v->node_size, sizeof(int));
    double *u_len = (double *)calloc(node_u->node_size, sizeof(double));
    
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_wei[u] += wei;
            u_nb_cnt[u] += 1;
            v_nb_cnt[v] += 1;
            if (adjmode == 21) u_len[u] += wei;
            if (adjmode == 22) u_len[u] += wei * wei;
            
            adj_size += 1;
        }
    }
    
    if (adjmode != 1) for (int u = 0; u != node_u->node_size; u++)
    {
        if (u_nb_cnt[u] == 0) u_wei[u] = 0;
        else u_wei[u] = 1;
    }
    smp_u = ransampl_alloc(node_u->node_size);
    ransampl_set(smp_u, u_wei);
    
    if (adjmode == 22) for (int k = 0; k != node_u->node_size; k++)
    {
        if (u_len[k] != 0) u_len[k] = sqrt(u_len[k]);
        else u_len[k] = 1;
    }
    
    u_nb_id = (int **)malloc(node_u->node_size * sizeof(int *));
    u_nb_wei = (double **)malloc(node_u->node_size * sizeof(double *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        u_nb_id[k] = (int *)malloc(u_nb_cnt[k] * sizeof(int));
        u_nb_wei[k] = (double *)malloc(u_nb_cnt[k] * sizeof(double));
    }
    
    v_nb_id = (int **)malloc(node_v->node_size * sizeof(int *));
    v_nb_wei = (double **)malloc(node_v->node_size * sizeof(double *));
    for (int k = 0; k != node_v->node_size; k++)
    {
        v_nb_id[k] = (int *)malloc(v_nb_cnt[k] * sizeof(int));
        v_nb_wei[k] = (double *)malloc(v_nb_cnt[k] * sizeof(double));
    }
    
    int *pst_u = (int *)calloc(node_u->node_size, sizeof(int));
    int *pst_v = (int *)calloc(node_v->node_size, sizeof(int));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            if (adjmode == 21 || adjmode == 22) wei = wei / u_len[u];
            
            u_nb_id[u][pst_u[u]] = v;
            u_nb_wei[u][pst_u[u]] = wei;
            pst_u[u]++;
            
            v_nb_id[v][pst_v[v]] = u;
            v_nb_wei[v][pst_v[v]] = wei;
            pst_v[v]++;
        }
    }
    free(pst_u);
    free(pst_v);
    free(u_len);
    
    smp_u_nb = (ransampl_ws **)malloc(node_u->node_size * sizeof(ransampl_ws *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        if (u_nb_cnt[k] == 0) continue;
        smp_u_nb[k] = ransampl_alloc(u_nb_cnt[k]);
        ransampl_set(smp_u_nb[k], u_nb_wei[k]);
    }
    
    smp_v_nb = (ransampl_ws **)malloc(node_v->node_size * sizeof(ransampl_ws *));
    for (int k = 0; k != node_v->node_size; k++)
    {
        if (v_nb_cnt[k] == 0) continue;
        smp_v_nb[k] = ransampl_alloc(v_nb_cnt[k]);
        ransampl_set(smp_v_nb[k], v_nb_wei[k]);
    }
    
    printf("Reading adjacency from file: %s, DONE!\n", phin->hin_file);
    printf("Adjacency size: %lld\n", adj_size);
}

int line_adjacency::sample(int u, double (*func_rand_num)())
{
    int index, node, v;
    
    if (u == -1) return -1;
    
    if (adjmode == 1)
    {
        if (u_nb_cnt[u] == 0) return -1;
        index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
        node = u_nb_id[u][index];
        return node;
    }
    else
    {
        if (u_nb_cnt[u] == 0) return -1;
        index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
        v = u_nb_id[u][index];
        
        if (v_nb_cnt[v] == 0) return -1;
        index = (int)(ransampl_draw(smp_v_nb[v], func_rand_num(), func_rand_num()));
        node = v_nb_id[v][index];
        
        return node;
    }
}

int line_adjacency::sample_head(double (*func_rand_num)())
{
    return (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
}

line_trainer_line::line_trainer_line()
{
    edge_tp = 0;
    phin = NULL;
    expTable = NULL;
    u_nb_cnt = NULL;
    u_nb_id = NULL;
    u_nb_wei = NULL;
    u_wei = NULL;
    v_wei = NULL;
    smp_u = NULL;
    smp_u_nb = NULL;
    expTable = NULL;
    neg_table = NULL;
}

line_trainer_line::~line_trainer_line()
{
    edge_tp = 0;
    phin = NULL;
    if (expTable != NULL) {free(expTable); expTable = NULL;}
    if (u_nb_cnt != NULL) {free(u_nb_cnt); u_nb_cnt = NULL;}
    if (u_nb_id != NULL) {free(u_nb_id); u_nb_id = NULL;}
    if (u_nb_wei != NULL) {free(u_nb_wei); u_nb_wei = NULL;}
    if (u_wei != NULL) {free(u_wei); u_wei = NULL;}
    if (v_wei != NULL) {free(v_wei); v_wei = NULL;}
    if (smp_u != NULL)
    {
        ransampl_free(smp_u);
        smp_u = NULL;
    }
    if (smp_u_nb != NULL)
    {
        free(smp_u_nb);
        smp_u_nb = NULL;
    }
    if (neg_table != NULL) {free(neg_table); neg_table = NULL;}
}

void line_trainer_line::init(line_hin *p_hin, char edge_type)
{
    edge_tp = edge_type;
    phin = p_hin;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    if (node_u->vector_size != node_v->vector_size)
    {
        printf("ERROR: vector dimsions are not same!\n");
        exit(1);
    }
    
    // compute the degree of vertices
    u_nb_cnt = (int *)calloc(node_u->node_size, sizeof(int));
    u_wei = (double *)calloc(node_u->node_size, sizeof(double));
    v_wei = (double *)calloc(node_v->node_size, sizeof(double));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_cnt[u]++;
            u_wei[u] += wei;
            v_wei[v] += wei;
        }
    }
    
    // allocate spaces for edges
    u_nb_id = (int **)malloc(node_u->node_size * sizeof(int *));
    u_nb_wei = (double **)malloc(node_u->node_size * sizeof(double *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        u_nb_id[k] = (int *)malloc(u_nb_cnt[k] * sizeof(int));
        u_nb_wei[k] = (double *)malloc(u_nb_cnt[k] * sizeof(double));
    }
    
    // read neighbors
    int *pst = (int *)calloc(node_u->node_size, sizeof(int));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_id[u][pst[u]] = v;
            u_nb_wei[u][pst[u]] = wei;
            pst[u]++;
        }
    }
    free(pst);
    
    // init sampler for edges
    smp_u = ransampl_alloc(node_u->node_size);
    ransampl_set(smp_u, u_wei);
    smp_u_nb = (ransampl_ws **)malloc(node_u->node_size * sizeof(ransampl_ws *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        if (u_nb_cnt[k] == 0) continue;
        smp_u_nb[k] = ransampl_alloc(u_nb_cnt[k]);
        ransampl_set(smp_u_nb[k], u_nb_wei[k]);
    }
    
    // Init negative sampling table
    neg_table = (int *)malloc(neg_table_size * sizeof(int));
    
    int a, i;
    double total_pow = 0, d1;
    double power = 0.75;
    for (a = 0; a < node_v->node_size; a++) total_pow += pow(v_wei[a], power);
    a = 0; i = 0;
    d1 = pow(v_wei[i], power) / (double)total_pow;
    while (a < neg_table_size) {
        if ((a + 1) / (double)neg_table_size > d1) {
            i++;
            if (i >= node_v->node_size) {i = node_v->node_size - 1; d1 = 2;}
            d1 += pow(v_wei[i], power) / (double)total_pow;
        }
        else
            neg_table[a++] = i;
    }
    
    expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
    for (int i = 0; i < EXP_TABLE_SIZE; i++) {
        expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
        expTable[i] = expTable[i] / (expTable[i] + 1);                   // Precompute f(x) = x / (x + 1)
    }
}

void line_trainer_line::copy_neg_table(line_trainer_line *p_trainer_line)
{
    if (phin->node_v->node_size != p_trainer_line->phin->node_v->node_size)
    {
        printf("ERROR: node sizes are not same!\n");
        exit(1);
    }
    
    int node_size = phin->node_v->node_size;
    
    for (int k = 0; k != node_size; k++) v_wei[k] = p_trainer_line->v_wei[k];
    for (int k = 0; k != neg_table_size; k++) neg_table[k] = p_trainer_line->neg_table[k];
}

void line_trainer_line::train_uv(int u, int v, real lr, int neg_samples, real *_error_vec, unsigned long long &rand_index)
{
    int target, label, vector_size;
    real f, g;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    vector_size = node_u->vector_size;
    Eigen::Map<BLPVector> error_vec(_error_vec, vector_size);
    error_vec.setZero();
    
    for (int d = 0; d < neg_samples + 1; d++)
    {
        if (d == 0)
        {
            target = v;
            label = 1;
        }
        else
        {
            rand_index = rand_index * (unsigned long long)25214903917 + 11;
            target = neg_table[(rand_index >> 16) % neg_table_size];
            if (target == v) continue;
            label = 0;
        }
        f = node_u->vec.row(u) * node_v->vec.row(target).transpose();
        if (f > MAX_EXP) g = (label - 1) * lr;
        else if (f < -MAX_EXP) g = (label - 0) * lr;
        else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * lr;
        error_vec += g * ((node_v->vec.row(target)));
        node_v->vec.row(target) += g * ((node_u->vec.row(u)));
    }
    node_u->vec.row(u) += error_vec;
    new (&error_vec) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

void line_trainer_line::train_uv_od3(int u, int v, real lr, int neg_samples, real *_error_vec, unsigned long long &rand_index)
{
    int target, label, vector_size;
    real f, g;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    vector_size = node_u->vector_size;
    Eigen::Map<BLPVector> error_vec(_error_vec, vector_size);
    error_vec.setZero();
    
    for (int d = 0; d < neg_samples + 1; d++)
    {
        if (d == 0)
        {
            target = v;
            label = 1;
        }
        else
        {
            rand_index = rand_index * (unsigned long long)25214903917 + 11;
            target = neg_table[(rand_index >> 16) % neg_table_size];
            if (target == v) continue;
            label = 0;
        }
        f = 0;
        f += node_u->vec.row(u) * node_u->vec.row(target).transpose();
        f += node_u->vec.row(u) * node_v->vec.row(target).transpose();
        if (f > MAX_EXP) g = (label - 1) * lr;
        else if (f < -MAX_EXP) g = (label - 0) * lr;
        else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * lr;
        error_vec += g * (node_u->vec.row(target) + (node_v->vec.row(target)));
        node_u->vec.row(target) += g * ((node_u->vec.row(u)));
        node_v->vec.row(target) += g * ((node_u->vec.row(u)));
    }
    node_u->vec.row(u) += error_vec;
    new (&error_vec) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

void line_trainer_line::train_uv_od3_attention(int u, int v, real lr, int neg_samples, real *_error_vec, unsigned long long &rand_index, real lr2, real *_para)
{
    int target, label, vector_size;
    real f, g;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    vector_size = node_u->vector_size;
    
    Eigen::Map<BLPMatrix> para(_para, 2, vector_size);
    Eigen::Map<BLPVector> error_vec(_error_vec, vector_size);
    error_vec.setZero();
    
    for (int d = 0; d < neg_samples + 1; d++)
    {
        if (d == 0)
        {
            target = v;
            label = 1;
        }
        else
        {
            rand_index = rand_index * (unsigned long long)25214903917 + 11;
            target = neg_table[(rand_index >> 16) % neg_table_size];
            if (target == v) continue;
            label = 0;
        }
        
        real s = 0;
        s += node_u->vec.row(u) * para.row(0).transpose();
        s += node_u->vec.row(target) * para.row(1).transpose();
        real sig = 1 / (1 + exp(-s));
        
        real xuxv = node_u->vec.row(u) * node_u->vec.row(target).transpose();
        real xucv = node_u->vec.row(u) * node_v->vec.row(target).transpose();
        
        f = sig * xuxv + (1 - sig) * xucv;
        if (f > MAX_EXP) g = (label - 1);
        else if (f < -MAX_EXP) g = (label - 0);
        else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]);
        
        real h = g * (xuxv - xucv) * sig * (1 - sig);
        para.row(0) += h * lr2 * node_u->vec.row(u);
        para.row(1) += h * lr2 * node_u->vec.row(target);
        
        error_vec += g * lr * (sig * node_u->vec.row(target) + (1 - sig) * node_v->vec.row(target));
        node_u->vec.row(target) += g * lr * sig * (node_u->vec.row(u));
        node_v->vec.row(target) += g * lr * (1 - sig) * (node_u->vec.row(u));
    }
    node_u->vec.row(u) += error_vec;
    new (&error_vec) Eigen::Map<BLPVector>(NULL, 0);
    new (&para) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

void line_trainer_line::train_sample(real lr, int neg_samples, real *_error_vec, double (*func_rand_num)(), unsigned long long &rand_index)
{
    int u, v, index;
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    if (u_nb_cnt[u] == 0) return;
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    train_uv(u, v, lr, neg_samples, _error_vec, rand_index);
}

void line_trainer_line::train_sample_od3(real lr, int neg_samples, real *_error_vec, double (*func_rand_num)(), unsigned long long &rand_index)
{
    int u, v, index;
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    if (u_nb_cnt[u] == 0) return;
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    train_uv_od3(u, v, lr, neg_samples, _error_vec, rand_index);
}

void line_trainer_line::train_sample_od3_attention(real lr, int neg_samples, real *_error_vec, double (*func_rand_num)(), unsigned long long &rand_index, real lr2, real *_para)
{
    int u, v, index;
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    if (u_nb_cnt[u] == 0) return;
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    train_uv_od3_attention(u, v, lr, neg_samples, _error_vec, rand_index, lr2, _para);
}

void line_trainer_line::train_sample_depth(real lr, int neg_samples, real *_error_vec, double (*func_rand_num)(), unsigned long long &rand_index, int depth, line_adjacency *p_adjacency, char pst)
{
    int u, v, index;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    if (pst == 'r')
    {
        node_lst.push_back(v);
        
        for (int k = 1; k != depth; k++)
        {
            v = p_adjacency->sample(v, func_rand_num);
            node_lst.push_back(v);
        }
        
        for (int k = 0; k != depth; k++)
        {
            v = node_lst[k];
            if (v == -1) continue;
            train_uv(u, v, lr, neg_samples, _error_vec, rand_index);
        }
    }
    else if (pst == 'l')
    {
        node_lst.push_back(u);
        
        for (int k = 1; k != depth; k++)
        {
            u = p_adjacency->sample(u, func_rand_num);
            node_lst.push_back(u);
        }
        
        for (int k = 0; k != depth; k++)
        {
            u = node_lst[k];
            if (u == -1) continue;
            train_uv(u, v, lr, neg_samples, _error_vec, rand_index);
        }
    }
}

line_trainer_norm::line_trainer_norm() : para(NULL, 0)
{
    edge_tp = 0;
    phin = NULL;
    u_nb_cnt = NULL;
    u_nb_id = NULL;
    u_nb_wei = NULL;
    u_wei = NULL;
    v_wei = NULL;
    smp_u = NULL;
    smp_u_nb = NULL;
    _para = NULL;
}

line_trainer_norm::~line_trainer_norm()
{
    edge_tp = 0;
    phin = NULL;
    if (u_nb_cnt != NULL) {free(u_nb_cnt); u_nb_cnt = NULL;}
    if (u_nb_id != NULL) {free(u_nb_id); u_nb_id = NULL;}
    if (u_nb_wei != NULL) {free(u_nb_wei); u_nb_wei = NULL;}
    if (u_wei != NULL) {free(u_wei); u_wei = NULL;}
    if (v_wei != NULL) {free(v_wei); v_wei = NULL;}
    if (smp_u != NULL)
    {
        ransampl_free(smp_u);
        smp_u = NULL;
    }
    if (smp_u_nb != NULL)
    {
        free(smp_u_nb);
        smp_u_nb = NULL;
    }
    if (_para != NULL)
    {
        free(_para);
        _para = NULL;
    }
}

void line_trainer_norm::init(line_hin *p_hin, char edge_type)
{
    edge_tp = edge_type;
    phin = p_hin;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    if (node_u->vector_size != node_v->vector_size)
    {
        printf("ERROR: vector dimsions are not same!\n");
        exit(1);
    }
    
    // compute the degree of vertices
    u_nb_cnt = (int *)calloc(node_u->node_size, sizeof(int));
    u_wei = (double *)calloc(node_u->node_size, sizeof(double));
    v_wei = (double *)calloc(node_v->node_size, sizeof(double));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_cnt[u]++;
            u_wei[u] += wei;
            v_wei[v] += wei;
        }
    }
    
    // allocate spaces for edges
    u_nb_id = (int **)malloc(node_u->node_size * sizeof(int *));
    u_nb_wei = (double **)malloc(node_u->node_size * sizeof(double *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        u_nb_id[k] = (int *)malloc(u_nb_cnt[k] * sizeof(int));
        u_nb_wei[k] = (double *)malloc(u_nb_cnt[k] * sizeof(double));
    }
    
    // read neighbors
    int *pst = (int *)calloc(node_u->node_size, sizeof(int));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_id[u][pst[u]] = v;
            u_nb_wei[u][pst[u]] = wei;
            pst[u]++;
        }
    }
    free(pst);
    
    // init sampler for edges
    smp_u = ransampl_alloc(node_u->node_size);
    ransampl_set(smp_u, u_wei);
    smp_u_nb = (ransampl_ws **)malloc(node_u->node_size * sizeof(ransampl_ws *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        if (u_nb_cnt[k] == 0) continue;
        smp_u_nb[k] = ransampl_alloc(u_nb_cnt[k]);
        ransampl_set(smp_u_nb[k], u_nb_wei[k]);
    }
    
    // init para
    int vector_size = node_u->vector_size;
    _para = (real *)malloc(vector_size * sizeof(real));
    for (int k = 0; k != vector_size; k++) _para[k] = 1;
    new (&para) Eigen::Map<BLPVector>(_para, vector_size);
}

void line_trainer_norm::train_uv(int u, int v, real lr, real margin, int dis_type, real *_error_vec, double randv)
{
    int n, vector_size;
    real dpos = 0, dneg = 0;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    vector_size = node_u->vector_size;
    Eigen::Map<BLPVector> error_vec(_error_vec, vector_size);
    error_vec.setZero();
    
    n = node_v->node_size * randv;
    
    if (dis_type == 1)
    {
        dpos = (node_u->vec.row(u) - node_v->vec.row(v)).array().abs().sum();
        dneg = (node_u->vec.row(u) - node_v->vec.row(n)).array().abs().sum();
    }
    else if (dis_type == 2)
    {
        dpos = (node_u->vec.row(u) - node_v->vec.row(v)).array().pow(2).sum();
        dneg = (node_u->vec.row(u) - node_v->vec.row(n)).array().pow(2).sum();
    }
    
    if (dneg - dpos < margin)
    {
        real x = 0;
        for (int c = 0; c != vector_size; c++)
        {
            if (dis_type == 1)
            {
                x = 2 * (node_u->vec(u, c) - node_v->vec(v, c));
                if (x > 0) x = 1;
                else x = -1;
            }
            else if (dis_type == 2)
            {
                x = 2 * (node_u->vec(u, c) - node_v->vec(v, c));
            }
            node_v->vec(v, c) += lr * x;
            error_vec(c) -= lr * x;
            
            if (dis_type == 1)
            {
                x = 2 * (node_u->vec(u, c) - node_v->vec(n, c));
                if (x > 0) x = 1;
                else x = -1;
            }
            else if (dis_type == 2)
            {
                x = 2 * (node_u->vec(u, c) - node_v->vec(n, c));
            }
            node_v->vec(n, c) -= lr * x;
            error_vec(c) += lr * x;
        }
        node_u->vec.row(u) += error_vec;
    }
    new (&error_vec) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

void line_trainer_norm::train_sample(real lr, real margin, int dis_type, real *_error_vec, double (*func_rand_num)())
{
    int u, v, index;
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    if (u_nb_cnt[u] == 0) return;
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    train_uv(u, v, lr, margin, dis_type, _error_vec, func_rand_num());
}

void line_trainer_norm::train_sample_depth(real lr, real margin, int dis_type, real *_error_vec, double (*func_rand_num)(), int depth, line_adjacency *p_adjacency, char pst)
{
    int u, v, index;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    if (pst == 'r')
    {
        node_lst.push_back(v);
        
        for (int k = 1; k != depth; k++)
        {
            v = p_adjacency->sample(v, func_rand_num);
            node_lst.push_back(v);
        }
        
        for (int k = 1; k != depth; k++)
        {
            v = node_lst[k];
            if (v == -1) continue;
            train_uv(u, v, lr, margin, dis_type, _error_vec, func_rand_num());
        }
    }
    else if (pst == 'l')
    {
        node_lst.push_back(u);
        
        for (int k = 1; k != depth; k++)
        {
            u = p_adjacency->sample(u, func_rand_num);
            node_lst.push_back(u);
        }
        
        for (int k = 1; k != depth; k++)
        {
            u = node_lst[k];
            if (u == -1) continue;
            train_uv(u, v, lr, margin, dis_type, _error_vec, func_rand_num());
        }
    }
}

void line_trainer_norm::output(const char *file_name, int binary)
{
    FILE *fo = fopen(file_name, "wb");
    line_node *node_u = phin->node_u;
    int vector_size = node_u->vector_size;
    fprintf(fo, "%d\n", vector_size);
    if (binary) for (int b = 0; b != vector_size; b++) fwrite(&_para[b], sizeof(real), 1, fo);
    else for (int b = 0; b != vector_size; b++) fprintf(fo, "%lf ", _para[b]);
        fprintf(fo, "\n");
    fclose(fo);
}

line_trainer_reg::line_trainer_reg()
{
    edge_tp = 0;
    phin = NULL;
    u_nb_cnt = NULL;
    u_nb_id = NULL;
    u_nb_wei = NULL;
    u_wei = NULL;
    v_wei = NULL;
    smp_u = NULL;
    smp_u_nb = NULL;
}

line_trainer_reg::~line_trainer_reg()
{
    edge_tp = 0;
    phin = NULL;
    if (u_nb_cnt != NULL) {free(u_nb_cnt); u_nb_cnt = NULL;}
    if (u_nb_id != NULL) {free(u_nb_id); u_nb_id = NULL;}
    if (u_nb_wei != NULL) {free(u_nb_wei); u_nb_wei = NULL;}
    if (u_wei != NULL) {free(u_wei); u_wei = NULL;}
    if (v_wei != NULL) {free(v_wei); v_wei = NULL;}
    if (smp_u != NULL)
    {
        ransampl_free(smp_u);
        smp_u = NULL;
    }
    if (smp_u_nb != NULL)
    {
        free(smp_u_nb);
        smp_u_nb = NULL;
    }
}

void line_trainer_reg::init(line_hin *p_hin, char edge_type)
{
    edge_tp = edge_type;
    phin = p_hin;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    if (node_u->vector_size != node_v->vector_size)
    {
        printf("ERROR: vector dimsions are not same!\n");
        exit(1);
    }
    
    // compute the degree of vertices
    u_nb_cnt = (int *)calloc(node_u->node_size, sizeof(int));
    u_wei = (double *)calloc(node_u->node_size, sizeof(double));
    v_wei = (double *)calloc(node_v->node_size, sizeof(double));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_cnt[u]++;
            u_wei[u] += wei;
            v_wei[v] += wei;
        }
    }
    
    // allocate spaces for edges
    u_nb_id = (int **)malloc(node_u->node_size * sizeof(int *));
    u_nb_wei = (double **)malloc(node_u->node_size * sizeof(double *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        u_nb_id[k] = (int *)malloc(u_nb_cnt[k] * sizeof(int));
        u_nb_wei[k] = (double *)malloc(u_nb_cnt[k] * sizeof(double));
    }
    
    // read neighbors
    int *pst = (int *)calloc(node_u->node_size, sizeof(int));
    for (int u = 0; u != node_u->node_size; u++)
    {
        for (int k = 0; k != (int)(phin->hin[u].size()); k++)
        {
            int v = phin->hin[u][k].nb_id;
            char cur_edge_type = phin->hin[u][k].eg_tp;
            double wei = phin->hin[u][k].eg_wei;
            
            if (cur_edge_type != edge_tp) continue;
            
            u_nb_id[u][pst[u]] = v;
            u_nb_wei[u][pst[u]] = wei;
            pst[u]++;
        }
    }
    free(pst);
    
    // init sampler for edges
    smp_u = ransampl_alloc(node_u->node_size);
    ransampl_set(smp_u, u_wei);
    smp_u_nb = (ransampl_ws **)malloc(node_u->node_size * sizeof(ransampl_ws *));
    for (int k = 0; k != node_u->node_size; k++)
    {
        if (u_nb_cnt[k] == 0) continue;
        smp_u_nb[k] = ransampl_alloc(u_nb_cnt[k]);
        ransampl_set(smp_u_nb[k], u_nb_wei[k]);
    }
}

void line_trainer_reg::train_uv(int u, int v, real lr)
{
    int vector_size;
    line_node *node_u = phin->node_u, *node_v = phin->node_v;
    
    vector_size = node_u->vector_size;
    
    for (int c = 0; c != vector_size; c++)
    {
        real f = node_u->vec(u, c) - node_v->vec(v, c);
        node_u->vec(u, c) -= lr * f;
        node_v->vec(v, c) += lr * f;
    }
}

void line_trainer_reg::train_sample(real lr, double (*func_rand_num)())
{
    int u, v, index;
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    if (u_nb_cnt[u] == 0) return;
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    train_uv(u, v, lr);
}

void line_trainer_reg::train_sample_depth(real lr, double (*func_rand_num)(), int depth, line_adjacency *p_adjacency, char pst)
{
    int u, v, index;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = (int)(ransampl_draw(smp_u, func_rand_num(), func_rand_num()));
    index = (int)(ransampl_draw(smp_u_nb[u], func_rand_num(), func_rand_num()));
    v = u_nb_id[u][index];
    
    if (pst == 'r')
    {
        node_lst.push_back(v);
        
        for (int k = 1; k != depth; k++)
        {
            v = p_adjacency->sample(v, func_rand_num);
            node_lst.push_back(v);
        }
        
        for (int k = 0; k != depth; k++)
        {
            v = node_lst[k];
            if (v == -1) continue;
            train_uv(u, v, lr);
        }
    }
    else if (pst == 'l')
    {
        node_lst.push_back(u);
        
        for (int k = 1; k != depth; k++)
        {
            u = p_adjacency->sample(u, func_rand_num);
            node_lst.push_back(u);
        }
        
        for (int k = 0; k != depth; k++)
        {
            u = node_lst[k];
            if (u == -1) continue;
            train_uv(u, v, lr);
        }
    }
}

line_triple::line_triple()
{
    node_h = NULL;
    node_t = NULL;
    node_r = NULL;
    triple_size = 0;
    triple_h = NULL;
    triple_t = NULL;
    triple_r = NULL;
    triple_file[0] = 0;
}

line_triple::~line_triple()
{
    node_h = NULL;
    node_t = NULL;
    node_r = NULL;
    triple_size = 0;
    if (triple_h != NULL) {free(triple_h); triple_h = NULL;}
    if (triple_t != NULL) {free(triple_t); triple_t = NULL;}
    if (triple_r != NULL) {free(triple_r); triple_r = NULL;}
    triple_file[0] = 0;
}

void line_triple::init(const char *file_name, line_node *p_h, line_node *p_t, line_node *p_r)
{
    strcpy(triple_file, file_name);
    node_h = p_h;
    node_t = p_t;
    node_r = p_r;
    
    triple trip;
    char sh[MAX_STRING], st[MAX_STRING], sr[MAX_STRING];
    int h, t, r;
    
    // compute the number of edges
    FILE *fi = fopen(triple_file, "rb");
    if (fi == NULL)
    {
        printf("ERROR: triple file not found!\n");
        printf("%s\n", triple_file);
        exit(1);
    }
    triple_size = 0;
    while (1)
    {
        if (fscanf(fi, "%s %s %s", sh, st, sr) != 3) break;
        
        if (triple_size % 10000 == 0)
        {
            printf("%lldK%c", triple_size / 1000, 13);
            fflush(stdout);
        }
        
        h = node_h->search(sh);
        t = node_t->search(st);
        r = node_r->search(sr);
        
        if (h == -1 || t == -1 || r == -1) continue;
        
        triple_size += 1;
    }
    fclose(fi);
    
    // allocate spaces
    triple_h = (int *)malloc(triple_size * sizeof(int));
    triple_t = (int *)malloc(triple_size * sizeof(int));
    triple_r = (int *)malloc(triple_size * sizeof(int));
    if (triple_h == NULL || triple_t == NULL || triple_r == NULL)
    {
        printf("Error: memory allocation failed!\n");
        exit(1);
    }
    
    // read edges
    fi = fopen(triple_file, "rb");
    int cnt = 0;
    while (1)
    {
        if (fscanf(fi, "%s %s %s", sh, st, sr) != 3) break;
        
        h = node_h->search(sh);
        t = node_t->search(st);
        r = node_r->search(sr);
        
        if (h == -1 || t == -1 || r == -1) continue;
        
        // store edges
        triple_h[cnt] = h;
        triple_t[cnt] = t;
        triple_r[cnt] = r;
        
        cnt += 1;
        
        trip.h = h;
        trip.r = r;
        trip.t = t;
        
        appear.insert(trip);
    }
    fclose(fi);
    
    printf("Reading edges from file: %s, DONE!\n", triple_file);
    printf("Edge size: %lld\n", triple_size);
}

void line_triple::train_ht(real lr, int dis_type, int h, int t, int r, int nh, int nt, int nr)
{
    int vector_size = node_r->vector_size;
    BLPVector dir_p, dir_n;
    dir_p.resize(vector_size);
    dir_n.resize(vector_size);
    real x = 0, sign;
    
    dir_p.array() = (node_t->vec.row(t) - node_h->vec.row(h)).array().abs();
    dir_n.array() = (node_t->vec.row(nt) - node_h->vec.row(nh)).array().abs();
    
    for (int c = 0; c != vector_size; c++)
    {
        if (dis_type == 1)
        {
            x = 2 * (dir_p(c) - node_r->vec(r, c));
            if (x > 0) x = 1;
            else x = -1;
        }
        else if (dis_type == 2)
        {
            x = 2 * (dir_p(c) - node_r->vec(r, c));
        }
        
        if (node_t->vec(t, c) - node_h->vec(h, c) > 0) sign = 1;
        else sign = -1;
        
        node_r->vec(r, c) += lr * x;
        node_h->vec(h, c) += lr * sign * x;
        node_t->vec(t, c) -= lr * sign * x;
        
        
        if (dis_type == 1)
        {
            x = 2 * (dir_n(c) - node_r->vec(nr, c));
            if (x > 0) x = 1;
            else x = -1;
        }
        else if (dis_type == 2)
        {
            x = 2 * (dir_n(c) - node_r->vec(nr, c));
        }
        
        if (node_t->vec(nt, c) - node_h->vec(nh, c) > 0) sign = 1;
        else sign = -1;
        
        node_r->vec(nr, c) -= lr * x;
        node_h->vec(nh, c) -= lr * sign * x;
        node_t->vec(nt, c) += lr * sign * x;
    }
    
    double norm;
    
    norm = node_r->vec.row(r).norm();
    if (norm > 1) node_r->vec.row(r) /= norm;
    //norm = node_h->vec.row(h).norm();
    //if (norm > 1) node_h->vec.row(h) /= norm;
    //norm = node_t->vec.row(t).norm();
    //if (norm > 1) node_t->vec.row(t) /= norm;
    
    norm = node_r->vec.row(nr).norm();
    if (norm > 1) node_r->vec.row(nr) /= norm;
    //norm = node_h->vec.row(nh).norm();
    //if (norm > 1) node_h->vec.row(nh) /= norm;
    //norm = node_t->vec.row(nt).norm();
    //if (norm > 1) node_t->vec.row(nt) /= norm;
}

void line_triple::train_sample(real lr, real margin, int dis_type, double (*func_rand_num)())
{
    int triple_id, h, t, r, neg;
    real sn = 0, sp = 0;
    triple trip;
    
    triple_id = triple_size * func_rand_num();
    
    h = triple_h[triple_id];
    t = triple_t[triple_id];
    r = triple_r[triple_id];
    
    int vector_size = node_r->vector_size;
    BLPVector dir_p, dir_n;
    dir_p.resize(vector_size);
    dir_n.resize(vector_size);
    
    double coin = func_rand_num();
    if (coin < 0.5)
    {
        neg = func_rand_num() * node_h->node_size;
        trip.h = neg; trip.t = t; trip.r = r;
        while (appear.count(trip))
        {
            neg = func_rand_num() * node_h->node_size;
            trip.h = neg; trip.t = t; trip.r = r;
        }
        
        dir_p.array() = (node_t->vec.row(t) - node_h->vec.row(h)).array().abs();
        dir_n.array() = (node_t->vec.row(t) - node_h->vec.row(neg)).array().abs();
        
        if (dis_type == 1)
        {
            sp = (dir_p - node_r->vec.row(r)).array().abs().sum();
            sn = (dir_n - node_r->vec.row(r)).array().abs().sum();
        }
        else if (dis_type == 2)
        {
            sp = (dir_p - node_r->vec.row(r)).array().pow(2).sum();
            sn = (dir_n - node_r->vec.row(r)).array().pow(2).sum();
        }
        
        if (sn - sp < margin)
        {
            train_ht(lr, dis_type, h, t, r, neg, t, r);
        }
    }
    else
    {
        neg = func_rand_num() * node_t->node_size;
        trip.h = h; trip.t = neg; trip.r = r;
        while (appear.count(trip))
        {
            neg = func_rand_num() * node_t->node_size;
            trip.h = h; trip.t = neg; trip.r = r;
        }
        
        dir_p.array() = (node_t->vec.row(t) - node_h->vec.row(h)).array().abs();
        dir_n.array() = (node_t->vec.row(neg) - node_h->vec.row(h)).array().abs();
        
        if (dis_type == 1)
        {
            sp = (dir_p - node_r->vec.row(r)).array().abs().sum();
            sn = (dir_n - node_r->vec.row(r)).array().abs().sum();
        }
        else if (dis_type == 2)
        {
            sp = (dir_p - node_r->vec.row(r)).array().pow(2).sum();
            sn = (dir_n - node_r->vec.row(r)).array().pow(2).sum();
        }
        
        if (sn - sp < margin)
        {
            train_ht(lr, dis_type, h, t, r, h, neg, r);
        }
    }
}


line_regularizer_line::line_regularizer_line()
{
    node = NULL;
    expTable = NULL;
}

line_regularizer_line::~line_regularizer_line()
{
    node = NULL;
    if (expTable != NULL) {free(expTable); expTable = NULL;}
}

void line_regularizer_line::init(line_node *p_node)
{
    node = p_node;
    
    expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
    for (int i = 0; i < EXP_TABLE_SIZE; i++) {
        expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
        expTable[i] = expTable[i] / (expTable[i] + 1);                   // Precompute f(x) = x / (x + 1)
    }
}

void line_regularizer_line::train_uv(real lr, int u, int v, int neg_samples, real *_error_vec, double (*func_rand_num)())
{
    int vector_size = node->vector_size;
    int target, label;
    real f, g;
    Eigen::Map<BLPVector> error_vec(_error_vec, vector_size);
    error_vec.setZero();
    
    for (int d = 0; d < neg_samples + 1; d++)
    {
        if (d == 0)
        {
            target = v;
            label = 1;
        }
        else
        {
            target = func_rand_num() * node->node_size;
            label = 0;
        }
        f = node->vec.row(u) * node->vec.row(target).transpose();
        if (f > MAX_EXP) g = (label - 1) * lr;
        else if (f < -MAX_EXP) g = (label - 0) * lr;
        else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * lr;
        error_vec += g * ((node->vec.row(target)));
        node->vec.row(target) += g * ((node->vec.row(u)));
    }
    node->vec.row(u) += error_vec;
    new (&error_vec) Eigen::Map<BLPMatrix>(NULL, 0, 0);
}

void line_regularizer_line::train_sample(real lr, int neg_samples, real *_error_vec, double (*func_rand_num)(), int depth, line_adjacency *p_adjacency)
{
    int u, v;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = p_adjacency->sample_head(func_rand_num);
    v = u;
    for (int k = 0; k != depth; k++)
    {
        v = p_adjacency->sample(v, func_rand_num);
        node_lst.push_back(v);
    }
    
    for (int k = 0; k != depth; k++)
    {
        v = node_lst[k];
        if (v == -1) continue;
        
        train_uv(lr, u, v, neg_samples, _error_vec, func_rand_num);
    }
}


line_regularizer_norm::line_regularizer_norm()
{
    node = NULL;
}

line_regularizer_norm::~line_regularizer_norm()
{
    node = NULL;
}

void line_regularizer_norm::init(line_node *p_node)
{
    node = p_node;
}

void line_regularizer_norm::train_uv(real lr, int dis_type, int u, int v)
{
    int vector_size = node->vector_size;
    real x = 0;
    
    for (int c = 0; c != vector_size; c++)
    {
        if (dis_type == 1)
        {
            x = 2 * (node->vec(u, c) - node->vec(v, c));
            if (x > 0) x = 1;
            else x = -1;
        }
        else if (dis_type == 2)
        {
            x = 2 * (node->vec(u, c) - node->vec(v, c));
        }
        node->vec(u, c) -= lr * x;
        node->vec(v, c) += lr * x;
    }
}

void line_regularizer_norm::train_uv_neg(real lr, int dis_type, int u, int v, int n)
{
    int vector_size = node->vector_size;
    real x = 0, y = 0;
    
    for (int c = 0; c != vector_size; c++)
    {
        if (dis_type == 1)
        {
            x = 2 * (node->vec(v, c) - node->vec(u, c));
            if (x > 0) x = 1;
            else x = -1;
        }
        else if (dis_type == 2)
        {
            x = 2 * (node->vec(v, c) - node->vec(u, c));
        }
        node->vec(v, c) -= lr * x;
        
        
        if (dis_type == 1)
        {
            y = 2 * (node->vec(n, c) - node->vec(u, c));
            if (y > 0) y = 1;
            else y = -1;
        }
        else if (dis_type == 2)
        {
            y = 2 * (node->vec(n, c) - node->vec(u, c));
        }
        node->vec(n, c) += lr * y;
        
        node->vec(u, c) += lr * (x - y);
    }
}

void line_regularizer_norm::train_sample(real lr, int dis_type, double (*func_rand_num)(), int depth, line_adjacency *p_adjacency)
{
    int u, v;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = p_adjacency->sample_head(func_rand_num);
    v = u;
    for (int k = 0; k != depth; k++)
    {
        v = p_adjacency->sample(v, func_rand_num);
        node_lst.push_back(v);
    }
    
    for (int k = 0; k != depth; k++)
    {
        v = node_lst[k];
        if (v == -1) continue;
        
        train_uv(lr, dis_type, u, v);
    }
}

void line_regularizer_norm::train_sample_neg(real lr, real margin, int dis_type, double (*func_rand_num)(), int depth, line_adjacency *p_adjacency)
{
    int u, v, neg;
    real sn = 0, sp = 0;
    std::vector<int> node_lst;
    
    node_lst.clear();
    
    u = p_adjacency->sample_head(func_rand_num);
    v = u;
    for (int k = 0; k != depth; k++)
    {
        v = p_adjacency->sample(v, func_rand_num);
        node_lst.push_back(v);
    }
    
    for (int k = 0; k != depth; k++)
    {
        v = node_lst[k];
        if (v == -1) continue;
        
        neg = func_rand_num() * node->node_size;
        
        if (dis_type == 1)
        {
            sp = (node->vec.row(u) - node->vec.row(v)).array().abs().sum();
            sn = (node->vec.row(u) - node->vec.row(neg)).array().abs().sum();
        }
        else if (dis_type == 2)
        {
            sp = (node->vec.row(u) - node->vec.row(v)).array().pow(2).sum();
            sn = (node->vec.row(u) - node->vec.row(neg)).array().pow(2).sum();
        }
        
        if (sn - sp < margin)
        {
            train_uv_neg(lr, dis_type, u, v, neg);
        }
    }
}

line_trainer_feature::line_trainer_feature()
{
    data_file[0] = 0;
    data_fea.clear();
    data_lab.clear();
    data_size = 0;
    fea = NULL;
    lab = NULL;
}

line_trainer_feature::~line_trainer_feature()
{
    data_file[0] = 0;
    data_fea.clear();
    data_lab.clear();
    data_size = 0;
    fea = NULL;
    lab = NULL;
}

void line_trainer_feature::ReadWord(char *word, FILE *fin)
{
    int a = 0, ch;
    while (!feof(fin)) {
        ch = fgetc(fin);
        if (ch == 13) continue;
        if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
            if (a > 0) {
                if (ch == '\n') ungetc(ch, fin);
                break;
            }
            if (ch == '\n') {
                strcpy(word, (char *)"</s>");
                return;
            }
            else continue;
        }
        word[a] = ch;
        a++;
        if (a >= MAX_STRING - 1) a--;   // Truncate too long words
    }
    word[a] = 0;
}

void line_trainer_feature::Softmax(BLPMatrix &mat)
{
    mat.array() = mat.array().exp();
    double sum = mat.sum();
    if (sum != 0) mat.array() /= sum;
}

void line_trainer_feature::init(const char *file_name, line_node *p_label, line_node *p_feature)
{
    strcpy(data_file, file_name);
    fea = p_feature;
    lab = p_label;
    if (fea->vector_size != lab->vector_size)
    {
        printf("ERROR: vector dimsions are not same!\n");
        exit(1);
    }
    
    FILE *fi = fopen(data_file, "rb");
    if (fi == NULL)
    {
        printf("ERROR: label-feature file not found!\n");
        printf("%s\n", data_file);
        exit(1);
    }
    char slab[MAX_STRING], sfea[MAX_STRING];
    std::vector<int> vtfea;
    int labid, feaid;
    data_size = 0;
    while (1)
    {
        if (fscanf(fi, "%s", slab) != 1) break;
        
        vtfea.clear();
        while (1)
        {
            ReadWord(sfea, fi);
            if (strcmp(sfea, "</s>") == 0) break;
            feaid = fea->search(sfea);
            if (feaid == -1) continue;
            vtfea.push_back(feaid);
        }
        
        labid = lab->search(slab);
        if (labid == -1) continue;
        if (vtfea.empty()) continue;
        
        data_size++;
        data_lab.push_back(labid);
        data_fea.push_back(vtfea);
    }
    fclose(fi);
    
    printf("Reading lab-fea from file: %s, DONE!\n", data_file);
    printf("Data size: %d\n", data_size);
}

void line_trainer_feature::train_sample(real lr, double (*func_rand_num)())
{
    if (data_size == 0) return;
    
    int data_id, feaid, labid, len;
    int vector_size = fea->vector_size;
    int label_size = lab->node_size;
    real target;
    std::vector<int> vtfea;
    BLPVector neu, err;
    
    neu.resize(vector_size);
    err.resize(vector_size);
    data_id = data_size * func_rand_num();
    labid = data_lab[data_id];
    vtfea = data_fea[data_id];
    len = (int)(vtfea.size());
    
    neu.setZero();
    for (int k = 0; k != len; k++)
    {
        feaid = vtfea[k];
        neu += fea->vec.row(feaid);
    }
    neu /= len;
    
    err.setZero();
    for (int l = 0; l != label_size; l++)
    {
        if (l == labid) target = 1;
        else target = 0;
        
        real f = lab->vec.row(l) * neu.transpose();
        f = 1.0 / (1.0 + exp(-f));
        real g = (target - f) * lr;
        
        err += g * lab->vec.row(l);
        lab->vec.row(l) += g * neu;
    }
    err /= len;
    for (int k = 0; k != len; k++)
    {
        feaid = vtfea[k];
        fea->vec.row(feaid) += err;
    }
}
