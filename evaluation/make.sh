#!/bin/sh

g++ -O2 vocab.cpp -o vocab
g++ -O2 gen_cand_eval.cpp -o gen_cand_eval
g++ -O2 pair2bow.cpp -o pair2bow
