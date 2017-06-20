#!/bin/sh

text_file="../../data/text.txt"
pair_file="../../data/pairs.txt"
parsing_file="../../data/parsing.txt"
eval_file="../../data/eval.txt"
knn_file="../../data/knn.txt"

window=5
min_count=10
vocab_size=0

net_file="../../data/net.txt"
index_file="../../data/index.txt"
string_set="../../data/string.set"
pattern_file="../../data/pattern.txt"
bipartite_file="../../data/bipartite.txt"

./data2net -train ${text_file} -output ${net_file} -debug 2 -window ${window} -min-count ${min_count}
python vocab.py ${net_file} ${string_set}
python gen_index.py ${text_file} ${index_file}
python label.py ${pair_file} ${knn_file} pairs.txt
./vocab -train ${text_file} -output vocab.txt -min-count ${min_count} -size ${vocab_size}
./gen_pattern -index ${index_file} -text ${text_file} -vocab vocab.txt -query pairs.txt -output-pattern ${pattern_file} -parsing ${parsing_file}
python netww2netwe.py ${net_file} ${bipartite_file}

rm -rf pairs.txt vocab.txt
