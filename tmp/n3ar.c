/*
	n3ar - n3n0's ASCII art renderer
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

// includes
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

//#define OMIT_OPENCV

#ifndef OMIT_OPENCV
#include <cv.h>
#include <highgui.h>
#endif

// defines
#define ABS(x) (((x)>=0)?(x):(-(x)))

#define PACKETS_PER_FRAME 8
#define MAX_PACKET_SIZE 512

// globals
static const int Q = 8;
static const char C[] = "WGCc;,. ";
static const int V[] = {245, 210, 175, 140, 105, 70, 35, 0};

#ifndef OMIT_OPENCV
static CvCapture* videostream = NULL;
#endif

static int socketid;
static struct sockaddr_in sockaddr;

/*
	
*/

int initialize_ncurses()
{
	initscr();
	timeout(1); // wait time at getch: directly determines frame rate
	raw(); // line buffering disabled
	
	return 1;
}

void uninitialize_ncurses()
{
	endwin();
}

/*
	
*/

int initialize_video_stream(char avifile[])
{
#ifndef OMIT_OPENCV
	if(avifile)
		videostream = cvCaptureFromAVI(avifile);
	else
		videostream = cvCaptureFromCAM(0);
	
	if(!videostream)
		return 0;
	
	return 1;
#else
	return 0;
#endif
}

void uninitialize_video_stream()
{
#ifndef OMIT_OPENCV
	cvReleaseCapture(&videostream);
#endif
}

/*
	
*/

int initialize_ip_communication(char ipaddress[], int port)
{
	//
	socketid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(socketid == -1)
		return 0;
	
	//	
	memset(&sockaddr, 0, sizeof(sockaddr));
	
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(ipaddress)
	{
		if(inet_aton(ipaddress, &sockaddr.sin_addr) == 0)
		{
			close(socketid);
		
			return 0;
		}
	}
	else
	{
		if(bind(socketid, (const struct sockaddr*)&sockaddr, sizeof(sockaddr))==-1)
			return 0;

		// set to non-blocking mode
		fcntl(socketid, F_SETFL, O_NONBLOCK);
	}
	
	return 1;
}

void uninitialize_ip_communication()
{
	close(socketid);
}

/*
	
*/

void set_bit(uint8_t stream[], int p, int v)
{
	if(v)
		stream[p/8] |= 1<<(p%8);
	else
		stream[p/8] &= ~(1<<(p%8));
}

int get_bit(uint8_t stream[], int p)
{
	return (stream[p/8]&(1<<(p%8)))?1:0;
}

void compress(uint8_t out[], int* n, uint8_t in[], int nrows, int ncols)
{
	int i, j, b;
	
	//
	((int32_t*)out)[0] = nrows;
	((int32_t*)out)[1] = ncols;
	
	//
	b = 2*sizeof(int32_t)*8;
	
	for(i=0; i<nrows; ++i)
		for(j=0; j<ncols; ++j)
		{
			switch(in[i*ncols + j])
			{
				case 0:
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					break;
				case 1:
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					break;
				case 2:
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 0); ++b;
					break;
				case 3:
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 0); ++b;
					break;
				case 4:
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 1); ++b;
					break;
				case 5:
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 1); ++b;
					break;
				case 6:
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 1); ++b;
					break;
				case 7:
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 1); ++b;
					set_bit(out, b, 1); ++b;
					break;
				default:
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					set_bit(out, b, 0); ++b;
					break;
			}
		}
	
	//
	*n = b/8 + 1;
}

void decompress(uint8_t out[], int* h, int* w, uint8_t data[])
{
	int i, b;
	
	//
	*h = ((int*)data)[0];
	*w = ((int*)data)[1];
	
	//
	b = 2*sizeof(int)*8;
	
	for(i=0; i<*h**w; ++i)
	{
		out[i] = 1*get_bit(data, b+0) + 2*get_bit(data, b+1) + 4*get_bit(data, b+2);
		
		b += 3;
	}
}

/*
	
*/
void decimate(uint8_t out[], uint8_t in[], int nrows, int ncols, int ldim, int kh, int kw)
{
	int i, j;
	
	//
	for(i=0; i<nrows/kh; ++i)
		for(j=0; j<ncols/kw; ++j)
		{
			int k, l, sum;
			
			sum = 0;
			
			for(k=i*kh; k<(i+1)*kh; ++k)
				for(l=j*kw; l<(j+1)*kw; ++l)
					sum = sum + in[k*ldim + l];
			
			out[i*(ncols/kw) + j] = sum/(kh*kw);
		}
}

void quantize(uint8_t out[], uint8_t in[], int nrows, int ncols, int ldim)
{
	//
	int i, j;
	
	//
	for(i=0; i<nrows; ++i)
		for(j=0; j<ncols; ++j)
		{
			int q, e, old;
			
			//
			old = in[i*ldim + j];
			
			//
			e = 1024;
			for(q=0; q<Q; ++q)
				if( ABS(old-V[q]) < ABS(e) )
				{
					e = old-V[q];
					out[i*ldim + j] = q;
				}
		}
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

//
void convert(uint8_t img[], int* nrows, int* ncols, int ldim)
{
	int h, w, chrh, chrw;

	//
	h = *nrows;
	w = *ncols;
		
	//
	CLAHE(img, img, h, w, ldim, 8, 8, 2);
	
	//
	chrh = 16;
	chrw = 8;
	
	decimate(img, img, h, w, ldim, chrh, chrw);
	
	h = h/chrh;
	w = w/chrw;
	
	//
	CLAHE(img, img, h, w, w, 4, 8, 3);
	
	// pixel intensity quantization
	quantize(img, img, h, w, w);
	
	//
	*nrows = h;
	*ncols = w;
}

//
int render(uint8_t img[], int nrows, int ncols, int ldim)
{
	int i, j;
	static char cimg[640*480];

	//
	for(i=0; i<nrows; ++i)
	{
		for(j=0; j<ncols; ++j)
			cimg[i*(ncols+1) + j] = C[ img[i*ldim+j] ];
			
		cimg[i*(ncols+1) + j] = '\n';
	}
	
	mvprintw(0, 0, "%s", cimg);
	
	refresh(); // print it to the real screen
	
	//
	return 1;
}

/*
	
*/

uint8_t* get_frame_from_video_stream(int* nrows, int* ncols, int* ldim)
{
#ifndef OMIT_OPENCV
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
#else
	return NULL;
#endif
}

uint8_t* get_frame_over_ip(int* nrows, int* ncols, int* ldim)
{
	int i, n, stop;

	static uint8_t img[1024*1024];
	static uint8_t data[PACKETS_PER_FRAME*MAX_PACKET_SIZE];

	int flags[PACKETS_PER_FRAME], lengths[PACKETS_PER_FRAME];
	static uint8_t parts[PACKETS_PER_FRAME][MAX_PACKET_SIZE];

	//
	static int current_frame = 0;

	//
	memset(flags, 0, PACKETS_PER_FRAME*sizeof(int));
	
	stop = 0;

	while(!stop)
	{
		int id;
		static int8_t packet[MAX_PACKET_SIZE];
		
		//
		n = recv(socketid, packet, MAX_PACKET_SIZE, 0);
		
		if(n == -1)
			return NULL;
		
		//
		id = *(int32_t*)&packet[0];
		
		if(n>0 && id/PACKETS_PER_FRAME >= current_frame)
		{
			if(id/PACKETS_PER_FRAME > current_frame)
			{
				current_frame = id/PACKETS_PER_FRAME;
				
				memset(flags, 0, PACKETS_PER_FRAME*sizeof(int));
			}
		
			flags[id%PACKETS_PER_FRAME] = 1;
			
			//
			memcpy(&parts[id%PACKETS_PER_FRAME][0], &packet[sizeof(int32_t)], n-sizeof(int32_t));
			lengths[id%PACKETS_PER_FRAME] = n-sizeof(int32_t);
		
			// do we have all the packets we need to render the frame?
			stop = 1;
			
			for(i=0; i<PACKETS_PER_FRAME; ++i)
				if(!flags[i])
					stop = 0;
		}
	}
	
	//
	n = 0;
	for(i=0; i<PACKETS_PER_FRAME; ++i)
	{
		memcpy(&data[n], &parts[i][0], lengths[i]);
		
		n += lengths[i];
	}

	//
	decompress(img, nrows, ncols, data);
	
	*ldim = *ncols;
	
	//
	return img;
}

int convert_and_render(uint8_t frame[], int nrows, int ncols, int ldim)
{
	//
	convert(frame, &nrows, &ncols, ldim);
	
	//
	render(frame, nrows, ncols, ncols);

	//
	return 1;
}

int send_frame_over_ip(uint8_t frame[], int nrows, int ncols, int ldim)
{
	static int id = 0;
	static uint8_t buffer[PACKETS_PER_FRAME*MAX_PACKET_SIZE];
	
	uint8_t* ptr;
	int p, size;
	
	//
	convert(frame, &nrows, &ncols, ldim);
	
	compress(buffer, &size, frame, nrows, ncols);
	
	//
	ptr = &buffer[0];
	
	for(p=0; p<PACKETS_PER_FRAME; ++p)
	{
		int n;
		uint8_t packet[MAX_PACKET_SIZE];
		
		//
		*(int32_t*)&packet[0] = id;
		
		//
		n = size/(PACKETS_PER_FRAME-p);
		
		memcpy(&packet[sizeof(int32_t)], ptr, n);
		
		size -= n;
		ptr = &ptr[n];
		
		//
		if(sendto(socketid, packet, sizeof(int32_t)+n, 0, (const struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
			return 0;
		
		//
		++id;
	}
	
	return 1;
}

/*
	
*/

int process_stream(uint8_t* (get_frame)(int*, int*, int*), int (*process_frame)(uint8_t*, int, int, int))
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
    	key = getch();

    	//
    	frame = get_frame(&nrows, &ncols, &ldim);

		//
		if(key=='q')
			stop = 1;
		else if(!frame)
			usleep(10000);
		else
			process_frame(frame, nrows, ncols, ldim);
	}

	//
	return 1;
}

/*
	
*/

int main(int argc, char* argv[])
{
	int port, ipaddress;
	
	if(argc == 1)
	{
		//
		if(!initialize_video_stream(NULL) || !initialize_ncurses())
			return 1;
	
		//
		process_stream(get_frame_from_video_stream, convert_and_render);
	
		//
		uninitialize_video_stream();
		uninitialize_ncurses();
	}
	else if(argc==2)
	{	
		//
		sscanf(argv[1], "%d", &port);
		
		//
		if(!initialize_ip_communication(NULL, port) || !initialize_ncurses())
			return 1;
	
		//
		process_stream(get_frame_over_ip, render);
	
		//
		uninitialize_ncurses();
		uninitialize_ip_communication();
	}
	else if(argc == 3)
	{
		//
		sscanf(argv[2], "%d", &port);
		
		//
		if(!initialize_ip_communication(argv[1], port) || !initialize_video_stream(NULL) || !initialize_ncurses())
			return 1;
	
		//
		process_stream(get_frame_from_video_stream, send_frame_over_ip);
	
		//
		uninitialize_video_stream();
		uninitialize_ncurses();
		uninitialize_ip_communication();
	}
	else
		return 1;

    //
    return 0;
}
