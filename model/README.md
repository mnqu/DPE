#Model
The folder contains the codes of the DPE model.

##Input
DPE relies on four input files, including the string vocabulary, co-occurrence network between strings, training patterns, and the synonym seeds.

####String vocabulary
The file consists of all strings in the vocabulary, with one string per line. You can find an example file as follows:
```
usa
america
canada
the
a
```

####Co-occurrence network
The file records the co-occurrence information between different strings. Each line contains a pair of strings and also their co-occurrence count. Below is an example:
```
usa america 5
america usa 5
usa the 10
the usa 10
usa canada 1
canada usa 1
```

####Training patterns
The file includes all patterns used in training. Each line represents a pattern, which starts with the pattern label, followed by the lexical and syntactic features. Below is an example:
```
1 known NNP VBN NNP nsubj acl xcomp
1 called NNP VBN NNP nsubj acl:relcl xcomp
```

####Synonym seeds
This file contains the synonym seeds used in training. Each line corresponds to a string pair with the synonym relation. Below is an example:
```
usa america 1
america usa 1
```

##Run
Users can directly use the script (run.sh) we provide to train the model.
