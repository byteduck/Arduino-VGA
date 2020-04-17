#!/bin/python3
#Requires imageio

import os
import sys
import imageio

def convert(filename):
	image = imageio.imread(filename)
	print(f"#define IMAGE_WIDTH {int(image.shape[1]/2)}")
	print(f"#define IMAGE_HEIGHT {image.shape[0]}")
	ret = "const byte PROGMEM IMAGE[IMAGE_WIDTH*IMAGE_HEIGHT] = {\n"
	for y in range(len(image)):
		ret += "  "
		for x in range(0, len(image[y]), 2):
			byte = makePixel(image[y][x]) << 5
			byte += makePixel(image[y][x+1]) << 2
			ret += f"{byte:#0{4}x}" + ","
		ret += "\n"
	ret += "};"
	print(ret)
			
def makePixel(pixel):
	ret  = 1 if pixel[0] > 127 else 0
	ret += 2 if pixel[1] > 127 else 0
	ret += 4 if pixel[2] > 127 else 0
	return ret


if __name__  == "__main__":
	if len(sys.argv) == 2:
		print("Warning: this program doesn't take care of dithering or resizing.")
		convert(sys.argv[1])
	else:
		print("Usage: imageConverter image.bmp")
		exit(1)
