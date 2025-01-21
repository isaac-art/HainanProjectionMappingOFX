#pragma once
#include "BaseElement.h"

class ImageElement : public BaseElement {
public:
    ImageElement();
    virtual ~ImageElement() = default;
    
    // Draw function specific to ImageElement
    void draw(const vector<ofImage>& images, const vector<ofColor>& colorSwatches, 
             bool showGui = false, size_t tileIndex = 0) const;
             
    // Set the image region for this tile
    void setImageRegion(size_t index, const ofRectangle& region);
    
    // Image index in the images vector
    size_t imageIndex;
}; 