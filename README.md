# DPE
This is an implementation of the DPE model proposed in the KDD 2017 paper ["Automatic Synonym Detection with Knowledge Bases"](https://arxiv.org/abs/1706.08186).

Given a corpus and a knowledge base, DPE will automatically discover missing entity synonyms from the corpus. Specifically, DPE leverages the idea of distant supervision and collects existing entity synonyms in knowledge bases as training seeds. The collected seeds are then used to train the DPE model, which aims at predicting whether two strings are synonymous or not. DPE has two modules: the distributional module predicts synonym relation from the corpus-level statistics, while the pattern module considers local contexts for prediction. At the inference stage, both modules will collaborate to discover high-quality entity synonyms.

We provide the codes for data preprocessing, model training and model evaluation in the "codes" folder. Also, we provide the Wiki-Freebase dataset in the "data" folder.

## Install
Our codes rely on two external packages, which are the Eigen package and the GSL package.

#### Eigen
The [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) package is used for matrix operations. To run our codes, users need to download the Eigen package and modify the package path in the makefile.

#### GSL
The [GSL](https://www.gnu.org/software/gsl/) package is used to generate random numbers. After installing the package, users also need to modify the package path in the makefile. 

## Compile
After installing the two packages and modifying the package paths, users may go to every folder and use the makefile to compile the codes.

## Running
To run the DPE model and evaluate it on the Wiki-Freebase dataset, users may directly use the example script (run.sh) we provide. By running this scipt, the program will first generate all the training data for DPE, such as the co-occurrence network of strings. Then it will learn the string embeddings as well as the distributional score function of the distributional module and the pattern classifier of the pattern module. Finally, the distributional module and the pattern module will mutually collaborate for synonym prediction.

Compiling, training and evaluating DPE on the Wiki-Freebase dataset.
```
./run.sh
```

## Contact: 
If you have any questions about the codes and data, please feel free to contact us.
```
Meng Qu, qumn123@gmail.com
```

## Citation
```
@article{qu2017automatic,
title={Automatic Synonym Discovery with Knowledge Bases},
author={Qu, Meng and Ren, Xiang and Han, Jiawei},
journal={arXiv preprint arXiv:1706.08186},
year={2017}
}
```
