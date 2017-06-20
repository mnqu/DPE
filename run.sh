#!/bin/sh

### Compile the codes
cd codes

cd preprocess
./make.sh
cd ..

cd dpe
make
cd ..

cd evaluation
./make.sh
cd ..

### Data preprocessing
cd preprocess
./run.sh
cd ..

### Model training
cd dpe
./run.sh
cd ..

### Evaluation
cd evaluation
./run.sh
cd ..

### Clean temporary files
cd ../data
rm -rf net.txt index.txt string.set pattern.txt bipartite.txt
cd ..