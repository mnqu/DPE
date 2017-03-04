#!/bin/sh

string_set="../data/word.set"
occur_net="../data/net_word_clean.txt"
syn_net="../data/net_syn.txt"
pattern="../data/pattern.txt"

output_string="word.emb"
output_pat="pat.txt"
output_dis="dis.txt"

size=100
negative=5
samples=10000
alpha=0.01
threads=30

./dpe -string-set ${string_set} -label-set label.txt -occur-net ${occur_net} -syn-net ${syn_net} -pattern ${pattern} -output-string ${output_string} -output-pat ${output_pat} -output-dis ${output_dis} -binary 1 -size ${size} -negative ${negative} -samples ${samples} -alpha ${alpha} -threads ${threads}
