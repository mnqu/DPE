#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_word {
    double cn;
    int *point;
    char *word, *code, codelen;
};

char train_file[MAX_STRING], vector_file[MAX_STRING], output_file[MAX_STRING];
struct vocab_word *vocab;
int binary = 0, debug_mode = 2;
int *vocab_hash;
long long vocab_max_size = 1000, vocab_size = 0, layer1_size = 100;
double *syn0, *syn1;

long long nedges;
int *edge_from, *edge_to;
double *edge_weight;

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
void ReadWord(char *word, FILE *fin) {
    int a = 0, ch;
    while (!feof(fin)) {
        ch = fgetc(fin);
        if (ch == 13) continue;
        if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
        {
            if (a > 0) break;
            else continue;
        }
        word[a] = ch;
        a++;
        if (a >= MAX_STRING - 1) a--;   // Truncate too long words
    }
    word[a] = 0;
}

// Returns hash value of a word
int GetWordHash(char *word) {
    unsigned long long a, hash = 0;
    for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
    hash = hash % vocab_hash_size;
    return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchVocab(char *word) {
    unsigned int hash = GetWordHash(word);
    while (1) {
        if (vocab_hash[hash] == -1) return -1;
        if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
        hash = (hash + 1) % vocab_hash_size;
    }
    return -1;
}

// Reads a word and returns its index in the vocabulary
int ReadWordIndex(FILE *fin) {
    char word[MAX_STRING];
    ReadWord(word, fin);
    if (feof(fin)) return -1;
    return SearchVocab(word);
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word, int id) {
    unsigned int hash, length = strlen(word) + 1;
    if (length > MAX_STRING) length = MAX_STRING;
    vocab[id].word = (char *)calloc(length, sizeof(char));
    strcpy(vocab[id].word, word);
    vocab[id].cn = 0;
    hash = GetWordHash(word);
    while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
    vocab_hash[hash] = id;
    return id;
}

void LearnVocabFromTrainFile()
{
    FILE *fi = fopen(vector_file, "rb");
    if (fi == NULL) {
        printf("Vector file not found\n");
        exit(1);
    }
    char ch, word[MAX_STRING];
    real f_num;
    
    fscanf(fi, "%lld %lld", &vocab_size, &layer1_size);
    
    vocab = (struct vocab_word *)malloc(vocab_size*sizeof(struct vocab_word));
    syn0 = (double *)malloc(vocab_size*layer1_size*sizeof(double));
    vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
    for (long long a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
    
    for (long long k = 0; k != vocab_size; k++)
    {
        fscanf(fi, "%s", word);
        ch = fgetc(fi);
        AddWordToVocab(word, k);
        for (int c = 0; c != layer1_size; c++)
        {
            fread(&f_num, sizeof(real), 1, fi);
            syn0[c + k * layer1_size] = (double)f_num;
        }
    }
    if (debug_mode>0)
    {
        printf("Vocab size: %lld\n", vocab_size);
        printf("Vector size: %lld\n", layer1_size);
    }
    
    fclose(fi);
}

void ReadNet()
{
    char word1[MAX_STRING], word2[MAX_STRING], str[MAX_STRING * 10];
    FILE *fin;
    
    fin = fopen(train_file, "rb");
    if (fin == NULL)
    {
        printf("ERROR: biterm data file not found!\n");
        exit(1);
    }
    nedges = 0;
    while (1)
    {
        if (fgets(str, sizeof(str), fin) == NULL) break;
        if (str[0] == '\r' || str[0] == '\n') break;
        nedges++;
    }
    fclose(fin);
    
    edge_weight = (double *)malloc(nedges*sizeof(double));
    edge_from = (int *)malloc(nedges*sizeof(int));
    edge_to = (int *)malloc(nedges*sizeof(int));
    
    if (edge_weight == NULL || edge_from == NULL || edge_to == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }
    
    fin = fopen(train_file, "rb");
    int n1, n2;
    long long pst = 0;
    double curweight;
    for (long long k = 0; k != nedges; k++)
    {
        if (k % 10000 == 0)
        {
            printf("%cRead network: %.3lf%%", 13, k / (double)(nedges + 1) * 100);
            fflush(stdout);
        }
        fscanf(fin, "%s %s %lf", word1, word2, &curweight);
        n1 = (int)(SearchVocab(word1));
        n2 = (int)(SearchVocab(word2));
        if (n1 < 0 || n2 < 0) continue;
        edge_from[pst] = n1;
        edge_to[pst] = n2;
        edge_weight[pst] = curweight;
        vocab[n1].cn += curweight;
        pst++;
    }
    nedges = pst;
    printf("\n");
    fclose(fin);
    if (debug_mode > 0) {
        printf("Edge size: %lld\n", nedges);
    }
}

void TrainModel() {
    long a, b;
    long long u, v, lu, lv;
    double w, p;
    FILE *fo;
    printf("Starting training using file %s\n", train_file);
    LearnVocabFromTrainFile();
    if (output_file[0] == 0) return;
    ReadNet();
    
    syn1 = (double *)malloc(vocab_size*layer1_size*sizeof(double));
    for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++) syn1[a * layer1_size + b] = 0;
    for (long long k = 0; k != nedges; k++)
    {
        u = edge_from[k];
        v = edge_to[k];
        w = edge_weight[k];
        p = w / vocab[u].cn;
        
        lu = u * layer1_size;
        lv = v * layer1_size;
        
        for (int c = 0; c != layer1_size; c++)
            syn1[lu + c] += syn0[lv + c] * p;
    }
    
    for (a = 0; a != vocab_size; a++)
    {
        double len = 0;
        for (b = 0; b != layer1_size; b++) len += syn1[a * layer1_size + b] * syn1[a * layer1_size + b];
        len = sqrt(len);
        if (len != 0) for (b = 0; b != layer1_size; b++) syn1[a * layer1_size + b] /= len;
        //else for (b = 0; b != layer1_size; b++) syn1[a * layer1_size + b] = 0.000001;
    }
    
    real f_num;
    fo = fopen(output_file, "wb");
    // Save the word vectors
    fprintf(fo, "%lld %lld\n", vocab_size, layer1_size);
    for (a = 0; a < vocab_size; a++) {
        fprintf(fo, "%s ", vocab[a].word);
        if (binary) for (b = 0; b < layer1_size; b++)
        {
            f_num = (real)(syn1[a * layer1_size + b]);
            fwrite(&f_num, sizeof(real), 1, fo);
        }
        else for (b = 0; b < layer1_size; b++) fprintf(fo, "%lf ", syn1[a * layer1_size + b]);
        fprintf(fo, "\n");
    }
    fclose(fo);
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
    output_file[0] = 0;
    if ((i = ArgPos((char *)"-train", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-vector", argc, argv)) > 0) strcpy(vector_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
    TrainModel();
    return 0;
}