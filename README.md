# n3ar: An ASCII art rendering library

This is an official implementation of the ASCII art rendering technique described in the following paper:

> N. Markus, M. Fratarcangeli, I. S. Pandzic and J. Ahlberg, "Fast Rendering of Image Mosaics and ASCII Art", Computer Graphics Forum, 2015, <dx.doi.org/10.1111/cgf.12597>

## The basic idea behind the algorithm

As is the usual way for rendering tone-based ASCII art, we subdivide an image into a rectangular grid and replace each cell with an appropriate font glyph.
We use a decision tree to determine an ASCII glyph for each image region.
The tree is learned by first converting a large set of images to ASCII art with the SSIM index (<http://en.wikipedia.org/wiki/Structural_similarity>) and then using the obtained data as a training set.

## Contact

For any additional information contact me at <nenad.markus@fer.hr>.

Copyright (c) 2013, Nenad Markus. All rights reserved.
