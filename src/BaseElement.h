#pragma once
#include "ofMain.h"
#include "ofxOpenCv.h"

class BaseElement {
public:
    static const int TILE_SIZE = 80;
    
    // Add virtual destructor
    virtual ~BaseElement() = default;
    
    // Common properties
    float x, y;
    float offsetX, offsetY;
    ofRectangle sourceRegion;
    bool isLoaded;
    
    // Static texture shared by all instances
    static ofTexture gradientTexture;
    static bool showGradient;
    static void loadGradientTexture();
    static void toggleGradient() { showGradient = !showGradient; }
    
    // Common methods
    virtual void setup(float x, float y) {
        this->x = x;
        this->y = y;
        offsetX = offsetY = 0;
        isLoaded = false;
    }
    
    virtual void adjustPosition(float dx, float dy) {
        offsetX += dx;
        offsetY += dy;
    }
    
    // Primary status
    bool isPrimary() const { return isPrimaryElement; }
    void setPrimary(bool primary) { isPrimaryElement = primary; }
    
    // Color input mode
    bool hasColorInput() const { return useColorInput; }
    void setColorInput(bool enabled) { useColorInput = enabled; }
    void setColorIndices(int color1, int color2) { 
        colorIndex1 = color1; 
        colorIndex2 = color2; 
    }
    int getColorIndex1() const { return colorIndex1; }
    int getColorIndex2() const { return colorIndex2; }
    
    // Path handling
    void setPath(const string& p) { path = p; }
    string getPath() const { return path; }
    
protected:
    bool isPrimaryElement = false;
    bool useColorInput = false;
    int colorIndex1 = 0;
    int colorIndex2 = 1;
    string path;
    
    // OpenCV image processing
    mutable ofxCvColorImage cvImage;
    mutable ofxCvGrayscaleImage grayImage;
    mutable bool isCvImageAllocated = false;
}; 