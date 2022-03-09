/********************************************************************
* tgx library example : displaying a 3D mesh...
*
*                        EXAMPLE FOR ESP32 
*                        
* Uses Bodmer's TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
* 
* Tested with an ESP32 dev module and an ILI9341 screen
********************************************************************/

// screen driver library
#include <TFT_eSPI.h>

// graphic library
#include <tgx.h>

// the mesh to draw
#include "naruto.h"

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// the screen driver
TFT_eSPI tft = TFT_eSPI();

// size of the drawing framebuffer
// (limited by the amount of memory in the ESP32). 
#define SLX 140
#define SLY 200

// the framebuffer we draw onto
uint16_t fb[SLX*SLY];

// second framebuffer used by eSPI_TFT for DMA update
// allocated via malloc
uint16_t * fb2;

// the z-buffer
float* zbuf;

// the image that encapsulate framebuffer fb
Image<RGB565> imfb(fb,SLX,SLY);

// the 3D mesh drawer (with zbuffer and perspective projection)
Renderer3D<RGB565, SLX, SLY, true, false> renderer;



// the setup function runs once when you press reset or power the board
void setup() 
    {
    Serial.begin(115200);

    // allocate the second framebuffer
    fb2 = (uint16_t*)malloc(SLX * SLY * sizeof(uint16_t));
    while (fb2 == nullptr)
        {
        Serial.println("Error: cannot allocate memory for fb2");
        delay(1000);
        }

    // allocate the zbuffer
    zbuf = (float*)malloc(SLX * SLY * sizeof(float));
    while (zbuf == nullptr)
        {
        Serial.println("Error: cannot allocate memory for zbuf");
        delay(1000);
        }

    // initialize the screen driver
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_CYAN);
    tft.initDMA();
    tft.startWrite();

    // setup the 3D renderer.
    renderer.setImage(&imfb); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf, SLX * SLY); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 0.1f, 1000.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
    renderer.setOffset(0, 0);
    }





/** Compute the model matrix according to the current time */  
tgx::fMat4 moveModel(int & loopnumber) 
    {    
    const float end1 = 6000;
    const float end2 = 2000;
    const float end3 = 6000;
    const float end4 = 2000;

    int tot = (int)(end1 + end2 + end3 + end4);
    int m = millis();

    loopnumber = m / tot; 
    float t = m % tot;

    const float dilat = 9; // scale model
    const float roty = 360 * (t / 4000); // rotate 1 turn every 4 seconds        
    float tz, ty;
    if (t < end1)
        { // far away
        tz = -25;
        ty = 0;
        }
    else
        {
        t -= end1;
        if (t < end2)
            { // zooming in
            t /= end2;
            tz = -25 + 18 * t;
            ty = -6.5f * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                { // close up
                tz = -7;
                ty = -6.5f;
                }
            else
                { // zooming out
                t -= end3;
                t /= end4;
                tz = -7 - 18 * t;
                ty = -6.5 + 6.5 * t;
                }
            }
        }

    fMat4 M;
    M.setScale({ dilat, dilat, dilat }); // scale the model
    M.multRotate(-roty, { 0,1,0 }); // rotate around y
    M.multTranslate({ 0,ty, tz }); // translate
    return M;
    }





/** Main loop */
void loop() 
    {  
    // compute the model position
    int loopnumber; 
    fMat4  M = moveModel(loopnumber);
    renderer.setModelMatrix(M);
    
    // choose the shader to use
    int shader = TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE; // default choice
    switch (loopnumber % 3) 
        {
        case 1: shader = TGX_SHADER_GOURAUD; break;
        case 2: shader = TGX_SHADER_FLAT; break;
        }

    // draw the 3D mesh
    imfb.fillScreen(RGB565_Black);              // clear the framebuffer (black background)
    renderer.clearZbuffer();                    // clear the z-buffer
    renderer.drawMesh(shader, &naruto_1, false);// draw the mesh !

    // upload the framebuffer to the screen (async. via DMA)
    tft.dmaWait();
    tft.pushImageDMA((tft.width() - SLX)/2, (tft.height()-SLY)/2, SLX, SLY, fb, fb2);
    }



/** end of file */