This text will guide you through the process of learning your own decision tree-based ASCII art renderer.

## 1. Compile `dgen.c` and `tlrn.c`

Use the provided `makefile`.

## 2. Get a large set of images

Approximately 200 five Mpixel images will do.

* Download a file from <https://googledrive.com/host/0Bw4IT5ZOzJj6NXlJUFh0UGZCWmc>
* Rename `0Bw4IT5ZOzJj6NXlJUFh0UGZCWmc` to `images.tar`
* Extract the contents to some folder: `path/to/images`

## 3. Generate the training set

Use the following command:

	$ ls path/to/images/* | ./dgen codebook dataset

If everything went well, you should find a file `dataset` in the folder.

## 4. Start the learning process

We will learn a tree of depth equal to 12 with the following command:

	$ ./tlrn dataset 12 tree

Learning should finish in less than an hour on a modern multicore machine.
If everything went well, you should find a file `tree` in the folder.

## 5. Prepare the rendering structure

The rendering structure consists of a codebook and a corresponding decision tree.
In this implementation, this is simply a concatenation of the two data structures:

	$ cat codebook > rstruct 
	$ cat tree >> rstruct

Convert the file into a hex array:

	$ tohexarray.py rstruct > rstruct.array

## 6. Try the generated renderer

Move to the folder `runtime/`.
Replace the file `rstruct.array` with the new one.
Compile and run the program.