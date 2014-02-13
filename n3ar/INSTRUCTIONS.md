# n3ar

This is a simple implementation of an ASCII art render developed in my spare time. As is the usual way, the program subdivides an image into a rectangular grid and replaces each cell with a font glyph in such a way that the average grey level stays approximately the same. I decided to use the following eight ASCII charasters: `WGCc;,. `. These provide visually pleasing results while you need only three bits to encode each of them. This makes it possible to stream the generated ASCII art over networks with limited bandwith. The main distinguishing characteristics of my approach to generating ASCII art is the preprocessing step which involves adaptive histogram equalization technique (CLAHE algorithm, <http://en.wikipedia.org/wiki/Adaptive_histogram_equalization>) to dramatically improve results.

## Compiling the program

To compile and run the program, you will need:

* a C compiler
* *nix OS with ncurses library
* OpenCV library, <http://opencv.org/>

The program can be compiled on *nix machines by using the provided makefile (some small modifications are usually required).
Just make sure that you have installed all prerequisites.

The program uses OpenCV library to obtain data from webcams.
Thus, if you wish to redistribute it or its modifications in binary form, it has to reproduce the OpenCV license (read more at <http://opencv.willowgarage.com/wiki/>).

Also, you can omit the linking with OpenCV by passing the flag `OMIT_OPENCV` to the compiler. In this case, the program can only operate as a receiver of ASCII art stream (see the next section for available modes of operation).
 
## Invoking the program

There are three ways of invoking the program:

1. Run the program without any arguments (this is equivalent to double-clicking the executable file). In this case, the program will read the frames from the webcam attached to the computer and render them using ASCII characters.

	$ ./n3ar

2. Run the program by passing one integer, `PORT`, as a command line argument. The program will expect a stream of data on port number `PORT`. After a neccessary ammount of data is collected, the program will render an image in ASCII art.

	$ ./n3ar 1337

3. Run the program by passing a string, `IP_ADDRESS`, and an integer, `PORT`, as command line arguments. The program will read the frames from the webcam attached to the computer and send them to port `PORT` of the computer with an IP address `IP_ADDRESS`.

	$ ./n3ar 192.168.1.35 1337

Combining 2 and 3 enables the user to stream ASCII art between two computers in the same network. Notice that the receiver machine does not need to have a graphical user interface.

To terminate the execution of the program, press `q`.
