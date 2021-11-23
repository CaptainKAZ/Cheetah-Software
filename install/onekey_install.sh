#install dependencies
sudo apt install build-essential openjdk-8-jdk python-dev libeigen3-dev mesa-common-dev freeglut3-dev coinor-libipopt-dev libblas-dev liblapack-dev gfortran liblapack-dev coinor-libipopt-dev cmake gcc build-essential libglib2.0-dev git qt5-default libqt5gamepad5 libqt5gamepad5-dev -y
#install lcm
cd ~
wget https://github.com/lcm-proj/lcm/releases/download/v1.4.0/lcm-1.4.0.zip
unzip lcm-*.zip
rm lcm-*.zip
sudo apt install cmake make -y
cd lcm-1.4.0
mkdir build
cd build
cmake ..
make -j
sudo make install
#pull and build cheetah
cd ~
git clone https://github.com/CaptainKAZ/Cheetah-Software.git
cd Cheetah-Software
cd scripts # for now, you must actually go into this folder
./make_types.sh # you may see an error like `rm: cannot remove...` but this is okay
cd ..
mkdir build
cd build
cmake .. # there are still some warnings here
sudo apt install gcc-10 g++-10 -y #for tiger-lake
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10
make -j4
