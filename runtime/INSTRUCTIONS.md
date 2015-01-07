# n3ar

## Compiling the program

To compile and run the program, you will need:

* a C compiler
* OpenCV library, <http://opencv.org/>

The program can be compiled on *nix machines with the provided `makefile` (some small modifications are usually required).
Just make sure that you have installed all prerequisites.

The program uses OpenCV library to obtain data from webcams.
Thus, if you wish to redistribute it or its modifications in binary form, it has to reproduce the OpenCV license (read more at <http://opencv.willowgarage.com/wiki/>).

## Invoking the program

The program has three modes of operation.

### Webcam input

Run the program without any arguments (this is equivalent to double-clicking the executable file).
In this case, the program will read the frames from the webcam attached to the computer and render them using ASCII characters.

Contrast enhancing preprocessing step can be toggled by pressing `t`.
It is based on the adaptive histogram equalization technique (CLAHE algorithm, <http://en.wikipedia.org/wiki/Adaptive_histogram_equalization>).
In some cases, it can dramatically improve results.
On the other hand, it is pretty computationally heavy.
Thus, if you would like to port this program to a mobile device, you should omit it.

To terminate the execution of the program, press `q`.

### View an image in ASCII

Run the program with a path to an image as a command line argument.
The program will load the image from the specified path and render it in ASCII with dithering techniques to improve reults.

    $ ./n3ar image.jpg

Contrast enhancing preprocessing step can be toggled with `t`.

### Output an ASCII image into a file

Run the program by passing the paths to the input image and the output.
The program will load the input image, transform it to ASCII and save it to the specified output path.

    $ ./n3ar input.jpg output.png

## Transforming a video to ASCII

The script `videotoascii.sh` enables you to transform a video file to ASCII. The pipeline is to decompose the video into a sequence of images using FFMPEG, transform each of them to ASCII with `n3lar` and then stitch these back to a video.

    $ sh videotoascii.sh video.mp4 asciivideo.mp4