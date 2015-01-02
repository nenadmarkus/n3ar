#include <stdint.h>

int unpack_rendering_structures(uint8_t** ppixels, int* n, int* nrows, int* ncols, int32_t** tree, uint8_t pack[])
{
	int i, k;

	//
	*n = *(int*)&pack[0*sizeof(int)];

	*nrows = *(int*)&pack[1*sizeof(int)];
	*ncols = *(int*)&pack[2*sizeof(int)];

	k = 3*sizeof(int);

	//
	for(i=0; i<*n; ++i)
	{
		//
		ppixels[i] = &pack[k];

		//
		k = k + *nrows**ncols;
	}

	//
	*tree = (int32_t*)&pack[k];

	//
	return 1;
}

#define BINTEST(r, c, t, pixels, ldim) ( (pixels)[(r)*(ldim)+(c)] > (t) )

int get_tree_output(int32_t* tree, uint8_t* pixels, int ldim)
{
	int nodeidx;
	uint8_t* n;

	//
	nodeidx = 0;
	n = (uint8_t*)&tree[1];
	
	while(n[0] == 1) // while we are at a nonterminal node
	{
		//
		if( 0==BINTEST(n[1], n[2], n[3], pixels, ldim) )
			nodeidx = 2*nodeidx+1;
		else
			nodeidx = 2*nodeidx+2;

		//
		n = (uint8_t*)&tree[1+nodeidx];
	}
	
	return n[1]; //+ 256*n[2];
}