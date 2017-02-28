//  Copyright 2013 Google Inc. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <set>
#include <map>
#include <Eigen/Dense>
#include <iostream>

#define MAX_STRING 1000
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

typedef Eigen::Matrix< real, Eigen::Dynamic,
Eigen::Dynamic, Eigen::RowMajor | Eigen::AutoAlign >
BLPMatrix;

typedef Eigen::Matrix< real, 1, Eigen::Dynamic,
Eigen::RowMajor | Eigen::AutoAlign >
BLPVector;

struct vocab_word
{
    char word[MAX_STRING];
    int flag;
};

struct entry
{
    char ent[MAX_STRING];
    std::vector<int> train;
    std::set<int> check, right;
};

struct pair
{
    int id;
    real vl;
};

struct kmax_list
{
    pair *list;
    int k_max, list_size;
    
    void init(int k)
    {
        k_max = k;
        list = (pair *)malloc((k_max + 1) * sizeof(pair));
        list_size = 0;
        for (int k = 0; k != k_max + 1; k++)
        {
            list[k].id = -1;
            list[k].vl = -1;
        }
    }
    
    void clear()
    {
        list_size = 0;
        for (int k = 0; k != k_max + 1; k++)
        {
            list[k].id = -1;
            list[k].vl = -1;
        }
    }
    
    void add(pair pr)
    {
        list[list_size].id = pr.id;
        list[list_size].vl = pr.vl;
        
        for (int k = list_size - 1; k >= 0; k--)
        {
            if (list[k].vl < list[k + 1].vl)
            {
                int tmp_id = list[k].id;
                real tmp_vl = list[k].vl;
                list[k].id = list[k + 1].id;
                list[k].vl = list[k + 1].vl;
                list[k + 1].id = tmp_id;
                list[k + 1].vl = tmp_vl;
            }
            else
                break;
        }
        
        if (list_size < k_max) list_size++;
    }
};

char data_file[MAX_STRING], vector_file[MAX_STRING], output_cand_file[MAX_STRING], output_pair_file[MAX_STRING];
struct vocab_word *vocab;
int *vocab_hash;
int vocab_max_size = 1000, vocab_size = 0, vector_size = 0, data_size = 0, k_max = 5, filter = 0;

BLPMatrix vec;
std::vector<entry> data;

real score(int wid, std::vector<int> &train)
{
    real sc = 0;
    int size = (int)(train.size());
    for (int k = 0; k != size; k++)
    {
        int tid = train[k];
        real f = vec.row(wid) * vec.row(tid).transpose();
        //if (f > sc) sc = f;
        sc += f;
    }
    if (size == 0) sc = 0;
    else sc /= size;
    return sc;
}

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
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
            }
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

// Adds a word to the vocabulary
int AddWordToVocab(char *word, int id) {
    unsigned int hash;
    strcpy(vocab[id].word, word);
    vocab[id].flag = 0;
    hash = GetWordHash(word);
    while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
    vocab_hash[hash] = id;
    return id;
}

void ReadVector()
{
    FILE *fi;
    char ch, word[MAX_STRING];
    real f;
    
    fi = fopen(vector_file, "rb");
    if (fi == NULL) {
        printf("Vector file not found\n");
        exit(1);
    }
    
    fscanf(fi, "%d %d", &vocab_size, &vector_size);
    
    vocab = (struct vocab_word *)malloc(vocab_size*sizeof(struct vocab_word));
    vec.resize(vocab_size, vector_size);
    
    for (int k = 0; k != vocab_size; k++)
    {
        fscanf(fi, "%s", word);
        ch = fgetc(fi);
        AddWordToVocab(word, k);
        for (int c = 0; c != vector_size; c++)
        {
            fread(&f, sizeof(real), 1, fi);
            vec(k, c) = f;
        }
        
        vec.row(k) /= vec.row(k).norm();
    }
    
    printf("Vocab size: %d\n", vocab_size);
    printf("Vector size: %d\n", vector_size);
    
    fclose(fi);
}

void ReadData()
{
    FILE *fi;
    char ent[MAX_STRING], word[MAX_STRING];
    int wid;
    entry curentry;
    
    fi = fopen(data_file, "rb");
    while (1)
    {
        if (fscanf(fi, "%s", ent) != 1) break;
        ReadWord(word, fi);
        
        curentry.train.clear();
        curentry.check.clear();
        curentry.right.clear();
        
        while (1)
        {
            ReadWord(word, fi);
            if (strcmp(word, "</s>") == 0) break;
            wid = SearchVocab(word);
            if (wid == -1) continue;
            curentry.train.push_back(wid);
            curentry.check.insert(wid);
            curentry.right.insert(wid);
        }
        
        while (1)
        {
            ReadWord(word, fi);
            if (strcmp(word, "</s>") == 0) break;
            wid = SearchVocab(word);
            if (wid == -1) continue;
            curentry.right.insert(wid);
        }
        
        if (curentry.check.empty() || curentry.right.empty()) continue;
        
        strcpy(curentry.ent, ent);
        
        data.push_back(curentry);
    }
    
    data_size = (int)(data.size());
    
    printf("Data size: %d\n", data_size);
}

void Evaluate()
{
    pair pr;
    std::vector<int> train;
    std::set<int> check, right;
    
    kmax_list rklist;
    rklist.init(k_max);
    
    FILE *foc = fopen(output_cand_file, "wb");
    FILE *fop = fopen(output_pair_file, "wb");
    
    for (int data_id = 0; data_id != data_size; data_id++)
    {
        if (data_id % 10 == 0)
        {
            printf("%cProgress: %.2f%%", 13, 100.0 * data_id / data_size);
            fflush(stdout);
        }
        
        train = data[data_id].train;
        check = data[data_id].check;
        right = data[data_id].right;
        
        // calculate average
        rklist.clear();
        for (int k = 0; k != vocab_size; k++)
        {
            if (filter)
            {
                if (check.count(k)) continue;
            }
            
            int pst = -1;
            int length = strlen(vocab[k].word);
            for (int i = 0; i != length; i++) if (vocab[k].word[i] == '|')
            {
                pst = i;
                break;
            }
            if (pst != -1)
            {
                pst += 2;
                length -= 2;
                if (length - pst != strlen(data[data_id].ent)) continue;
                
                char curent[MAX_STRING];
                for (int i = 0; i != length - pst; i++) curent[i] = vocab[k].word[pst + i];
                curent[length - pst] = 0;
                    
                if (strcmp(data[data_id].ent, curent) != 0) continue;
            }
            
            real f = score(k, train);
            
            pr.id = k;
            pr.vl = f;
            rklist.add(pr);
        }
        
        for (int k = 0; k != k_max; k++)
        {
            int wid = rklist.list[k].id;
            real vl = rklist.list[k].vl;
            fprintf(foc, "%s\t%s\t%d\t%lf\n", data[data_id].ent, vocab[wid].word, k, vl);
        }
        
        for (int i = 0; i != (int)(train.size()); i++)
        {
            int tid = train[i];
            for (int k = 0; k != k_max; k++)
            {
                int wid = rklist.list[k].id;
                fprintf(fop, "0\t%s\t%s\t%s\t%d\t%d\n", vocab[tid].word, vocab[wid].word, data[data_id].ent, i, k);
            }
        }
    }
    printf("\n");
    fclose(foc);
    fclose(fop);
}

void TrainModel()
{
    ReadVector();
    ReadData();
    Evaluate();
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
        printf("WORD VECTOR estimation toolkit v 0.1b\n\n");
        printf("Options:\n");
        printf("Parameters for training:\n");
        printf("\t-train <file>\n");
        printf("\t\tUse text data from <file> to train the model\n");
        printf("\t-output <file>\n");
        printf("\t\tUse <file> to save the resulting word vectors / word clusters\n");
        printf("\t-size <int>\n");
        printf("\t\tSet size of word vectors; default is 100\n");
        printf("\nExamples:\n");
        printf("./btm2vec -train btm.txt -output vec.txt -debug 2 -size 200 -samples 100 -negative 5 -hs 0 -binary 1\n\n");
        return 0;
    }
    if ((i = ArgPos((char *)"-data", argc, argv)) > 0) strcpy(data_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-vector", argc, argv)) > 0) strcpy(vector_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-cand", argc, argv)) > 0) strcpy(output_cand_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-output-pair", argc, argv)) > 0) strcpy(output_pair_file, argv[i + 1]);
    if ((i = ArgPos((char *)"-k-max", argc, argv)) > 0) k_max = atoi(argv[i + 1]);
    if ((i = ArgPos((char *)"-filter", argc, argv)) > 0) filter = atoi(argv[i + 1]);
    vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
    vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
    for (long long a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
    TrainModel();
    return 0;
}
