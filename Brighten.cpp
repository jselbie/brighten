// Brighten.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <iostream>
#include <vector>

HRESULT GetGdiplusEncoderClsid(const std::wstring& format, GUID* pGuid)
{
    HRESULT hr = S_OK;
    UINT  nEncoders = 0;          // number of image encoders
    UINT  nSize = 0;              // size of the image encoder array in bytes
    std::vector<BYTE> spData;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
    Gdiplus::Status status;
    bool found = false;

    if (format.empty() || !pGuid)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pGuid = GUID_NULL;
        status = Gdiplus::GetImageEncodersSize(&nEncoders, &nSize);

        if ((status != Gdiplus::Ok) || (nSize == 0))
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {

        spData.resize(nSize);
        pImageCodecInfo = (Gdiplus::ImageCodecInfo*) & spData.front();
        status = Gdiplus::GetImageEncoders(nEncoders, nSize, pImageCodecInfo);

        if (status != Gdiplus::Ok)
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        for (UINT j = 0; j < nEncoders && !found; j++)
        {
            if (pImageCodecInfo[j].MimeType == format)
            {
                *pGuid = pImageCodecInfo[j].Clsid;
                found = true;
            }
        }

        hr = found ? S_OK : E_FAIL;
    }

    return hr;
}

#define CLAMP(val) {val = (val > 255) ? 255 : ((val < 0) ? 0 : val);}

void Brighten(Gdiplus::BitmapData& dataIn, Gdiplus::BitmapData& dataOut, const double YMultiplier=1.3)
{
    if ( ((dataIn.PixelFormat != PixelFormat24bppRGB) && (dataIn.PixelFormat != PixelFormat32bppARGB)) ||
         ((dataOut.PixelFormat != PixelFormat24bppRGB) && (dataOut.PixelFormat != PixelFormat32bppARGB)))
    {
        return;
    }

    if ((dataIn.Width != dataOut.Width) || (dataIn.Height != dataOut.Height))
    {
        // images sizes aren't the same
        return;
    }


    const size_t incrementIn = dataIn.PixelFormat == PixelFormat24bppRGB ? 3 : 4;
    const size_t incrementOut = dataIn.PixelFormat == PixelFormat24bppRGB ? 3 : 4;
    const size_t width = dataIn.Width;
    const size_t height = dataIn.Height;


    for (size_t y = 0; y < height; y++)
    {
        auto ptrRowIn = (BYTE*)(dataIn.Scan0) + (y * dataIn.Stride);
        auto ptrRowOut = (BYTE*)(dataOut.Scan0) + (y * dataOut.Stride);;

        for (size_t x = 0; x < width; x++)
        {
            uint8_t B = ptrRowIn[0];
            uint8_t G = ptrRowIn[1];
            uint8_t R = ptrRowIn[2];
            uint8_t A = (incrementIn == 3) ? 0xFF : ptrRowIn[3];

            auto Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16;
            auto V = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128;
            auto U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128;

            Y *= YMultiplier;

            const auto YMinus16Times1164 = 1.164 * (Y - 16);
            const auto UMinus128 = U - 128;
            const auto VMinus128 = V - 128;
            auto newB = YMinus16Times1164 + 2.018*UMinus128;
            auto newG = YMinus16Times1164 - 0.813*VMinus128 - 0.391*UMinus128;
            auto newR = YMinus16Times1164 + 1.596*VMinus128;

            CLAMP(newR);
            CLAMP(newG);
            CLAMP(newB);

            ptrRowOut[0] = newB;
            ptrRowOut[1] = newG;
            ptrRowOut[2] = newR;
            if (incrementOut == 4)
            {
                ptrRowOut[3] = A; // keep original alpha
            }

            ptrRowIn += incrementIn;
            ptrRowOut += incrementOut;
        }
    }
}

void ProcessFile(Gdiplus::Bitmap* bmpIn, Gdiplus::Bitmap* bmpOut)
{
    Gdiplus::BitmapData dataIn = {};
    Gdiplus::BitmapData dataOut = {};
    Gdiplus::Rect rectIn(0, 0, bmpIn->GetWidth(), bmpIn->GetHeight());
    Gdiplus::Rect rectOut(0, 0, bmpOut->GetWidth(), bmpOut->GetHeight());

    bmpIn->LockBits(&rectIn, Gdiplus::ImageLockModeRead, bmpIn->GetPixelFormat(), &dataIn);
    bmpOut->LockBits(&rectOut, Gdiplus::ImageLockModeWrite, bmpOut->GetPixelFormat(), &dataOut);
    Brighten(dataIn, dataOut, .5);
}

Gdiplus::Bitmap* LoadFile(const std::wstring& filename)
{
    auto bmp = Gdiplus::Bitmap::FromFile(filename.c_str());
    return bmp;
}

Gdiplus::Bitmap* CloneFormat(Gdiplus::Bitmap* bmp)
{
    return new Gdiplus::Bitmap(bmp->GetWidth(), bmp->GetHeight(), bmp->GetPixelFormat());
}

void SaveFile(Gdiplus::Bitmap* bmp)
{
    GUID guidPng = {};
    GetGdiplusEncoderClsid(L"image/png", &guidPng);
    bmp->Save(L"C:/Users/jselbie/Pictures/updated.png", &guidPng);
}





int main()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    auto bmpIn = LoadFile(L"C:/Users/jselbie/Pictures/original.jpg");
    auto bmpOut = CloneFormat(bmpIn);
    ProcessFile(bmpIn, bmpOut);

    SaveFile(bmpOut);

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
