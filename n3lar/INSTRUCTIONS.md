# n3lar

This is an implementation of a machine learning approach to ASCII art rendering. As is the usual way, the program subdivides an image into a rectangular grid and replaces each cell with an **appropriate** font glyph. This program uses a **decision tree** to determine an ASCII glyph for each image region. The tree was learned by first converting a large set of images to ASCII art with the SSIM index (<http://en.wikipedia.org/wiki/Structural_similarity>) and then using the obtained data as a training set.

## Compiling the program

To compile and run the program, you will need:

* a C compiler
* OpenCV library, <http://opencv.org/>

The program can be compiled on *nix machines with the provided makefile (some small modifications are usually required).
Just make sure that you have installed all prerequisites.

The program uses OpenCV library to obtain data from webcams.
Thus, if you wish to redistribute it or its modifications in binary form, it has to reproduce the OpenCV license (read more at <http://opencv.willowgarage.com/wiki/>).

## Invoking the program

Run the program without any arguments (this is equivalent to double-clicking the executable file).
In this case, the program will read the frames from the webcam attached to the computer and render them using ASCII characters.

Contrast enhancing preprocessing step can be toggled by pressing `t`. It is based on the adaptive histogram equalization technique (CLAHE algorithm, <http://en.wikipedia.org/wiki/Adaptive_histogram_equalization>). In some cases, it can dramatically improve results. On the other hand, it is pretty computationally heavy. Thus, if you would like to port this program to a mobile device, you should omit it.

To terminate the execution of the program, press `q`.