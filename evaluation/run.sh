#!/bin/sh

text_file="../data/mapping.txt"
index_file="../data/index.txt"
eval_file="../data/eval-filter.txt"
knn_vector_file="../linelib/word.emb"
cont_vector_file="../linelib/word.emb"

k_nns=100
k_max=10
min_count=10
vocab_size=0

model_file="../evaluation/model.txt"
eval_vector_file="data-eval.txt"
eval_pair_file="pair-eval.txt"
predict_file="predict.txt"

vocab_file="vocab.txt"
cand_file="cand.txt"
pair_file="pair.txt"
syn_score="score.txt"
final_score="score_cmb.txt"

weight=0.1

./vocab -train ${text_file} -output ${vocab_file} -min-count ${min_count} -size ${vocab_size}
./gen_cand_eval -data ${eval_file} -vector ${knn_vector_file} -output-cand ${cand_file} -output-pair ${pair_file} -k-max ${k_nns} -filter 1
./pair2bow -vector ${cont_vector_file} -index ${index_file} -text ${text_file} -vocab ${vocab_file} -query ${pair_file} -output-vector ${eval_vector_file} -output-pair ${eval_pair_file}
./liblinear/predict -b 1 -q ${eval_vector_file} ${model_file} ${predict_file}

python score_syn.py ${predict_file} ${eval_pair_file} ${syn_score}

python final_score_wt.py ${cand_file} ${syn_score} ${final_score} ${weight}

python cal_result.py ${eval_file} ${final_score} ${k_max}
echo "\n"
python cal_result_embed.py ${eval_file} ${cand_file} ${k_max}
