### introduction  
this project is using to merge multiple ros2 bag to one  
  
### make sure you have already install ros2 foxy or other  
  
### install independencies  
sudo apt-get install ros-foxy-sbg-driver  
sudo apt-get install ros-foxy-nmea-msgs  

### clone  
git clone https://github.com/goergehu/merge_bags.git  

### compile
cd merge_bags  
mkdir build  
cmake ..  
make  
  
### usage  
./bag_merge_tool path/to/db3/dir path/to/output  
