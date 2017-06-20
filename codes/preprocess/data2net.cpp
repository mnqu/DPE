#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <map>
#define MAX_STRING 200
using namespace std;

const int vocab_hash_size = 30000000;

struct vocab_word {
	long long cn;
	char *word;
};

struct biterm
{
	int u;
	int v;
	friend bool operator<(biterm b1,biterm b2)
	{
		if(b1.u==b2.u)
			return b1.v<b2.v;
		return b1.u<b2.u;
	}
};

map<biterm,long long> btm2cnt;
//map<string,long long> word2cnt;
long long totalword=0,totalbtm=0;
long long last_processed=0,processed=0,file_size=0,train_words=0;
int min_count=0,min_reduce=1,debug_mode=2,window;
char input_file[MAX_STRING],output_file[MAX_STRING];

struct vocab_word *vocab;
int *vocab_hash;
long long vocab_max_size = 1000, vocab_size = 0;

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

int ReadWordIndex(FILE *fin) {
	char word[MAX_STRING];
	ReadWord(word, fin);
	if (feof(fin)) return -1;
	return SearchVocab(word);
}

int AddWordToVocab(char *word) {
	unsigned int hash, length = strlen(word) + 1;
	if (length > MAX_STRING) length = MAX_STRING;
	vocab[vocab_size].word = (char *)calloc(length, sizeof(char));
	strcpy(vocab[vocab_size].word, word);
	vocab[vocab_size].cn = 0;
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

int VocabCompare(const void *a, const void *b) {
	return ((struct vocab_word *)b)->cn - ((struct vocab_word *)a)->cn;
}

void SortVocab() {
	int a, size;
	unsigned int hash;
	// Sort the vocabulary and keep </s> at the first position
	qsort(&vocab[1], vocab_size - 1, sizeof(struct vocab_word), VocabCompare);
	for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	size = vocab_size;
	train_words = 0;
	for (a = 0; a < size; a++) {
		// Words occuring less than min_count times will be discarded from the vocab
		if (vocab[a].cn < min_count) {
			vocab_size--;
			free(vocab[vocab_size].word);
		} else {
			// Hash will be re-computed, as after the sorting it is not actual
			hash=GetWordHash(vocab[a].word);
			while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
			vocab_hash[hash] = a;
			train_words += vocab[a].cn;
		}
	}
	vocab = (struct vocab_word *)realloc(vocab, (vocab_size + 1) * sizeof(struct vocab_word));
}

void ReduceVocab() {
	int a, b = 0;
	unsigned int hash;
	for (a = 0; a < vocab_size; a++) if (vocab[a].cn > min_reduce) {
		vocab[b].cn = vocab[a].cn;
		vocab[b].word = vocab[a].word;
		b++;
	} else free(vocab[a].word);
	vocab_size = b;
	for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	for (a = 0; a < vocab_size; a++) {
		// Hash will be re-computed, as it is not actual
		hash = GetWordHash(vocab[a].word);
		while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
		vocab_hash[hash] = a;
	}
	fflush(stdout);
	min_reduce++;
}

void LearnVocabFromTrainFile() {
	char word[MAX_STRING];
	FILE *fin;
	long long a, i;
	for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	fin = fopen(input_file, "rb");
	if (fin == NULL) {
		printf("ERROR: training data file not found!\n");
		exit(1);
	}
	vocab_size = 0;
	AddWordToVocab((char *)"</s>");
	while (1) {
		ReadWord(word, fin);
		if (feof(fin)) break;
		train_words++;
		if ((debug_mode > 1) && (train_words % 100000 == 0)) {
			printf("%lldK%c", train_words / 1000, 13);
			fflush(stdout);
		}
		i = SearchVocab(word);
		if (i == -1) {
			a = AddWordToVocab(word);
			vocab[a].cn = 1;
		} else vocab[i].cn++;
		if (vocab_size > vocab_hash_size * 0.7) ReduceVocab();
	}
	SortVocab();
	if (debug_mode > 0) {
		printf("Vocab size: %lld\n", vocab_size);
		printf("Words in train file: %lld\n", train_words);
	}
	file_size = ftell(fin);
	fclose(fin);
}

void TrainModel()
{    
	vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
	vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
	LearnVocabFromTrainFile();

	FILE *fi=fopen(input_file,"r");
	fseek(fi, 0, SEEK_END);
	file_size=ftell(fi);
	fclose(fi);

	fi=fopen(input_file,"r");
	int word;
	biterm btm;
	int *buf=new int [window+2];
	int pst=0,exch=0;
	while(!feof(fi))
	{
		word=ReadWordIndex(fi);
		if(word==-1) continue;

		if(processed-last_processed>10000)
		{
			printf("%cRead file: %.3lf%%", 13, double(processed)/train_words*100);
			fflush(stdout);
			last_processed=processed;
		}

		processed++;
		if(word==0) { pst=0; exch=0; continue; }
		

		//word2cnt[word]++;
		//totalword++;
		//btm.u=word;btm.v=word;btm2cnt[btm]++;
		//totalbtm++;
		for(int k=0;k!=pst;k++)
		{
			btm.u=word;btm.v=buf[k];btm2cnt[btm]++;
			btm.u=buf[k];btm.v=word;btm2cnt[btm]++;

			totalbtm+=2;
		}

		if(pst<window)
			buf[pst++]=word;
		else
		{
			buf[exch++]=word;
			if(exch>=window) exch=0;
		}
	}
	printf("\n");
	fclose(fi);

	FILE *fo=fopen(output_file,"w");
	long long btmsize=btm2cnt.size();
	printf("Number of edges: %lld\n", btmsize);
	long long written=0;
	map<biterm,long long>::iterator iter=btm2cnt.begin();
	while(iter!=btm2cnt.end())
	{
		if(written%10000==0)
		{
			printf("%cWrite file: %.3lf%%", 13, double(written)/btmsize*100);
			fflush(stdout);
		}
		fprintf(fo,"%s\t%s\t%lld\n",vocab[(iter->first).u].word,vocab[(iter->first).v].word,(iter->second));

		written++;
		iter++;
	}
	printf("\n");
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
	if ((i = ArgPos((char *)"-train", argc, argv)) > 0) strcpy(input_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-window", argc, argv)) > 0) window = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0) min_count = atoi(argv[i + 1]);
	TrainModel();
	return 0;
}
