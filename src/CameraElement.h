#pragma once
#include "BaseElement.h"

class CameraElement : public BaseElement {
public:
    CameraElement();
    virtual ~CameraElement() = default;
    
    void update();
    void draw(const vector<ofVideoGrabber>& cameras, const vector<ofColor>& colorSwatches, 
             bool isEditMode, int index);
             
    void setCameraRegion(size_t index, const ofRectangle& region);
    
    // Camera index in the cameras vector
    size_t cameraIndex;
    bool isActive;
    
private:
    // Helper function for color processing
    void processColors(const ofPixels& pixels, const vector<ofColor>& colorSwatches) const;
}; 