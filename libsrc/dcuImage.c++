#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <bstring.h>
#include "glImage.h"
#include "dcuImage.h"

static void reverseLongwords(unsigned long *img,int size);


dcuImage::dcuImage(char *filename,int byteOrder)
{
 byteOrder_ = byteOrder;
 image_ = NULL;
 filename_ = strdup("");
 xdim_ = ydim_ = 0;
 loadFile(filename);
}

void dcuImage::setByteOrder(int byteOrder)
{
 if ((byteOrder_ != byteOrder) && (image_))
	reverseLongwords(image_,xdim_*ydim_);
 byteOrder_ = byteOrder;
}

int dcuImage::loadFile(char *filename)
{
 if (image_)
	free(image_);
 image_ = NULL;
 if (filename_)
	free(filename_);
 filename_ = NULL;
 if (filename)
	{
	filename_ = strdup(filename);
	ReadSgiImageLong();
	if (!image_)
		{
		free(filename_);
		filename_ = NULL;
		}
	}
 if (!image_)
	return 0;
 return 1;
}

void dcuImage::setImage(unsigned long *image,int xdim,int ydim)
{
 if (image_)
	free(image_);
 if (filename_)
	free(filename_);
 image_ = image;
 xdim_ = xdim;
 ydim_ = ydim;
 filename_ = strdup("");
}

void dcuImage::copyImage(unsigned long *image,int xdim,int ydim)
{
 unsigned long *newimg;
 newimg = (unsigned long *) malloc(xdim*ydim*sizeof(unsigned long));
 if (!newimg)
	{
	fprintf(stderr,"ERROR: dcuImage::copyImage: failed to allocate "
		"space for image\n");
	return;
	}
 memcpy((void*)newimg,(void*)image,xdim*ydim*sizeof(unsigned long));
 if (image_)
	free((void *)image_);
 if (filename_)
	free((void *)filename_);
 image_ = newimg;
 xdim_ = xdim;
 ydim_ = ydim;
 filename_ = strdup("");
}

void dcuImage::createImage(int xdim,int ydim)
{
 unsigned long *newimg;
 newimg = (unsigned long *) malloc(xdim*ydim*sizeof(unsigned long));
 bzero((void *)newimg,xdim*ydim*sizeof(unsigned long));
 if (image_)
	free((void *)image_);
 image_ = newimg;
 xdim_ = xdim;
 ydim_ = ydim;
 if (filename_)
	free((void *)filename_);
 filename_ = strdup("");
}

int dcuImage::ReadSgiImageLong(void)
{
 register IMAGE *imagefp;
 register int x,y;
 unsigned short *rbuf,*gbuf,*bbuf,*abuf;
 unsigned long *p;
 int zdim;
 if ((imagefp=iopen(filename_,"r")) == NULL)
	{
	fprintf(stderr,"dcuImage::ReadSgiImageLong: can't open input file %s\n",
		filename_);
	return 0;
	}
 xdim_ = imagefp->xsize;
 ydim_ = imagefp->ysize;
 zdim = imagefp->zsize;
 rbuf = (unsigned short *) malloc(imagefp->xsize*sizeof(unsigned short));
 if (zdim>1) gbuf = (unsigned short *) malloc(imagefp->xsize*sizeof(unsigned short));
 else gbuf = rbuf;
 if (zdim>2) bbuf = (unsigned short *) malloc(imagefp->xsize*sizeof(unsigned short));
 else bbuf = gbuf;
 if (zdim>3) abuf = (unsigned short *) malloc(imagefp->xsize*sizeof(unsigned short));
 p = image_ = (unsigned long *) malloc(imagefp->xsize*imagefp->ysize*sizeof(unsigned long));
 if (!image_)
	{
	fprintf(stderr,"dcuImage::ReadSgiImageLong: failed to allocate memory\n");
	return 0;
	}
 for (y=0; y < imagefp->ysize; y++)
	{
	getrow(imagefp,rbuf,y,0);
	if (zdim > 1) getrow(imagefp,gbuf,y,1);
	if (zdim > 2) getrow(imagefp,bbuf,y,2);
	if (zdim > 3)
		{
		getrow(imagefp,abuf,y,3);
		if (byteOrder_ == DCU_ABGR)
			for (x=0; x<imagefp->xsize; x++)
				*p++ = rbuf[x] | (gbuf[x]<<8) | (bbuf[x]<<16) |
					 (abuf[x]<<24);
		else
			for (x=0; x<imagefp->xsize; x++)
				*p++ = abuf[x] | (bbuf[x]<<8) | (gbuf[x]<<16) |
					 (rbuf[x]<<24);
		}
	else
		{
		if (byteOrder_ == DCU_ABGR)
			for (x=0; x<imagefp->xsize; x++)
				*p++ = rbuf[x] | (gbuf[x]<<8) | (bbuf[x]<<16) |
					0xff000000;
		else
			for (x=0; x<imagefp->xsize; x++)
				*p++ = 0xff | (bbuf[x]<<8) | (gbuf[x]<<16) |
					 (rbuf[x]<<24);
		}
	}
 free(rbuf);
 if (zdim>1) free(gbuf);
 if (zdim>2) free(bbuf);
 if (zdim>3) free(abuf);
 iclose(imagefp);
 return 1;
}


void dcuImage::writeFile(char *fname)
{
 int i,y,width = xdim(),height = ydim();
 unsigned short *sbuf;
 unsigned long *data = image();
 IMAGE *image;
 unsigned long *p;
 if (!data)
	return;
 if (!(image = iopen(fname,"w",RLE(1),3,width,height,4)))
	{
	fprintf(stderr,"ERROR: dcuImage::writeFile: can't open output file %s\n",
		fname);
	return;
	}
 sbuf = (unsigned short *) malloc(width*2);
 if (byteOrder() == DCU_ABGR)
	{
	for (y=0; y < height; y++, data+=width)
		{
		for (i=0, p=data; i<width; i++)
			sbuf[i] = (*p++) & 0xff;
		putrow(image,sbuf,y,0);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff00) >> 8;
		putrow(image,sbuf,y,1);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff0000) >> 16;
		putrow(image,sbuf,y,2);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff000000) >> 24;
		putrow(image,sbuf,y,3);
		}
	}
 else
	{
	for (y=0; y < height; y++, data+=width)
		{
		for (i=0, p=data; i<width; i++)
			sbuf[i] = (*p++) & 0xff;
		putrow(image,sbuf,y,3);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff00) >> 8;
		putrow(image,sbuf,y,2);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff0000) >> 16;
		putrow(image,sbuf,y,1);
		for (i=0, p=data; i<width; i++)
			sbuf[i] = ((*p++) & 0xff000000) >> 24;
		putrow(image,sbuf,y,0);
		}
	}
 iclose(image);
 free(sbuf);
}


static void reverseLongwords(unsigned long *img,int size)
{
 int i;
 for (i=0; i<size; i++, img++)
	*img =  ((*img & 0x000000ff) << 24) |
		((*img & 0x0000ff00) << 8) |
		((*img & 0x00ff0000) >> 8) |
		((*img & 0xff000000) >> 24);
}


dcuImage * dcuImage::resizedImage(int newXdim,int newYdim)
{
 unsigned long *newimgdata,*p;
 int newX,newY,oldX,oldY;
 dcuImage *newImage;
 if (!image_)
	return NULL;
 newimgdata = (unsigned long *) malloc(newXdim*newYdim*sizeof(unsigned long));
 if (!newimgdata)
	{
	fprintf(stderr,"ERROR: dcuImage::resizedImage: failed to allocate "
		"space for image\n");
	return NULL;
	}
 for (newY=0, p=newimgdata; newY < newYdim; newY++)
	{
	oldY = (newY * (ydim_-1)) / (newYdim-1);
	for (newX=0; newX < newXdim; newX++)
		{
		oldX = (newX * (xdim_-1)) / (newXdim-1);
		*p++ = image_[oldX + oldY * xdim_];
		}
	}
 newImage = new dcuImage;
 newImage->setByteOrder(byteOrder_);
 newImage->setImage(newimgdata,newXdim,newYdim);
 return newImage;
}

