Prepare SD card for sample
===========================================
1. Copy H264Codec\example\h264 folder to root directory of SDCard
2. Run the encoder or decoder sample


Encoder 
===========================================
Input file : 
	h264\pattern\foreman_qcif_2d.yuv

output file : 
	1. h264\encqcif.264 for encoded h264 bitstream
	2. h264\qcifframe_info.txt for each frame bitstream length information
	



Decoder 
===========================================
Input file :
	1. h264\patter\xxx.264 or xxx.jsv file (sample code will find all .264 or .jsv file in this folder to playback)
	2. h264\info\xxx.txt
(Note : The "xxx" means the file name and the name "xxx" for bitstream and info must be same.)

output to display device
Note : This sample uses GiantPlus GPM1006D QVGA panel as output deivce. 
If display device is change, please link correct display driver for display.


multi-decoder 
===========================================
Input file :
	1. h264\patter\xxx.264 or xxx.jsv file (sample code will find 4 .264 or .jsv files in this folder to playback)
	2. h264\info\xxx.txt
(Note : The "xxx" means the file name and the name "xxx" for bitstream and info must be same.)

output to display device
Note : This sample uses GiantPlus GPM1006D QVGA panel as output deivce. 
If display device is change, please link correct display driver for display.
