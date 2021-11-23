sudo apt install git -y
cd ~
git clone https://github.com/CaptainKAZ/Cheetah-Software.git
cd Cheetah-Software
cd scripts # for now, you must actually go into this folder
./make_types.sh # you may see an error like `rm: cannot remove...` but this is okay
cd ..
mkdir build
cd build
cmake .. # there are still some warnings here
make -j4
