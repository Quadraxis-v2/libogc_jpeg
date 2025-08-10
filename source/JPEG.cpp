/**
	@file		JPEG.cpp
	@author		Juan de la Cruz Caravaca Guerrero (Quadraxis_v2)
	@date		30/06/2022
    @brief		JPEG class
    @par		Description
                Implementation for the JPEG class
				
*/

#include <limits>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <ios>
#include <new>
#include <cstdlib>

#include <turbojpeg.h>

#include <gctypes.h>
#include <ogc/gx_struct.h>

#include "../include/JPEG.hpp"


/**
 * @brief Construct a new JPEG object from a file in the filesystem
 * 
 * @param CpacFilePath the path to the image in the filesystem
 */
JPEG::JPEG(const char* CpacFilePath) : _iWidth{}, _iHeight{}, _iPosX{std::numeric_limits<int32_t>::min()}, 
	_iPosY{std::numeric_limits<int32_t>::min()}, _pauiImgBuf{nullptr}
{
	uint8_t* pauyJpegBuf{nullptr};	// In-memory buffer for the compressed image
	uint64_t lJpegSize{};			// Size of the image in bytes

	/* Read the JPEG file into memory. */
	std::ifstream fileJpeg{CpacFilePath, std::ios_base::in | std::ios_base::binary | std::ios_base::ate};
	lJpegSize = fileJpeg.tellg();
	if (lJpegSize == 0) throw std::ios_base::failure("Input file contains no data");
	fileJpeg.seekg(0, std::ios_base::beg);

	if ((pauyJpegBuf = tjAlloc(lJpegSize)) == nullptr) throw std::bad_alloc();
	fileJpeg.read(reinterpret_cast<char*>(pauyJpegBuf), lJpegSize);

    fileJpeg.close();

	// Send the data to the other constructor
	try { *this = JPEG(pauyJpegBuf, lJpegSize); }
	catch (...) 
	{
		tjFree(pauyJpegBuf);  
		pauyJpegBuf = nullptr;
		throw;
	}

	// Free the temporary buffer
	tjFree(pauyJpegBuf);  
	pauyJpegBuf = nullptr;
}


/**
 * @brief Construct a new JPEG object from an image buffer
 * 
 * @param CpauyJpegBuf pointer to the buffer of the compressed image
 * @param lJpegSize the size of the image in bytes
 */
JPEG::JPEG(const uint8_t* CpauyJpegBuf, uint64_t lJpegSize) : _iWidth{}, _iHeight{}, 
	_iPosX{std::numeric_limits<int32_t>::min()}, _iPosY{std::numeric_limits<int32_t>::min()}, 
	_pauiImgBuf{nullptr}
{
	tjhandle tjHandle{};		// Handle instance for the decompression
	uint8_t* pauyRgbBuf{nullptr};		// Buffer for the decompressed image
	int32_t iInSubsamp{};			// Subsample value of the input image
	int32_t iInColorspace{};		// Colorspace of the input image

	if ((tjHandle = tjInitDecompress()) == nullptr) throw std::runtime_error(tjGetErrorStr());

	if (tjDecompressHeader3(tjHandle, CpauyJpegBuf, lJpegSize, &_iWidth, &_iHeight, &iInSubsamp, 
		&iInColorspace) == -1) throw std::runtime_error(tjGetErrorStr());

	if ((pauyRgbBuf = tjAlloc(_iWidth * _iHeight * tjPixelSize[TJPF::TJPF_RGB])) == nullptr)
		throw std::bad_alloc();

	if (tjDecompress2(tjHandle, CpauyJpegBuf, lJpegSize, pauyRgbBuf, _iWidth, 0, _iHeight, 
		TJPF::TJPF_RGB, 0) == -1) 
	{
		tjFree(pauyRgbBuf);
		pauyRgbBuf = nullptr;
		throw std::runtime_error(tjGetErrorStr());
	}

	_pauiImgBuf = new uint32_t[_iHeight * (_iWidth >> 1)];	// Allocate space for the converted Y1CbY2Cr image

	/* Translate the RGB values to Y1CbY2Cr and store them inside _apuiImgBuf */
	uint32_t uiRow{}, uiColumn{};
	for (s16 i = 0; i < _iHeight; i++)	// For every scanline
	{
		uiRow = i * _iWidth * tjPixelSize[TJPF_RGB];	// Offset to the i'th row. Rows have a number of color components in every pixel
		for (s16 j = 0; j < (_iWidth >> 1); j++)	// Every 2 pixels in the XFB share the same color values
		{
			uiColumn = j * (tjPixelSize[TJPF_RGB] << 1);	// We leap through every 2 columns since we process 2 pixels every iteration
			/* Translate values and store them in the right offset inside the XFB */
			_pauiImgBuf[i * (_iWidth >> 1) + j] = 
				JPEG::Rgb2Yuv(pauyRgbBuf[uiRow + uiColumn], pauyRgbBuf[uiRow + uiColumn + 1],
					pauyRgbBuf[uiRow + uiColumn + 2], pauyRgbBuf[uiRow + uiColumn + 3], 
					pauyRgbBuf[uiRow + uiColumn + 4], pauyRgbBuf[uiRow + uiColumn + 5]);
		}
	}

	// Free resources
	tjFree(pauyRgbBuf);
	pauyRgbBuf = nullptr;
    tjDestroy(tjHandle);
	tjHandle = nullptr;
}


/**
 * @brief Copy constructor
 * 
 * @param CjpegOther the JPEG::JPEG object to be copied
 */
JPEG::JPEG(const JPEG& CjpegOther) : _iWidth{CjpegOther._iWidth}, _iHeight{CjpegOther._iHeight},
	_iPosX{CjpegOther._iPosX}, _iPosY{CjpegOther._iPosY}, _pauiImgBuf{new uint32_t[(_iWidth >> 1) * _iHeight]}
{ std::memcpy(_pauiImgBuf, CjpegOther._pauiImgBuf, (_iWidth << 1) * _iHeight); }


/**
 * @brief Movement constructor
 * 
 * @param jpegOther the JPEG::JPEG object to be moved
 */
JPEG::JPEG(JPEG&& jpegOther) noexcept : _iWidth{jpegOther._iWidth}, _iHeight{jpegOther._iHeight},
	_iPosX{jpegOther._iPosX}, _iPosY{jpegOther._iPosY}, _pauiImgBuf{jpegOther._pauiImgBuf}
{
	jpegOther._iWidth = 0;
	jpegOther._iHeight = 0;
	jpegOther._iPosX = 0;
	jpegOther._iPosY = 0;
	jpegOther._pauiImgBuf = nullptr;
}


/**
 * @brief Assign with copy operator
 * 
 * @param CjpegOther the JPEG::JPEG object to be assigned
 * @return JPEG& a copy of the JPEG::JPEG object
 */
JPEG& JPEG::operator =(const JPEG& CjpegOther)
{
	if (this != &CjpegOther)
	{
		delete[] _pauiImgBuf;

		_iWidth = CjpegOther._iWidth;
		_iHeight = CjpegOther._iHeight;
		_iPosX = CjpegOther._iPosX;
		_iPosY = CjpegOther._iPosY;
		_pauiImgBuf = new uint32_t[(_iWidth >> 1) * _iHeight];

		std::memcpy(_pauiImgBuf, CjpegOther._pauiImgBuf, (_iWidth << 1) * _iHeight);
	}

	return *this;
}


/**
 * @brief Assignment with movement operator
 * 
 * @param jpegOther the JPEG::JPEG object to be assigned
 * @return JPEG& the moved JPEG::JPEG object
 */
JPEG& JPEG::operator =(JPEG&& jpegOther) noexcept
{
	if (this != &jpegOther)
	{
		delete[] _pauiImgBuf;

		_iWidth = jpegOther._iWidth;
		_iHeight = jpegOther._iHeight;
		_iPosX = jpegOther._iPosX;
		_iPosY = jpegOther._iPosY;
		_pauiImgBuf = jpegOther._pauiImgBuf;
		
		jpegOther._iWidth = 0;
		jpegOther._iHeight = 0;
		jpegOther._iPosX = 0;
		jpegOther._iPosY = 0;
		jpegOther._pauiImgBuf = nullptr;
	}

	return *this;
}


/**
 * @brief Destroy the JPEG::JPEG object
 */
JPEG::~JPEG() noexcept
{
	delete[] _pauiImgBuf;
	_pauiImgBuf = nullptr;
}


/**
 * @brief Displays the JPEG on the XFB. Width or height of the image can be out
 * of the XFB depending on the given coordinates. In those cases, the image will
 * be partially displayed
 * 
 * @param paXfb a pointer to the start of the XFB region
 * @param CpGXRmode a rendermode object holding the rendering parameters
 * @param fOriginalWidth the width of the canvas that is being drawn. If unsure, set this to the framebuffer's width
 * @param fOriginalHeight the height of the canvas that is being drawn. If unsure, set this to the framebuffer's height
 * @param fX the coordinate X of the top left corner of the image on the canvas
 * @param fY the coordinate Y of the top left corner of the image on the canvas
 */
void JPEG::Display(void* paXfb, const GXRModeObj* CpGXRmode, float fOriginalWidth, float fOriginalHeight, 
    float fX, float fY)
{
	if (fOriginalWidth <= 0 || fOriginalHeight <= 0) throw std::domain_error("Invalid dimensions");

	// Store the given positions
	_iPosX = fX;
	_iPosY = fY;

	// Rule of thumb to translate coordinates to the XFB
	int32_t iX = static_cast<int32_t>(fX * (CpGXRmode->fbWidth >> 1) / fOriginalWidth);
	int32_t iY = static_cast<int32_t>(fY * CpGXRmode->xfbHeight / fOriginalHeight);

	if (iX < CpGXRmode->fbWidth)
	{
		uint32_t* pauiFrameBuffer{static_cast<uint32_t*>(paXfb)};

		/* Copy whatever part of the image buffer is needed to the XFB */
		uint16_t i = (iY >= 0) ? 0 : -iY;
		while (i < _iHeight && iY + i < CpGXRmode->xfbHeight)	// For every scanline that is needed
		{
			if (iX < 0)	// If the X coordinate is to the left of the XFB copy the relevant portion into it
				std::memcpy(pauiFrameBuffer + (iY + i) * (CpGXRmode->fbWidth >> 1), 
					_pauiImgBuf + i * (_iWidth >> 1) - iX, 
					std::min(((_iWidth >> 1) + iX) << 2, CpGXRmode->fbWidth << 1));
			else		// If the X coordinate is inside the XFB copy the relevant portion into it
				std::memcpy(pauiFrameBuffer + (iY + i) * (CpGXRmode->fbWidth >> 1) + iX, 
					_pauiImgBuf + i * (_iWidth >> 1), 
					std::min(_iWidth << 1, ((CpGXRmode->fbWidth >> 1) - iX) << 2));

			i++;
		}
	}
}


/**
 * @brief Convert two RGB pixels to one Y1CbY2Cr
 * 
 * @param uyR1 the red component of the first pixel
 * @param uyG1 the green component of the first pixel
 * @param uyB1 the blue component of the first pixel
 * @param uyR2 the red component of the second pixel
 * @param uyG2 the green component of the second pixel
 * @param uyB2 the blue component of the second pixel
 * @return uint32_t the converted Y1CbY2Cr pixel value
 */
uint32_t JPEG::Rgb2Yuv (uint8_t uyR1, uint8_t uyG1, uint8_t uyB1, uint8_t uyR2, uint8_t uyG2, uint8_t uyB2) noexcept
{
	int32_t iY1{}, iCb1{}, iCr1{}, iY2{}, iCb2{}, iCr2{}, iCb{}, iCr{};

	iY1 = (299 * uyR1 + 587 * uyG1 + 114 * uyB1) / 1000;
	iCb1 = (-16874 * uyR1 - 33126 * uyG1 + 50000 * uyB1 + 12800000) / 100000;
	iCr1 = (50000 * uyR1 - 41869 * uyG1 - 8131 * uyB1 + 12800000) / 100000;
	iY2 = (299 * uyR2 + 587 * uyG2 + 114 * uyB2) / 1000;
	iCb2 = (-16874 * uyR2 - 33126 * uyG2 + 50000 * uyB2 + 12800000) / 100000;
	iCr2 = (50000 * uyR2 - 41869 * uyG2 - 8131 * uyB2 + 12800000) / 100000;

	iCb = (iCb1 + iCb2) >> 1;
	iCr = (iCr1 + iCr2) >> 1;
	return (iY1 << 24) | (iCb << 16) | (iY2 << 8) | iCr;
}
