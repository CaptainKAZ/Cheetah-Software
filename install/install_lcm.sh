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
