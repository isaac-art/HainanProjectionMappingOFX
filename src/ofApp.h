#pragma once

#include "ofMain.h"
#include "VideoElement.h"
#include "ofxGui.h"
#include "ofJson.h"
#include "ofxOsc.h"
#include "ofxOpenCv.h"
#include "ImageElement.h"
#include "CameraElement.h"

enum class APlaybackMode {
    LOOP,           // Default looping playback
    OSC_PLAYBACK    // Controlled by OSC input
};

enum class AOscInputType {
    YAW,
    PITCH,
    ROLL
};

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	// Standard OF events
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	
	// Media elements
	vector<VideoElement> tiles;
	vector<ImageElement> imageTiles;
	vector<ofVideoPlayer> videos;
	vector<tuple<APlaybackMode, AOscInputType>> videoPlaybackSettings;
	vector<ofImage> images;
	
	// Media loading functions
	void loadVideoAsTiles(const string& path);
	void loadImageAsTiles(const string& path);
	void loadNewVideo();
	void loadNewImage();
	void changeSelectedVideo();
	void changeSelectedImage();
	
	// Selection and manipulation
	int selectedTile;
	float adjustmentSpeed;
	bool isDragging;
	ofPoint dragStartPos;
	ofPoint tileStartPos;
	vector<int> selectedTiles;
	bool isGroupSelected;
	
	void deleteTile(int index);
	int findTileUnderMouse(int x, int y);
	void selectTilesFromSameSource(int tileIndex);
	void moveSelectedTiles(float dx, float dy);
	
	// Undo system
	struct TileMove {
		int tileIndex;
		float oldX, oldY;
		float newX, newY;
	};
	
	struct MoveAction {
		vector<TileMove> moves;
		bool isGroup;
	};
	
	static const int MAX_UNDO_HISTORY = 20;
	deque<MoveAction> undoHistory;
	void recordTileMove(const vector<int>& tileIndices, bool isGroup);
	void undo();
	
	// GUI Elements
	ofxPanel gui;
	ofxPanel infoPanel;
	ofxPanel swatchPanel;
	
	// GUI Buttons
	ofxButton saveLayoutBtn;
	ofxButton saveChangesBtn;
	ofxButton loadLayoutBtn;
	ofxButton changeVideoBtn;
	ofxButton addVideoBtn;
	ofxButton addImageBtn;
	ofxToggle gradientToggle;
	ofxButton newLayoutBtn;
	
	// GUI Labels
	ofxLabel currentLayoutLabel;
	ofxLabel videoPathLabel;
	ofxLabel tilePosLabel;
	ofxLabel tileIndexLabel;
	ofxLabel tileSizeLabel;
	ofxLabel primaryVideoLabel;
	
	// GUI Parameters
	ofParameter<int> primaryVideoIndex{"Set Primary", -1, -1, 0};
	ofParameter<int> color1Index;
	ofParameter<int> color2Index;
	ofxToggle colorInputToggle;
	
	// GUI Functions
	void setupGui();
	void updateInfoPanel();
	void updatePrimaryVideoDropdown();
	void onPrimaryVideoChanged(int& index);
	void onColorInputToggled(bool& value);
	void onColor1Changed(int& index);
	void onColor2Changed(int& index);
	void onGradientToggled(bool& value);
	
	// Layout Management
	vector<string> layoutFiles;
	int selectedLayout;
	bool showGui;
	void saveLayout();
	void loadLayout();
	void saveCurrentLayout();
	void createNewLayout();
	string getLayoutPath(const string& name);
	void refreshLayoutList();
	void nextLayout();
	void previousLayout();
	string generateLayoutName();
	
	// OSC
	ofxOscReceiver oscReceiver;
	static const int OSC_PORT = 9000;
	void setupOsc();
	void updateOsc();

	float yawValue;
	float pitchValue;
	float rollValue;
	
	// Color Management
	static const int NUM_SWATCHES = 6;
	static const int PROCESS_WIDTH = 64;
	vector<ofColor> colorSwatches;
	ofxCvColorImage cvImage;
	bool isCvImageAllocated = false;
	bool needsSwatchUpdate = false;
	
	void updateColorSwatches();
	void drawColorSwatches();
	void updateColorSwatchesFromPrimary();
	float colorDistance(const ofColor& c1, const ofColor& c2);
	vector<string> getUniqueVideoPaths() const;
	
	// Add new member variables
	vector<CameraElement> cameraTiles;
	vector<ofVideoGrabber> cameras;
	
	// Add new function declarations
	void setupCamera();
	void addCameraTile();
	ofxButton addCameraBtn;
	

	float lastSwatchUpdate;
	// Video playback control
	void setVideoPlaybackMode(size_t videoIndex, APlaybackMode mode, AOscInputType oscType);
	
	// Video Preview Panel
	ofxPanel videoPreviewPanel;
	ofParameter<bool> isOscMode{"OSC Mode", false};
	ofParameter<int> oscInputType{"OSC Input", 0, 0, 2};  // 0=YAW, 1=PITCH, 2=ROLL
	ofRectangle previewRect;
	static const int PREVIEW_WIDTH = 320;
	static const int PREVIEW_HEIGHT = 240;
	bool showVideoPreview = false;
	
	void setupVideoPreviewPanel();
	void drawVideoPreview();
	void onOscModeChanged(bool& value);
	void onOscInputChanged(int& value);
	void updateVideoPreviewPanel();

private:
	bool isEditMode() const { return showGui; }
	
	// Helper function for loading tile data
	template<typename T>
	void loadTileData(T& tile, const ofJson& tileData, size_t newIndex);
	
	// Add this member variable
	vector<ofPoint> tileRelativePositions;  // Store relative positions for group dragging
	
	// Template helpers for loadTileData
	template<typename T>
	void setRegionForTile(T& tile, size_t index, const ofRectangle& region, std::true_type);
	
	template<typename T>
	void setRegionForTile(T& tile, size_t index, const ofRectangle& region, std::false_type);
	
	void setRegionForSpecificTile(VideoElement& tile, size_t index, const ofRectangle& region);
	void setRegionForSpecificTile(ImageElement& tile, size_t index, const ofRectangle& region);
	void setRegionForSpecificTile(CameraElement& tile, size_t index, const ofRectangle& region);
	
	// Add these constants
	static const int GRID_COLS = 12;
	static const int GRID_ROWS = 10;
	static const int GRID_SPACING = 24;  // 0.3 ratio of tile size (80px)
	
	// Add this helper function
	void alignTilesToGrid();
	

};
