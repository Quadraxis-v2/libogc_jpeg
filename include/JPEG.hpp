/**
	@file		JPEG.hpp
	@author		Juan de la Cruz Caravaca Guerrero (Quadraxis_v2)
	@date		22/06/2022
    @brief		JPEG class
    @par		Description
                Class for decoding and displaying JPEGs
				
*/

#ifndef JPEG_HPP_
#define JPEG_HPP_

#include <cstdint>
#include <ogc/gx_struct.h>

/**
 * @brief Class for decoding and displaying JPEGs
 */
class JPEG
{
public:
    /** Getters */
    int32_t GetWidth() const noexcept;
    int32_t GetHeight() const noexcept;
    int32_t GetPosX() const noexcept;
    int32_t GetPosY() const noexcept;
    uint32_t* GetImgBuf() const noexcept;


    /**
     * @brief Construct a new JPEG object from a file in the filesystem
     * @param CpacFilePath the path to the image in the filesystem
     */
    explicit JPEG(const char* CpacFilePath);


    /**
     * @brief Construct a new JPEG object from an image buffer
     * @param CpauyJpegBuf pointer to the buffer of the compressed image
     * @param lJpegSize the size of the image in bytes
     */
    explicit JPEG(const uint8_t* CpauyJpegBuf, uint64_t lJpegSize);
    

    JPEG(const JPEG& CjpegOther);       /**< Copy constructor */
    JPEG(JPEG&& jpegOther) noexcept;    /**< Movement constructor */
    ~JPEG() noexcept;                   /**< Destructor */


    JPEG& operator =(const JPEG& CjpegOther);        /**< Assign with copy operator */
    JPEG& operator =(JPEG&& jpegOther) noexcept;     /**< Assign with movement operator */


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
    void Display(void* paXfb, const GXRModeObj* CpGXRmode, float fOriginalWidth, float fOriginalHeight, 
        float fX, float fY);

private:
    int32_t _iWidth;        /**< Width of the image in pixels */
    int32_t _iHeight;       /**< Height of the image in pixels */
    int32_t _iPosX;         /**< Coordinate X for the top left corner of the displayed image on the canvas*/
    int32_t _iPosY;         /**< Coordinate Y for the top left corner of the displayed image on the canvas*/
    uint32_t* _pauiImgBuf;  /**< Image buffer in Y1CbY2Cr values */


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
    static uint32_t Rgb2Yuv(uint8_t uyR1, uint8_t uyG1, uint8_t uyB1, uint8_t uyR2, uint8_t uyG2, 
        uint8_t uyB2) noexcept;

};


inline int32_t JPEG::GetWidth() const noexcept { return _iWidth; }
inline int32_t JPEG::GetHeight() const noexcept { return _iHeight; }
inline int32_t JPEG::GetPosX() const noexcept { return _iPosX; }
inline int32_t JPEG::GetPosY() const noexcept { return _iPosY; }
inline uint32_t* JPEG::GetImgBuf() const noexcept { return _pauiImgBuf; }


#endif
