# Arduino-VGA
Displays simple VGA output using an Arduino Mega 2560.

### Image Converter
I wrote a small python script `imageConverter.py` that will take an image and turn it into a byte array that can be used with the program. It doesn't take care of resizing or color dithering, so I recommend using a program like GIMP to take care of that. The `image.h` file contains some example images that can be included in the program by using `#define IMAGE_[NAME]` and then `#include 'image.h'`, and then the code at the top of `setup()` will copy that image from progmem into the framebuffer for display.
