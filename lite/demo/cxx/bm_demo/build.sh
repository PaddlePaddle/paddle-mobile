rm -f lib/bmcompiler
rm -f lib/pcie
ln -s $(pwd)/lib/bmcompiler1682 $(pwd)/lib/bmcompiler
ln -s $(pwd)/lib/pcie1682 $(pwd)/lib/pcie
mkdir ./build
cd ./build
cmake ..
make
cd ..
rm -rf ./build
