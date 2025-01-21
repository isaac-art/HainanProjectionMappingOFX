#pragma once
#include "BaseElement.h"
#include "ofxCv.h"
#include "opencv2/opencv.hpp"


class VideoElement : public BaseElement {
public:
    VideoElement();
    virtual ~VideoElement() = default;
    void update();
    void draw(const vector<ofVideoPlayer>& videos, const vector<ofColor>& colorSwatches, 
             bool isEditMode, int index);
    void setVideoRegion(size_t index, const ofRectangle& region);
    
    size_t videoIndex;
    bool isPlaying;
    
    static constexpr float POSITION_CHANGE_THRESHOLD = 0.01f;
    float lastPosition;
    
}; 