#!/bin/sh

string_set="../../data/string.set"
occur_net="../../data/net.txt"
syn_net="../../data/pairs.txt"
pattern="../../data/pattern.txt"
label_set="../../data/label.set"

output_string="word.emb"
output_pat="pat.txt"
output_dis="dis.txt"

size=100
negative=5
samples=10000
alpha=0.01
threads=30

./dpe -string-set ${string_set} -label-set ${label_set} -occur-net ${occur_net} -syn-net ${syn_net} -pattern ${pattern} -output-string ${output_string} -output-pat ${output_pat} -output-dis ${output_dis} -binary 1 -size ${size} -negative ${negative} -samples ${samples} -alpha ${alpha} -threads ${threads}
