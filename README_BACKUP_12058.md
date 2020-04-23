# Arduino-VGA
<<<<<<< HEAD
Displays simple VGA output with 6-bit color using an Arduino Mega 2560.
=======
Displays simple VGA output using an Arduino Mega 2560.

### Image Converter
I wrote a small python script `imageConverter.py` that will take an image and turn it into a byte array that can be used with the program. It doesn't take care of resizing or color dithering, so I recommend using a program like GIMP to take care of that. The `image.h` file contains some example images that can be included in the program by using `#define IMAGE_[NAME]` and then `#include 'image.h'`, and then the code at the top of `setup()` will copy that image from progmem into the framebuffer for display.
>>>>>>> e79c4ffccf3ae6f125fab889a8fb1fa61f336286
