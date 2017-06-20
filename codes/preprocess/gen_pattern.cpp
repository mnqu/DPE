#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#define MAX_STRING 500
using namespace std;

const int vocab_hash_size = 30000000;

typedef float real;                    // Precision of float numbers

struct vocab_word {
    char word[MAX_STRING];
};

char vocab_file[MAX_STRING], index_file[MAX_STRING], text_file[MAX_STRING], query_file[MAX_STRING], output_pattern_file[MAX_STRING];

struct vocab_word *vocab;
int *vocab_hash;
long long vocab_max_size = 1000, vocab_size = 0, doc_size = 0, vector_size = 0, query_size = 0;
real *vec;
std::set<int> *wid2did;
std::map<int,int> *wid2did2pst;
std::vector< std::vector<int> > doc;

void ReadWord(char *word, FILE *fin) {
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
            } else continue;
        }
        word[a] = ch;
        a++;
        if (a >= MAX_STRING - 1) a--;   // Truncate too long words
    }
    word[a] = 0;
}
int GetWordHash(char *word) {
    unsigned long long a, hash = 0;
    for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
    hash = hash % vocab_hash_size;
    return hash;
}

int SearchVocab(char *word) {
    unsigned int hash = GetWordHash(word);
    while (1) {
        if (vocab_hash[hash] == -1) return -1;
        if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
        hash = (hash + 1) % vocab_hash_size;
    }
    return -1;
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
    unsigned int hash, length = strlen(word) + 1;
    if (length > MAX_STRING) length = MAX_STRING;
    strcpy(vocab[vocab_size].word, word);
    vocab_size++;
    // Reallocate memory if needed
    if (vocab_size + 2 >= vocab_max_size) {
        vocab_max_size += 1000;
        vocab = (struct vocab_word *)realloc(vocab, vocab_max_size * sizeof(struct vocab_word));
    }
    hash = GetWordHash(word);
    while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
    vocab_hash[hash] = vocab_size - 1;
    return vocab_size - 1;
}

void ReadVocab()
{
    char word[MAX_STRING];
    FILE *fin;
    for (long long a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
    fin = fopen(vocab_file, "rb");
    if (fin == NULL) {
        printf("ERROR: training data file not found!\n");
        exit(1);
    }
    vocab_size = 0;
    while (1)
    {
        if (fscanf(fin, "%s", word) != 1) break;
        if (SearchVocab(word) != -1) continue;
        AddWordToVocab(word);
    }
    fclose(fin);
    printf("Vocab size: %lld\n", vocab_size);
    wid2did = new std::set<int> [vocab_size];
    wid2did2pst = new std::map<int,int> [vocab_size];
}

void ReadIndex()
{
    FILE *fi = fopen(index_file, "rb");
    char word[MAX_STRING * 10];
    int wid, did, pst;
    while (1)
    {
        if (fscanf(fi, "%s", word) != 1) break;
        
        wid = SearchVocab(word);
        
        while (1)
        {
            ReadWord(word, fi);
            if (strcmp(word, "</s>") == 0) break;
            sscanf(word, "%d:%d", &did, &pst);
            if (wid == -1) continue;
            wid2did[wid].insert(did);
            wid2did2pst[wid][did] = pst;
        }
    }
    fclose(fi);
}

void ReadText()
{
    FILE *fi = fopen(text_file, "rb");
    char word[MAX_STRING];
    int wid;
    std::vector<int> curdoc;
    doc_size = 0;
    while (1)
    {
        ReadWord(word, fi);
        if (feof(fi)) break;
        
        wid = SearchVocab(word);
        curdoc.push_back(wid);
        if (strcmp(word, "</s>") == 0)
        {
            doc.push_back(curdoc);
            curdoc.clear();
            doc_size++;
        }
    }
    fclose(fi);
    printf("Text size: %lld\n", doc_size);
}

void Output()
{
    FILE *fi, *fo;
    char wordu[MAX_STRING], wordv[MAX_STRING], label[MAX_STRING], word[MAX_STRING];
    int u, v, wid, did, i, j, pstu, pstv, cnt;
    int *buf = (int *)calloc(doc_size, sizeof(int));
    int *bg, *ed;
    double sum = 0;
    std::map<int, int>::iterator iter;
    std::vector<std::string> inbuf;
    fi = fopen(query_file, "rb");
    fo = fopen(output_pattern_file, "wb");
    while (1)
    {
        if (fscanf(fi, "%s %s %s", label, wordu, wordv) != 3) break;
        
        inbuf.clear();
        while (1)
        {
            ReadWord(word, fi);
            if (strcmp(word, "</s>") == 0) break;
            inbuf.push_back(word);
        }
        
        u = SearchVocab(wordu);
        v = SearchVocab(wordv);
        if (u == -1 || v == -1) continue;
        
        ed = set_intersection(wid2did[u].begin(), wid2did[u].end(), wid2did[v].begin(), wid2did[v].end(), buf);
        
        for (bg = buf; bg != ed; bg++)
        {
            did = *bg;
            
            pstu = wid2did2pst[u][did];
            pstv = wid2did2pst[v][did];
            
            if (pstu > pstv) {i = pstv; j = pstu;}
            else {i = pstu; j = pstv;}
            
            if (j - i > 6) continue;
            
            fprintf(fo, "%s", label);
            for (int k = i - 1; k <= j + 1; k++)
            {
                if (k == i || k == j) continue;
                
                wid = doc[did][k];
                if (wid == -1) continue;
                fprintf(fo, " %s", vocab[wid].word);
            }
            fprintf(fo, "\n");
        }
        
        query_size += 1;
    }
    fclose(fi);
    fclose(fo);
    free(buf);
    printf("Query size: %lld\n", query_size);
}

void TrainModel()
{
    vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
    vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
    
    ReadVocab();
    ReadIndex();
    ReadText();
    
    Output();
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
    if ((i = ArgPos((char *)"-vocab", argc, argv)) > 0) strcpy(vocab_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-index", argc, argv)) > 0) strcpy(index_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-text", argc, argv)) > 0) strcpy(text_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-query", argc, argv)) > 0) strcpy(query_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-pattern", argc, argv)) > 0) strcpy(output_pattern_file, argv[i + 1]);
    TrainModel();
    return 0;
}
