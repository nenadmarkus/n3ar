# n3 ASCII art rendering

This is an implementation of a machine learning approach to ASCII art rendering.
As is the usual way, the program subdivides an image into a rectangular grid and replaces each cell with an appropriate font glyph.
This program uses a decision tree to determine an ASCII glyph for each image region.
The tree is learned by first converting a large set of images to ASCII art with the SSIM index (<http://en.wikipedia.org/wiki/Structural_similarity>) and then using the obtained data as a training set.

## Contact

For any additional information contact me at <nenad.markus@fer.hr>.

Copyright (c) 2013, Nenad Markus. All rights reserved.
