#!/bin/python3
#Requires imageio

import os
import sys
import imageio

def convert(filename):
	image = imageio.imread(filename)
	print(f"#define IMAGE_WIDTH {int(image.shape[1])}")
	print(f"#define IMAGE_HEIGHT {image.shape[0]}")
	ret = "const byte PROGMEM IMAGE[IMAGE_WIDTH*IMAGE_HEIGHT] = {\n"
	for y in range(len(image)):
		ret += "  "
		for x in range(len(image[y])):
			pixel = image[y][x]
			byte  = getColor(pixel[0]) << 2
			byte += getColor(pixel[1]) << 4
			byte += getColor(pixel[2]) << 6
			ret += f"{byte:#0{4}x}" + ","
		ret += "\n"
	ret += "};"
	print(ret)
			
def getColor(color):
	if color < 64:
		return 0
	elif color < 128:
		return 1
	elif color < 192:
		return 2
	else:
		return 3


if __name__  == "__main__":
	if len(sys.argv) == 2:
		print("Warning: this program doesn't take care of dithering or resizing.")
		convert(sys.argv[1])
	else:
		print("Usage: imageConverter image.bmp")
		exit(1)
