/*
	n3lar - n3n0's learned ASCII art renderer
	Copyright (c) 2013, Nenad Markus

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

	Contact the author by neno.markus@gmail.com.
*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <cv.h>
#include <highgui.h>

//
static CvCapture* videostream = NULL;

/*
	
*/

#define CHARH 8
#define CHARW 8

static uint8_t fontdata[256][64]=
#include "fontdata"
;

/*
	
*/

void do_gamma_correction(uint8_t* pixels, int nrows, int ncols, int ldim, float gamma)
{
	int r, c, ind;

	#define NLEVELS 4096
	#define MINGAMMA 0.0f
	#define MAXGAMMA 30.0f

	static int islutcomputed = 0;
	static uint8_t lut[NLEVELS][256];

	//
	if(!islutcomputed)
	{
		for(r=0; r<NLEVELS; ++r)
			for(c=0; c<256; ++c)
				lut[r][c] = (uint8_t)( 255.0f*pow(c/255.0f, r*(MAXGAMMA-MINGAMMA)/NLEVELS) );

		islutcomputed = 1;
	}
	
	//
	gamma = MIN(MAX(gamma, MINGAMMA), MAXGAMMA);

	ind = (int)( gamma/((MAXGAMMA-MINGAMMA)/NLEVELS) );

	//
	for(r=0; r<nrows; ++r)
		for(c=0; c<ncols; ++c)
			pixels[r*ldim+c] = lut[ind][pixels[r*ldim+c]];
}

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

#define BINTEST(r, c, t, pixels, ldim) ( (pixels)[(r)*(ldim)+(c)] > (t) )

int get_tree_output(int32_t* tree, uint8_t* pixels, int ldim)
{
	int nodeidx;
	uint8_t* n;
	
	//
	nodeidx = 0;
	n = (uint8_t*)&tree[0];
	
	while(n[0] == 1) // while we are at a nonterminal node
	{
		//
		if( 0==BINTEST(n[1], n[2], n[3], pixels, ldim) )
			nodeidx = 2*nodeidx+1;
		else
			nodeidx = 2*nodeidx+2;
		
		//
		n = (uint8_t*)&tree[nodeidx];
	}
	
	return n[1];
}

int convert_and_render(uint8_t* pixels, int nrows, int ncols, int ldim)
{
	int r, c;
	
	static uint8_t treearray[] = 
#include "tree.array"
	;

	//
	for(r=0; r<nrows; r+=CHARH)
		for(c=0; c<ncols; c+=CHARW)
		{
			int i, j, ind;
			uint8_t* glyph;
			
			//
			ind = get_tree_output((int32_t*)treearray, &pixels[r*ldim+c], ldim);
			
			//
			glyph = (uint8_t*)&fontdata[ind][0];
			
			for(i=0; i<CHARH; ++i)
				for(j=0; j<CHARW; ++j)
					pixels[(r+i)*ldim+(c+j)] = glyph[i*CHARW+j];
		}
}

/*
	
*/

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
	static IplImage* gray = NULL;
	
	IplImage* frame;
	
	//
	if(!videostream)
		return NULL;

	// get the frame
	cvGrabFrame(videostream);
	frame = cvRetrieveFrame(videostream, CV_LOAD_IMAGE_GRAYSCALE);
	
	if(!frame)
		return NULL;
	
	// convert to grayscale
	if(!gray)
		gray = cvCreateImage(cvSize(frame->width, frame->height), frame->depth, 1);

	cvCvtColor(frame, gray, CV_RGB2GRAY);

	cvFlip(gray, gray, 1);
	
	// extract relevant data
	*nrows = gray->height;
	*ncols = gray->width;
	*ldim = gray->widthStep;
	
	return (uint8_t*)gray->imageData;
}

/*

*/

void convert_to_ascii_art(uint8_t pixels[], int nrows, int ncols, int ldim)
{
	CLAHE(pixels, pixels, nrows, ncols, ldim, 8, 8, 3);

	do_gamma_correction(pixels, nrows, ncols, ldim, 2.0f);

	CLAHE(pixels, pixels, nrows, ncols, ldim, 8, 8, 3);

	convert_and_render(pixels, nrows, ncols, ldim);
}

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

int process_stream()
{
	int stop;

    uint8_t* frame;
	int nrows, ncols, ldim;

    //
	stop = 0;

	while(!stop)
	{
		int key = 0;

    	// get the pressed key ...
    	key = cvWaitKey(1);

    	//
    	frame = get_frame_from_video_stream(&nrows, &ncols, &ldim);

		//
		if(key=='q')
			stop = 1;
		else if(!frame)
			usleep(10000);
		else
		{
			convert_to_ascii_art(frame, nrows, ncols, ldim);

			display_image(frame, nrows, ncols, ldim);
		}
	}

	//
	return 1;
}

/*
	
*/

int main()
{
	if(!initialize_video_stream(NULL))
		return 1;

	process_stream();

	uninitialize_video_stream();

	return 0;
}

