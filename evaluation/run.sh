#!/bin/sh

text_file="../data/mapping.txt"
index_file="../data/index.txt"
eval_file="../data/eval.txt"
knn_vector_file="../model/word.emb"
cont_vector_file="../model/word.emb"
final_score="score_cmb.txt"

pat_file="../model/pat.txt"
dis_file="../model/dis.txt"

k_nns=100
k_max=10
min_count=10
vocab_size=0

weight=0.1

python gen_model.py ${pat_file} ${dis_file} model.txt
./vocab -train ${text_file} -output vocab.txt -min-count ${min_count} -size ${vocab_size}
./gen_cand_eval -data ${eval_file} -vector ${knn_vector_file} -output-cand cand.txt -output-pair pair.txt -k-max ${k_nns} -filter 1
./pair2bow -vector ${cont_vector_file} -index ${index_file} -text ${text_file} -vocab vocab.txt -query pair.txt -output-vector data-eval.txt -output-pair pair-eval.txt
./liblinear/predict -b 1 -q data-eval.txt model.txt predict.txt

python score_syn.py predict.txt pair-eval.txt score.txt

python final_score_wt.py cand.txt score.txt ${final_score} ${weight}

python cal_result.py ${eval_file} ${final_score} ${k_max}
echo "\n"
python cal_result_embed.py ${eval_file} cand.txt ${k_max}

rm -rf vocab.txt cand.txt pair.txt score.txt pair-eval.txt model.txt predict.txt