#include "PngProc.h"

extern "C"
{
#include <png.h>
}

#include <stdio.h>
#include <string.h>

using namespace NPngProc;

class CFileGuard
{
public:
	CFileGuard(FILE* pFile) : m_pFile(pFile) { }
	~CFileGuard()	{ fclose(m_pFile); }
protected:
	FILE* m_pFile;
};

class CPNGExeption
{
public:
	CPNGExeption(const char* szErrorMessage)
	{
		strcpy(m_szErrorMessage, szErrorMessage);
	}
public:
	char m_szErrorMessage[512];
};


static void PNGError(png_structp png_ptr, png_const_charp error_message)
{
	throw CPNGExeption(error_message);
}

size_t NPngProc::readPngFile(const char* szFileName
		   , unsigned char* pBuf
		   , size_t*	pWidth
		   , size_t*	pHeight
		   , unsigned int* pBPP
		   )
{
	try
	{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type;
	FILE *fp;

	if ((fp = fopen(szFileName, "rb")) == NULL)
		throw CPNGExeption("can't open png file");

	CFileGuard FileGuard(fp);


	/* Create and initialize the png_struct with the desired error handler
	* functions.  If you want to use the default stderr and longjump method,
	* you can supply NULL for the last three parameters.  We also supply the
	* the compiler header file version, so that we know if the application
	* was compiled with a compatible version of the library.  REQUIRED
	*/
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		0, PNGError, 0);

	if (png_ptr == NULL)
	{
		throw CPNGExeption("can't create png read struct");
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{		
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		throw CPNGExeption("can't create png info struct");
	}

	png_init_io(png_ptr, fp);

	/* If we have already read some of the signature */
	png_set_sig_bytes(png_ptr, 0);

	png_read_info(png_ptr, info_ptr);

	// 	png_uint_32 width, height;
	// 	int bit_depth, color_type;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		int_p_NULL, int_p_NULL, int_p_NULL);

	int pixel_depth; // always gray 

	// TODO: need try!!!
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GA:
	case PNG_COLOR_TYPE_GRAY:	pixel_depth = 8; break;
	case PNG_COLOR_TYPE_PALETTE:	
	case PNG_COLOR_TYPE_RGB: 
	case PNG_COLOR_TYPE_RGBA: pixel_depth = 24; break;
	default:
		throw CPNGExeption("undefined color_type");
	};

	if (pBuf == NULL)
	{		
		if (pWidth) *pWidth = width;
		if (pHeight) *pHeight = height;
		if (pBPP) *pBPP = pixel_depth;

		return width * height * (pixel_depth / 8);
	}



	/* Set up the data transformations you want.  Note that these are all
	* optional.  Only call them if you want/need them.  Many of the
	* transformations only work on specific types of images, and many
	* are mutually exclusive.
	*/

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Strip alpha bytes from the input data without combining with the
	* background (not recommended).
	*/
	png_set_strip_alpha(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	* byte into separate bytes (useful for paletted and grayscale images).
	*/
	png_set_packing(png_ptr);

	/* Expand paletted colors into true RGB triplets */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	/* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_gray_1_2_4_to_8(png_ptr);

	/* Turn on interlace handling.  REQUIRED if you are not using
	* png_read_image().  To see how to handle interlacing passes,
	* see the png_read_row() method below:
	*/
	int number_passes = png_set_interlace_handling(png_ptr);

	/* Allocate the memory to hold the image using the fields of info_ptr. */

	/* The easiest way to read the image: */

	unsigned long nRowSize = width * (pixel_depth / 8);

	for (int pass = 0; pass < number_passes; pass++)
	{	
		unsigned char* pOut = pBuf;
		for (unsigned int y = 0; y < height; y++)
		{
			png_read_rows(png_ptr, &pOut, png_bytepp_NULL, 1);
			pOut += nRowSize;
		}
	}

	/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* At this point you have read the entire image */

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);


	if (pWidth) *pWidth = width;
	if (pHeight) *pHeight = height;
	if (pBPP) *pBPP = pixel_depth;

	return nRowSize * height;
	}
	catch (CPNGExeption& e)
	{
		printf ("NPngProc:readPngFile: Error - %s", e.m_szErrorMessage);
		return NPngProc::PNG_ERROR;
	}

}

size_t NPngProc::readPngFileGray(const char* szFileName
			     , unsigned char* pBuf
			     , size_t*	pWidth
			     , size_t*	pHeight
			     )
{
	try
	{
		png_structp png_ptr;
		png_infop info_ptr;
		unsigned int sig_read = 0;
		png_uint_32 width, height;
		int bit_depth, color_type;
		FILE *fp;

		if ((fp = fopen(szFileName, "rb")) == NULL)
			throw CPNGExeption("can't open png file");

		CFileGuard FileGuard(fp);


		/* Create and initialize the png_struct with the desired error handler
		* functions.  If you want to use the default stderr and longjump method,
		* you can supply NULL for the last three parameters.  We also supply the
		* the compiler header file version, so that we know if the application
		* was compiled with a compatible version of the library.  REQUIRED
		*/
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			0, PNGError, 0);

		if (png_ptr == NULL)
		{
			throw CPNGExeption("can't create png read struct");
		}

		/* Allocate/initialize the memory for image information.  REQUIRED. */
		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL)
		{		
			png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
			throw CPNGExeption("can't create png info struct");
		}

		png_init_io(png_ptr, fp);

		/* If we have already read some of the signature */
		png_set_sig_bytes(png_ptr, 0);

		png_read_info(png_ptr, info_ptr);

		// 	png_uint_32 width, height;
		// 	int bit_depth, color_type;

		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
			int_p_NULL, int_p_NULL, int_p_NULL);

		int pixel_depth = 8;

		// TODO: need try!!!
		switch (color_type)
		{
		case PNG_COLOR_TYPE_GA:
		case PNG_COLOR_TYPE_GRAY:	pixel_depth = 8; break;
		case PNG_COLOR_TYPE_PALETTE:	
		case PNG_COLOR_TYPE_RGB: 
		case PNG_COLOR_TYPE_RGBA: pixel_depth = 24; break;
		default:
			throw CPNGExeption("undefined color_type");
		};

		if (pBuf == NULL)
		{		
			if (pWidth) *pWidth = width;
			if (pHeight) *pHeight = height;			

			return width * height;
		}



		/* Set up the data transformations you want.  Note that these are all
		* optional.  Only call them if you want/need them.  Many of the
		* transformations only work on specific types of images, and many
		* are mutually exclusive.
		*/

		/* tell libpng to strip 16 bit/color files down to 8 bits/color */
		png_set_strip_16(png_ptr);

		/* Strip alpha bytes from the input data without combining with the
		* background (not recommended).
		*/
		png_set_strip_alpha(png_ptr);

		/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
		* byte into separate bytes (useful for paletted and grayscale images).
		*/
		png_set_packing(png_ptr);

		/* Expand paletted colors into true RGB triplets */
		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png_ptr);

		/* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			png_set_gray_1_2_4_to_8(png_ptr);

		/* Turn on interlace handling.  REQUIRED if you are not using
		* png_read_image().  To see how to handle interlacing passes,
		* see the png_read_row() method below:
		*/
		int number_passes = png_set_interlace_handling(png_ptr);

		/* Allocate the memory to hold the image using the fields of info_ptr. */

		/* The easiest way to read the image: */

		unsigned int nRowSize = width;

		if (pixel_depth == 8)
		{
			for (int pass = 0; pass < number_passes; pass++)
			{	
				unsigned char* pOut = pBuf;
				for (unsigned int y = 0; y < height; y++)
				{
					png_read_rows(png_ptr, &pOut, png_bytepp_NULL, 1);

					pOut += nRowSize;
				}
			}
		}
		else
		{
			unsigned char* pTempBuf = new unsigned char[width * (pixel_depth / 8)];
			if (!pTempBuf)
				throw CPNGExeption("can't allocate memory for temp buf");

			for (int pass = 0; pass < number_passes; pass++)
			{	
				unsigned char* pOut = pBuf;
				for (unsigned int y = 0; y < height; y++)
				{
					png_read_rows(png_ptr, &pTempBuf, png_bytepp_NULL, 1);

					unsigned char* pIn = pTempBuf;
					for (unsigned int x = 0; x < width; ++x)
					{
						*pOut++ = ((77 * *pIn + 150 * *(pIn + 1) + 29 * *(pIn + 2)))/256;
						pIn += 3;
					}					
				}
			}	

			delete [] pTempBuf;
		}
		/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
		png_read_end(png_ptr, info_ptr);

		/* At this point you have read the entire image */

		/* clean up after the read, and free any memory allocated - REQUIRED */
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);


		if (pWidth) *pWidth = width;
		if (pHeight) *pHeight = height;
		
		return nRowSize * height;
	}
	catch (CPNGExeption& e)
	{
		printf ("NPngProc:readPngFile: Error - %s", e.m_szErrorMessage);
		return NPngProc::PNG_ERROR;
	}

}

size_t NPngProc::writePngFile(const char* szFileName
			      , unsigned char* pBuf
			      , size_t	nWidth
			      , size_t	nHeight
			      , unsigned int nBPP
			      )
{
	try
	{
		FILE *fp;
		png_structp png_ptr;
		png_infop info_ptr;
		//png_colorp palette;

		/* open the file */
		fp = fopen(szFileName, "wb");
		if (fp == NULL)
			throw CPNGExeption("can't create png file");
			
		CFileGuard FileGuard(fp);	

		/* Create and initialize the png_struct with the desired error handler
		* functions.  If you want to use the default stderr and longjump method,
		* you can supply NULL for the last three parameters.  We also check that
		* the library version is compatible with the one used at compile time,
		* in case we are using dynamically linked libraries.  REQUIRED.
		*/
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			 0, PNGError, 0);

		if (png_ptr == NULL)
		{
			throw CPNGExeption("can't create write png structure");
		}

		/* Allocate/initialize the image information data.  REQUIRED */
		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL)
		{			
			png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
			throw CPNGExeption("can't create info structure");
		}


		png_init_io(png_ptr, fp);

		/* This is the hard way */

		/* Set the image information here.  Width and height are up to 2^31,
		* bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
		* the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
		* PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
		* or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
		* PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
		* currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
		*/

		int nPngColorType = PNG_COLOR_TYPE_GRAY; 
		int bit_depth = 8;
		
		if (nBPP == 24)
		{
			nPngColorType = PNG_COLOR_TYPE_RGB;
		}
			

		png_set_IHDR(png_ptr, info_ptr, (png_uint_32)nWidth, (png_uint_32)nHeight, bit_depth, nPngColorType,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);


		png_color_8 sig_bit;
		if (nBPP == 8)
		{
			sig_bit.gray = 8;
			sig_bit.red = 0;
			sig_bit.green = 0;
			sig_bit.blue = 0;
			/* if the image has an alpha channel then */
			sig_bit.alpha = 0;

		}
		else
		{
			sig_bit.gray = 0;
			sig_bit.red = 8;
			sig_bit.green = 8;
			sig_bit.blue = 8;
			/* if the image has an alpha channel then */
			sig_bit.alpha = 0;
		}
		/* if we are dealing with a grayscale image then */
		

		png_set_sBIT(png_ptr, info_ptr, &sig_bit);


		/* Write the file header information.  REQUIRED */
		png_write_info(png_ptr, info_ptr);

		/* pack pixels into bytes */
		png_set_packing(png_ptr);

		

		size_t nRowWidth = nWidth * nBPP / 8;

		unsigned char* pIn = pBuf;
		for (unsigned int i = 0; i < nHeight; ++i)
		{
			png_write_row(png_ptr, pIn);
			pIn += nRowWidth;
		}
		


		/* It is REQUIRED to call this to finish writing the rest of the file */
		png_write_end(png_ptr, info_ptr);


		/* clean up after the write, and free any memory allocated */
		png_destroy_write_struct(&png_ptr, &info_ptr);

		/* close the file */

		/* that's it */
		return 0;
	}
	catch (CPNGExeption& e)
	{
		printf ("NPngProc:readPngFile: Error - %s", e.m_szErrorMessage);
		return NPngProc::PNG_ERROR;
	}
}