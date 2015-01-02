#include <stdio.h>
#include <stdint.h>

#include <cv.h>
#include <highgui.h>

#include "n3ar.c"

/*
	rendering structures
*/

//
int glyphnum = 0;
uint8_t* glyphs[256];

int glyphnrows = 0;
int glyphncols = 0;

//
int32_t* tree = 0;

/*
	
*/

void RCLAHEM(uint8_t imap[], uint8_t img[], int i0, int j0, int i1, int j1, int ldim, uint8_t s)
{
	//
	#define NBINS 256
	double p[NBINS];
	double P[NBINS];

	int i, j, k;
	int nrows, ncols;

	//
	nrows = i1 - i0 + 1;
	ncols = j1 - j0 + 1;

	//
	for(i=0; i<NBINS; ++i)
		p[i] = 0.0;

	// compute histogram
	for(i=i0; i<=i1; ++i)
		for(j=j0; j<=j1; ++j)
		{
			k = img[i*ldim + j];

			p[k] = p[k] + 1.0/(nrows*ncols);
		}

	// clip the histogram (ideally, we should do a few iterations of this)
	for(k=0; k<NBINS; ++k)
	{
		if(p[k] >= (double)s/NBINS)
		{
			//
			double d;

			//
			d = p[k] - (double)s/NBINS;

			//
			p[k] = (double)s/NBINS;
			
			// redistribute d
			for(i=0; i<NBINS; ++i)
				p[i] += d/NBINS;
		}
	}

	// compute cumulative histogram
	P[0] = p[0];
	for(i=1; i<NBINS; ++i)
		P[i] = P[i-1] + p[i];

	// compute intensity map
	for(k=0; k<NBINS; ++k)
		imap[k] = (NBINS-1)*P[k];
}

void CLAHE(uint8_t out[], uint8_t in[], int nrows, int ncols, int ldim, int di, int dj, uint8_t s)
{	
	//
	#define MAXDIVS 16
	uint8_t imaps[MAXDIVS][MAXDIVS][NBINS];

	int ics[MAXDIVS], jcs[MAXDIVS];

	int i, j, k, l, i0, j0, i1, j1, I, J;

	uint8_t v00, v01, v10, v11;

	//
	i0 = 0;
	j0 = 0;

	I = nrows;
	J = ncols;

	for(i=0; i<di; ++i)
	{
		for(j=0; j<dj; ++j)
		{	
			//
			i0 = i*I/di;
			j0 = j*J/dj;

			i1 = (i+1)*I/di;
			j1 = (j+1)*J/dj;

			if(i1>=I)
				i1 = I - 1;
	
			if(j1 >= J)
				j1 = J - 1;

			//
			RCLAHEM(imaps[i][j], in, i0, j0, i1, j1, ldim, s);

			//
			ics[i] = (i0 + i1)/2;
			jcs[j] = (j0 + j1)/2;
		}
	}

	// SPECIAL CASE: image corners
	for(i=0; i<ics[0]; ++i)
	{
		for(j=0; j<jcs[0]; ++j)
			out[i*ldim + j] = imaps[0][0][ in[i*ldim + j] ];

		for(j=jcs[dj-1]; j<J; ++j)
			out[i*ldim + j] = imaps[0][dj-1][ in[i*ldim + j] ];
	}

	for(i=ics[di-1]; i<I; ++i)
	{
		for(j=0; j<jcs[0]; ++j)
			out[i*ldim + j] = imaps[di-1][0][ in[i*ldim + j] ];

		for(j=jcs[dj-1]; j<J; ++j)	
			out[i*ldim + j] = imaps[di-1][dj-1][ in[i*ldim + j] ];
	}

	// SPECIAL CASE: image boundaries
	for(k=0; k<di-1; ++k)
	{
		for(i=ics[k]; i<ics[k+1]; ++i)
		{
			for(j=0; j<jcs[0]; ++j)
			{
				v00 = imaps[k+0][0][ in[i*ldim + j] ];
				v10 = imaps[k+1][0][ in[i*ldim + j] ];

				out[i*ldim + j] = ( (ics[k+1]-i)*v00 + (i-ics[k])*v10 )/(ics[k+1] - ics[k]);
			}

			for(j=jcs[dj-1]; j<J; ++j)
			{
				v01 = imaps[k+0][dj-1][ in[i*ldim + j] ];
				v11 = imaps[k+1][dj-1][ in[i*ldim + j] ];

				out[i*ldim + j] = ( (ics[k+1]-i)*v01 + (i-ics[k])*v11 )/(ics[k+1] - ics[k]);
			}
		}
	}

	for(k=0; k<dj-1; ++k)
		for(j=jcs[k]; j<jcs[k+1]; ++j)
		{
			for(i=0; i<ics[0]; ++i)
			{
				v00 = imaps[0][k+0][ in[i*ldim + j] ];
				v01 = imaps[0][k+1][ in[i*ldim + j] ];

				out[i*ldim + j] = ( (jcs[k+1]-j)*v00 + (j-jcs[k])*v01 )/(jcs[k+1] - jcs[k]);
			}

			for(i=ics[di-1]; i<I; ++i)
			{
				v10 = imaps[di-1][k+0][ in[i*ldim + j] ];
				v11 = imaps[di-1][k+1][ in[i*ldim + j] ];

				out[i*ldim + j] = ( (jcs[k+1]-j)*v10 + (j-jcs[k])*v11 )/(jcs[k+1] - jcs[k]);
			}
		}

	//
	for(k=0; k<di-1; ++k)
		for(l=0; l<dj-1; ++l)
			for(j=jcs[l]; j<jcs[l+1]; ++j)
				for(i=ics[k]; i<ics[k+1]; ++i)
				{
					uint8_t p;

					p = in[i*ldim + j];

					v00 = imaps[k+0][l+0][p];
					v01 = imaps[k+0][l+1][p];
					v10 = imaps[k+1][l+0][p];
					v11 = imaps[k+1][l+1][p];

					out[i*ldim + j] =
						(
							(ics[k+1]-i)*(jcs[l+1]-j)*v00 + (ics[k+1]-i)*(j-jcs[l])*v01 + (i-ics[k])*(jcs[l+1]-j)*v10 + (i-ics[k])*(j-jcs[l])*v11
						)/((ics[k+1] - ics[k])*(jcs[l+1] - jcs[l]));
				}
}

/*
	
*/

void compute_index_matrix(uint8_t indexmatrix[], uint8_t pixels[], int nrows, int ncols, int ldim)
{
	int i;

	int r, c;

	//
	i = 0;

	for(r=0; r<nrows; r+=glyphnrows)
	{
		for(c=0; c<ncols; c+=glyphncols)
		{
			//
			if(tree)
				indexmatrix[i] = get_tree_output(tree, &pixels[r*ldim+c], ldim);
			else
				indexmatrix[i] = 0;

			//
			++i;
		}
	}
}

void transform_to_ascii(uint8_t pixels[], int* nrows, int* ncols, int ldim)
{
	static uint8_t indexmatrix[640*480];

	int r, c;

	int n;

	//
	*nrows = (*nrows/glyphnrows)*glyphnrows;
	*ncols = (*ncols/glyphncols)*glyphncols;

	//
	compute_index_matrix(indexmatrix, pixels, *nrows, *ncols, ldim);

	//
	n = 0;

	for(r=0; r<*nrows; r+=glyphnrows)
		for(c=0; c<*ncols; c+=glyphncols)
		{
			int i, j, idx;
			uint8_t* glyph;

			//
			idx = indexmatrix[ n ];

			++n;

			//
			glyph = (uint8_t*)&glyphs[idx][0];

			for(i=0; i<glyphnrows; ++i)
				for(j=0; j<glyphncols; ++j)
					pixels[(r+i)*ldim+(c+j)] = glyph[i*glyphncols+j];
		}
}

/*
	
*/

void display_image(uint8_t pixels[], int nrows, int ncols, int ldim)
{
	IplImage* header;

	//
	header = cvCreateImageHeader(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);

	header->height = nrows;
	header->width = ncols;
	header->widthStep = ldim;

	header->imageData = (char*)pixels;

	//
	cvShowImage("n3lar", header);

	//
	cvReleaseImageHeader(&header);
}

/*
	
*/

uint8_t* load_image_from_file(char path[], int* nrows, int* ncols, int* ldim)
{
	/*
		Yes, I am aware that this function produces a memory leak.
	*/

	//
	IplImage* img;

	//
	img = cvLoadImage(path, CV_LOAD_IMAGE_GRAYSCALE);

	if(!img)
		return 0;

	//
	*nrows = img->height;
	*ncols = img->width;
	*ldim = img->widthStep;

	return (uint8_t*)img->imageData;
}

int save_image_to_file(char path[], uint8_t pixels[], int nrows, int ncols, int ldim)
{
	IplImage* header;

	//
	header = cvCreateImageHeader(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);

	header->height = nrows;
	header->width = ncols;
	header->widthStep = ldim;

	header->imageData = (char*)pixels;

	//
	cvSaveImage(path, header, 0);

	//
	cvReleaseImageHeader(&header);

	//
	return 1;
}

int process_image(char src[], char dst[])
{
	uint8_t* pixels;
	int nrows, ncols, ldim;

	//
	pixels = load_image_from_file(src, &nrows, &ncols, &ldim);

	if(!pixels)
		return 0;

	//
	CLAHE(pixels, pixels, nrows, ncols, ldim, 8, 8, 3);

	transform_to_ascii(pixels, &nrows, &ncols, ldim);

	//
	if(!save_image_to_file(dst, pixels, nrows, ncols, ldim))
		return 0;

	//
	return 1;
}

/*
	
*/

void view_in_ascii(char src[])
{
	int stop;

	uint8_t* pixels;
	int nrows, ncols, ldim;

	uint8_t* tmppixels;

	int useclahe = 0;

	//
	pixels = load_image_from_file(src, &nrows, &ncols, &ldim);

	if(!pixels)
		return;

	//
	tmppixels = (uint8_t*)malloc(nrows*ldim*sizeof(uint8_t));

	//
	stop = 0;

	while(!stop)
	{
		int key;

		// get the pressed key ...
		key = cvWaitKey(5);

		//
		memcpy(tmppixels, pixels, nrows*ldim*sizeof(uint8_t));

		//
		if(key == 't')
			useclahe = ~useclahe;

		//
		if(key=='q')
			stop = 1;
		else
		{
			int r, c, _nrows, _ncols;

			//
			r = rand()%( 3 );
			c = rand()%( 3 );

			_nrows = nrows - glyphnrows - 1;
			_ncols = ncols - glyphncols - 1;

			//	
			if(useclahe)
				CLAHE(tmppixels, tmppixels, nrows, ncols, ldim, 8, 8, 3);

			transform_to_ascii(&tmppixels[r*ldim+c], &_nrows, &_ncols, ldim);

			display_image(&tmppixels[r*ldim+c], _nrows, _ncols, ldim);
		}
	}

	//
	free(tmppixels);
}

/*
	
*/

static CvCapture* videostream = 0;

int initialize_video_stream(char avifile[])
{
	if(avifile)
		videostream = cvCaptureFromAVI(avifile);
	else
		videostream = cvCaptureFromCAM(0);

	if(!videostream)
		return 0;

	return 1;
}

void uninitialize_video_stream()
{
	cvReleaseCapture(&videostream);
}

uint8_t* get_frame_from_video_stream(int* nrows, int* ncols, int* ldim)
{
	static IplImage* gray = 0;
	
	IplImage* frame;
	
	//
	if(!videostream)
		return 0;

	// get the frame
	cvGrabFrame(videostream);
	frame = cvRetrieveFrame(videostream, CV_LOAD_IMAGE_GRAYSCALE);
	
	if(!frame)
		return 0;
	
	// convert to grayscale
	if(!gray)
		gray = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, 1);

	cvCvtColor(frame, gray, CV_RGB2GRAY);

	//cvFlip(gray, gray, 1);
	
	// extract relevant data
	*nrows = gray->height;
	*ncols = gray->width;
	*ldim = gray->widthStep;
	
	return (uint8_t*)gray->imageData;
}

int process_video_stream(char avifile[])
{
	int stop;

	uint8_t* frame;
	int nrows, ncols, ldim;

	int useclahe;

	//
	if(!initialize_video_stream(avifile))
	{
		printf("Cannot initialize video stream!\n");

		return 0;
	}

	//
	useclahe = 0;

	stop = 0;

	while(!stop)
	{
		int key = 0;

		// get the pressed key ...
		key = cvWaitKey(1);

		//
		frame = get_frame_from_video_stream(&nrows, &ncols, &ldim);

		if(key == 't')
			useclahe = ~useclahe;

		//
		if(key=='q')
			stop = 1;
		else if(!frame)
			return 0;
		else
		{
			if(useclahe)
				CLAHE(frame, frame, nrows, ncols, ldim, 8, 8, 3);

			transform_to_ascii(frame, &nrows, &ncols, ldim);

			display_image(frame, nrows, ncols, ldim);
		}
	}

	//
	uninitialize_video_stream();

	//
	return 1;
}

/*
	
*/

int main(int argc, char* argv[])
{
	// packed rendering structures
	static uint8_t pack[]=
	{
		#include "8x8.array"
	};

	//
	unpack_rendering_structures(glyphs, &glyphnum, &glyphnrows, &glyphncols, &tree, pack);

	//
	if(argc == 1)
		process_video_stream(0);
	else if(argc == 2)
		view_in_ascii(argv[1]);
		//process_video_stream(argv[1]);
	else if(argc == 3)
		process_image(argv[1], argv[2]);
	else
		return 0;

	//
	return 0;
}