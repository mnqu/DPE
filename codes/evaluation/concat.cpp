#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_word {
	long long cn;
	char word[MAX_STRING];
};

char vector_file1[MAX_STRING], vector_file2[MAX_STRING], output_file[MAX_STRING];
struct vocab_word *vocab;
int binary = 0, debug_mode = 2;
int *vocab_hash;
long long vocab_max_size = 1000, vocab_size = 0;
long long vector_size1, vector_size2;
real *syn0, *syn1;

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

// Reads a word and returns its index in the vocabulary
int ReadWordIndex(FILE *fin) {
	char word[MAX_STRING];
	ReadWord(word, fin);
	if (feof(fin)) return -1;
	return SearchVocab(word);
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word, int cn, int id) {
	unsigned int hash;
	strcpy(vocab[id].word, word);
	vocab[id].cn = cn;
	hash = GetWordHash(word);
	while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
	vocab_hash[hash] = id;
	return id;
}

void LearnVocabFromTrainFile()
{
	char ch, word[MAX_STRING];
	float f_num;
	long long l;

	vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
	for (long long a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;

	FILE *fi = fopen(vector_file1, "rb");
	if (fi == NULL) {
		printf("Vector file 1 not found\n");
		exit(1);
	}
	fscanf(fi, "%lld %lld", &vocab_size, &vector_size1);
	vocab = (struct vocab_word *)malloc(vocab_size * sizeof(struct vocab_word));
	syn0 = (real *)calloc(vocab_size * vector_size1, sizeof(real));
	for (long long k = 0; k != vocab_size; k++)
	{
		fscanf(fi, "%s", word);
		ch = fgetc(fi);
		AddWordToVocab(word, 0, k);
		l = k * vector_size1;
		for (int c = 0; c != vector_size1; c++)
		{
			fread(&f_num, sizeof(float), 1, fi);
			syn0[c + l] = (real)f_num;
		}
	}
	fclose(fi);

	fi = fopen(vector_file2, "rb");
	if (fi == NULL) {
		printf("Vector file 2 not found\n");
		exit(1);
	}
	fscanf(fi, "%lld %lld", &l, &vector_size2);
	syn1 = (real *)calloc((vocab_size + 1) * vector_size2, sizeof(real));
	for (long long k = 0; k != vocab_size; k++)
	{
		fscanf(fi, "%s", word);
		ch = fgetc(fi);
		int i = SearchVocab(word);
		if (i == -1) l = vocab_size * vector_size2;
		else l = i * vector_size2;
		for (int c = 0; c != vector_size1; c++)
		{
			fread(&f_num, sizeof(float), 1, fi);
			syn1[c + l] = (real)f_num;
		}
	}
	fclose(fi);

	if (debug_mode>0)
	{
		printf("Vocab size: %lld\n", vocab_size);
		printf("Vector size 1: %lld\n", vector_size1);
		printf("Vector size 2: %lld\n", vector_size2);
	}
}


void TrainModel() {
	long long a, b;
	double len;

	LearnVocabFromTrainFile();

	FILE *fo;
	fo = fopen(output_file, "wb");
	fprintf(fo, "%lld %lld\n", vocab_size, vector_size1 + vector_size2);
	for (a = 0; a < vocab_size; a++) {
		fprintf(fo, "%s ", vocab[a].word);

		len = 0;
		for (b = 0; b < vector_size1; b++) len += syn0[b + a * vector_size1] * syn0[b + a * vector_size1];
		len = sqrt(len);
		if (len != 0) for (b = 0; b < vector_size1; b++) syn0[b + a * vector_size1] /= len;

		len = 0;
		for (b = 0; b < vector_size2; b++) len += syn1[b + a * vector_size2] * syn1[b + a * vector_size2];
		len = sqrt(len);
		if (len != 0) for (b = 0; b < vector_size2; b++) syn1[b + a * vector_size2] /= len;

		if (binary)
		{
			for (b = 0; b < vector_size1; b++)
				fwrite(&syn0[a * vector_size1 + b], sizeof(real), 1, fo);
			for (b = 0; b < vector_size2; b++)
				fwrite(&syn1[a * vector_size2 + b], sizeof(real), 1, fo);
		}
		else
		{
			for (b = 0; b < vector_size1; b++)
				fprintf(fo, "%lf ", syn0[a * vector_size1 + b]);
			for (b = 0; b < vector_size2; b++)
				fprintf(fo, "%lf ", syn0[a * vector_size2 + b]);
		}
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
	if ((i = ArgPos((char *)"-input1", argc, argv)) > 0) strcpy(vector_file1, argv[i + 1]);
	if ((i = ArgPos((char *)"-input2", argc, argv)) > 0) strcpy(vector_file2, argv[i + 1]);
	if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
	vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
	vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
	TrainModel();
	return 0;
}
