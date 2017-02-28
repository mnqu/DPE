#DPE
This is an implementation of the DPE model for synonym discovery. 

We provide the training codes of DPE in the "model" folder, the evaluation codes in the "evaluation" folder. We also provide the Wiki dataset in the "data" folder.

##Install
Our codes rely on three external packages, which are the Eigen package, the GSL package and the liblinear package.

####Eigen
The [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) package is used for matrix operations. To run our codes, users need to download the Eigen package and modify the package path in the makefile.

####GSL
The [GSL](https://www.gnu.org/software/gsl/) package is used to generate random numbers. After installing the package, users also need to modify the package path in the makefile. 

####liblinear
We put the [liblinear](https://www.csie.ntu.edu.tw/~cjlin/liblinear/) package in the evaluation folder. Users can directly go to the folder and use the makefile to compile.

##Compile
After installing the two packages and modifying the package paths, users may go to every folder and use the makefile to compile the codes.

##Running
To run the DPE model, users may directly use the example script (run.sh) we provide.
