/*
	HOWTO: ls path/to/images/* | ./dgen 8x8 data
*/

/*
	includes
*/

#include <stdio.h>

#include <cv.h>
#include <highgui.h>

#include <time.h>
#include <math.h>

/*
	
*/

int dump_pixels(FILE* dst, uint8_t pixels[], int nrows, int ncols, int ldim)
{
	int r;

	//
	for(r=0; r<nrows; ++r)
		fwrite(&pixels[r*ldim], sizeof(uint8_t), ncols, dst);

	// we're done
	return 1;
}

/*
	
*/

int csize = 0;
uint8_t* cppixels[256];

int cnrows = 0;
int cncols = 0;

int load_codebook(const char* path)
{
	int i;
	FILE* file;

	//
	file = fopen(path, "rb");

	if(!file)
		return 0;

	//
	fread(&csize, sizeof(int), 1, file);
	fread(&cnrows, sizeof(int), 1, file);
	fread(&cncols, sizeof(int), 1, file);

	//
	for(i=0; i<csize; ++i)
	{
		//
		cppixels[i] = (uint8_t*)malloc(cnrows*cncols*sizeof(uint8_t));

		//
		fread(cppixels[i], sizeof(uint8_t), cnrows*cncols, file);
	}

	//
	fclose(file);

	//
	return 1;
}

/*
	
*/

int ssim(uint8_t* region, int ldim)
{
	int g, r, c;

	int mregion, s2region;

	static int init = 0;
	static int m[256], max, min, s2[256];

	int best;
	float bestsim;

	// histeq here?
	if(!init)
	{
		min = 255;
		max = 0;

		for(g=0; g<csize; ++g)
		{
			/*
				luminance
			*/
			m[g] = 0;

			for(r=0; r<cnrows; ++r)
				for(c=0; c<cncols; ++c)
					m[g] += cppixels[g][r*cncols+c];

			m[g] /= cnrows*cncols;

			if(m[g] < min)
				min = m[g];
			if(m[g] > max)
				max = m[g];

			/*
				variance
			*/
			s2[g] = 0;

			for(r=0; r<cnrows; ++r)
				for(c=0; c<cncols; ++c)
					s2[g] += (cppixels[g][r*cncols+c]-m[g])*(cppixels[g][r*cncols+c]-m[g]);

			s2[g] /= cnrows*cncols - 1;
		}

		//
		init = 1;
	}

	//
	mregion = 0;
	for(r=0; r<cnrows; ++r)
		for(c=0; c<cncols; ++c)
			mregion += region[r*ldim + c];
	mregion /= cnrows*cncols;

	s2region = 0;
	for(r=0; r<cnrows; ++r)
		for(c=0; c<cncols; ++c)
			s2region += (region[r*ldim+c]-mregion)*(region[r*ldim+c]-mregion);
	s2region /= cnrows*cncols - 1;

	//
	best = 0;
	bestsim = 0.0f;

	for(g=0; g<csize; ++g)
	{
		int cov;
		float L, CS;

		// luminance
		#define TRANSFORM(m) ( (255*((m)-min))/(max-min) )

		if(m[g]==0 && mregion==0)
			L = 1.0f;
		else
			L = 2.0f*mregion*TRANSFORM(m[g])/(float)( mregion*mregion + TRANSFORM(m[g])*TRANSFORM(m[g]) );

		// structure
		cov = 0;
		for(r=0; r<cnrows; ++r)
			for(c=0; c<cncols; ++c)
				cov += (region[r*ldim+c]-mregion)*(cppixels[g][r*cncols+c]-m[g]);
		cov /= cnrows*cncols - 1;

		if(s2region==0 && s2[g]==0)
			CS = 1.0f;
		else
			CS = 2.0f*cov/(float)(s2region + s2[g]);

		//
		static const float lexp = 10.0f;
		static const float csexp = 0.1f;

		if(pow(L, lexp)*pow(CS, csexp) >= bestsim)
		{
			best = g;
			bestsim = pow(L, lexp)*pow(CS, csexp);
		}
	}

	//
	return best;
}

/*
	
*/

int output_data(FILE* dst, uint8_t* pixels, int nrows, int ncols, int ldim)
{
	int r, c, n;

	//
	n = (nrows/cnrows)*(ncols/cncols);

	//
	for(r=0; r<nrows-cnrows-1; r+=cnrows)
	{
		for(c=0; c<ncols-cncols-1; c+=cncols)
		{
			int atom;

			//
			atom = ssim(&pixels[r*ldim+c], ldim);

			fwrite(&atom, 1, sizeof(int), dst);

			if(!dump_pixels(dst, &pixels[r*ldim+c], cnrows, cncols, ldim))
				return 0;
		}
	}

	//
	return n;
}

/*
	
*/

int main(int argc, char* argv[])
{
	char imgpath[1024];
	IplImage* img;

	int n;
	FILE* dst;

	//
	if(argc!=3)
	{
		printf("* specify arguments ...\n");

		return 1;
	}

	//
	if(!load_codebook(argv[1]))
	{
		printf("* cannot load codebook from '%s'\n", argv[1]);
		return 1;
	}

	//
	dst = fopen(argv[2], "wb");

	n = 0;
	fwrite(&n, 1, sizeof(int), dst);

	fwrite(&cnrows, 1, sizeof(int), dst);
	fwrite(&cncols, 1, sizeof(int), dst);

	//
	while(1==scanf("%s", imgpath))
	{
		img = cvLoadImage(imgpath, CV_LOAD_IMAGE_GRAYSCALE);
	
		if(!img)
		{
			printf("* failed to load image from '%s'\n", imgpath);
			//return 1;
		}
		else
		{
			n = n + output_data(dst, img->imageData, img->height, img->width, img->widthStep);

			cvReleaseImage(&img);
		}
	}

	//
	rewind(dst);
	fwrite(&n, 1, sizeof(int), dst);
	fclose(dst);

	//
	return 0;
}