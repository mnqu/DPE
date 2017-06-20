#!/bin/sh

text_file="../../data/text.txt"
index_file="../../data/index.txt"
eval_file="../../data/eval.txt"
bipartite_file="../../data/bipartite.txt"

vector_file="../dpe/word.emb"
pat_file="../dpe/pat.txt"
dis_file="../dpe/dis.txt"

score_file="result.txt"

k_nns=100
k_max=10
min_count=10
vocab_size=0

weight=0.1

./infer -train ${bipartite_file} -vector ${vector_file} -output cont.emb -debug 2 -binary 1
./concat -input1 ${vector_file} -input2 cont.emb -output concat.emb -debug 2 -binary 1
python gen_model.py ${pat_file} ${dis_file} model.txt
./vocab -train ${text_file} -output vocab.txt -min-count ${min_count} -size ${vocab_size}
./gen_cand_eval -data ${eval_file} -vector concat.emb -output-cand cand.txt -output-pair pair.txt -k-max ${k_nns} -filter 1
./pair2bow -vector ${vector_file} -index ${index_file} -text ${text_file} -vocab vocab.txt -query pair.txt -output-vector data-eval.txt -output-pair pair-eval.txt
./liblinear/predict -b 1 -q data-eval.txt model.txt predict.txt

python score_syn.py predict.txt pair-eval.txt score.txt

python final_score_wt.py cand.txt score.txt ${score_file} ${weight}

python cal_result.py ${eval_file} ${score_file} ${k_max}

rm -rf vocab.txt cand.txt pair.txt score.txt pair-eval.txt model.txt predict.txt cont.emb concat.emb