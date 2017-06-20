#!/bin/sh

g++ -O2 vocab.cpp -o vocab
g++ -O2 -I../../eigen-3.2.5 gen_cand_eval.cpp -o gen_cand_eval
g++ -O2 -I../../eigen-3.2.5 infer.cpp -o infer
g++ -O2 concat.cpp -o concat
g++ -O2 pair2bow.cpp -o pair2bow
cd liblinear/
make
cd ..
