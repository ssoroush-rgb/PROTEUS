#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <set>                            
#include <fstream>
#include <sstream>
#include <ctime>
#include <windows.h>
#include <commdlg.h>
#include <memory>
#include <unordered_map>
#include <map>
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <utility>




using namespace std;

static bool appFileExists(const string& path) {
    ifstream file(path, ios::binary);
    return file.good();
}

static vector<string> applicationFontCandidates() {
    vector<string> candidates;

    char* basePath = SDL_GetBasePath();
    if (basePath) {
        string base(basePath);
        SDL_free(basePath);
        candidates.push_back(base + "assets\\fonts\\arial.ttf");
        candidates.push_back(base + "assets\\fonts\\segoeui.ttf");
    }

    candidates.push_back("assets\\fonts\\arial.ttf");
    candidates.push_back("assets\\fonts\\segoeui.ttf");
    candidates.push_back("arial.ttf");

    const char* windowsDirectory = std::getenv("WINDIR");
    if (windowsDirectory && *windowsDirectory) {
        string fontsDirectory = string(windowsDirectory) + "\\Fonts\\";
        candidates.push_back(fontsDirectory + "arial.ttf");
        candidates.push_back(fontsDirectory + "segoeui.ttf");
        candidates.push_back(fontsDirectory + "tahoma.ttf");
    }

    candidates.push_back("C:\\Windows\\Fonts\\arial.ttf");
    candidates.push_back("C:\\Windows\\Fonts\\segoeui.ttf");
    candidates.push_back("C:\\Windows\\Fonts\\tahoma.ttf");
    return candidates;
}

static TTF_Font* openApplicationFont(int pointSize) {
    for (const string& path : applicationFontCandidates()) {
        if (!appFileExists(path)) continue;
        TTF_Font* loaded = TTF_OpenFont(path.c_str(), pointSize);
        if (loaded) return loaded;
    }
    return nullptr;
}


enum class PinKind {
    INPUT,
    OUTPUT,
    PASSIVE
};

struct LocalPin {
    int x;
    int y;
    PinKind kind;
};

// Section 6 OOP hierarchy.  The GUI still stores placement data separately,
// while every electrical part obtains its pins and defaults polymorphically.
class Component {
protected:
    string componentName;
    string componentCategory;
    string initialValue;

public:
    Component(string name, string category, string value)
        : componentName(std::move(name)), componentCategory(std::move(category)),
          initialValue(std::move(value)) {}
    virtual ~Component() = default;

    const string& name() const { return componentName; }
    const string& category() const { return componentCategory; }
    const string& defaultValue() const { return initialValue; }
    virtual vector<LocalPin> localPins(int inputCount) const = 0;
};

class FixedPinComponent : public Component {
    vector<LocalPin> fixedPins;
public:
    FixedPinComponent(string name, string category, string value, vector<LocalPin> pins)
        : Component(std::move(name), std::move(category), std::move(value)),
          fixedPins(std::move(pins)) {}
    vector<LocalPin> localPins(int) const override { return fixedPins; }
};

class PassiveComponent : public FixedPinComponent {
public:
    PassiveComponent(string name, string value, vector<LocalPin> pins)
        : FixedPinComponent(std::move(name), "Passive", std::move(value), std::move(pins)) {}
};

class SourceComponent : public FixedPinComponent {
public:
    SourceComponent(string name, string value, vector<LocalPin> pins)
        : FixedPinComponent(std::move(name), "Sources", std::move(value), std::move(pins)) {}
};

class InteractiveComponent : public FixedPinComponent {
public:
    InteractiveComponent(string name, string value, vector<LocalPin> pins)
        : FixedPinComponent(std::move(name), "Interactive", std::move(value), std::move(pins)) {}
};

class DisplayComponent : public FixedPinComponent {
public:
    DisplayComponent(string name, string value, vector<LocalPin> pins)
        : FixedPinComponent(std::move(name), "Display", std::move(value), std::move(pins)) {}
};

class LogicGateComponent : public Component {
public:
    LogicGateComponent(string name, string value)
        : Component(std::move(name), "Digital Logic", std::move(value)) {}

    vector<LocalPin> localPins(int inputCount) const override {
        vector<LocalPin> pins;
        if (componentName == "NOT Gate") {
            pins.push_back({-22, 0, PinKind::INPUT});
            pins.push_back({ 24, 0, PinKind::OUTPUT});
            return pins;
        }

        int count = std::max(2, std::min(8, inputCount));
        for (int i = 0; i < count; ++i) {
            int y = count == 1 ? 0 : -14 + (28 * i) / (count - 1);
            pins.push_back({-28, y, PinKind::INPUT});
        }
        pins.push_back({24, 0, PinKind::OUTPUT});
        return pins;
    }
};

class DFlipFlopComponent : public Component {
public:
    DFlipFlopComponent()
        : Component("D Flip-Flop", "Digital Logic", "delay=10") {}

    vector<LocalPin> localPins(int) const override {
        return {
            {-30, -10, PinKind::INPUT},   // D
            {-30,  10, PinKind::INPUT},   // CLK
            { 30,   0, PinKind::OUTPUT}   // Q
        };
    }
};

class ComponentLibrary {
public:
    static const Component& get(const string& name) {
        static PassiveComponent resistor("Resistor", "1000", {{-32,0,PinKind::PASSIVE},{32,0,PinKind::PASSIVE}});
        static PassiveComponent capacitor("Capacitor", "0.000001", {{0,-18,PinKind::PASSIVE},{0,18,PinKind::PASSIVE}});
        static PassiveComponent inductor("Inductor", "0.001", {{-27,0,PinKind::PASSIVE},{27,0,PinKind::PASSIVE}});

        static SourceComponent ground("Ground", "0", {{0,-12,PinKind::OUTPUT}});
        static SourceComponent vcc("VCC", "5", {{0,-14,PinKind::OUTPUT}});
        static SourceComponent dc("DC Voltage Source", "5", {{0,-18,PinKind::OUTPUT},{0,18,PinKind::OUTPUT}});
        static SourceComponent battery("Battery", "9;internal=0.5", {{0,-18,PinKind::OUTPUT},{0,18,PinKind::OUTPUT}});
        static SourceComponent clock("Clock Generator", "1", {{24,0,PinKind::OUTPUT}});

        static InteractiveComponent sw("Switch", "OPEN", {{-28,0,PinKind::PASSIVE},{28,0,PinKind::PASSIVE}});
        static InteractiveComponent button("Push Button", "MOMENTARY", {{-28,0,PinKind::PASSIVE},{28,0,PinKind::PASSIVE}});

        static DisplayComponent led("LED", "RED", {{0,-16,PinKind::PASSIVE},{0,16,PinKind::PASSIVE}});
        static DisplayComponent seven("7-Segment", "COMMON_CATHODE", {
            {-18,-14,PinKind::INPUT},{-18,-6,PinKind::INPUT},{-18,2,PinKind::INPUT},{-18,10,PinKind::INPUT},
            {18,-14,PinKind::INPUT},{18,-6,PinKind::INPUT},{18,2,PinKind::INPUT},{18,10,PinKind::INPUT}
        });

        static LogicGateComponent andGate("AND Gate", "inputs=2;delay=10");
        static LogicGateComponent orGate("OR Gate", "inputs=2;delay=10");
        static LogicGateComponent notGate("NOT Gate", "delay=10");
        static LogicGateComponent nandGate("NAND Gate", "inputs=2;delay=10");
        static LogicGateComponent xorGate("XOR Gate", "inputs=2;delay=10");
        static DFlipFlopComponent dff;

        static FixedPinComponent npn("NPN", "Transistor", "", {{0,-14,PinKind::PASSIVE},{0,14,PinKind::PASSIVE},{-12,0,PinKind::INPUT}});
        static FixedPinComponent pnp("PNP", "Transistor", "", {{0,-14,PinKind::PASSIVE},{0,14,PinKind::PASSIVE},{-12,0,PinKind::INPUT}});
        static FixedPinComponent generic("Generic", "Other", "", {{-15,0,PinKind::PASSIVE},{15,0,PinKind::PASSIVE}});

        if (name == "Resistor") return resistor;
        if (name == "Capacitor") return capacitor;
        if (name == "Inductor") return inductor;
        if (name == "Ground") return ground;
        if (name == "VCC") return vcc;
        if (name == "DC Voltage Source") return dc;
        if (name == "Battery") return battery;
        if (name == "Clock Generator") return clock;
        if (name == "Switch") return sw;
        if (name == "Push Button") return button;
        if (name == "LED") return led;
        if (name == "7-Segment") return seven;
        if (name == "AND Gate") return andGate;
        if (name == "OR Gate") return orGate;
        if (name == "NOT Gate") return notGate;
        if (name == "NAND Gate") return nandGate;
        if (name == "XOR Gate") return xorGate;
        if (name == "D Flip-Flop") return dff;
        if (name == "NPN" || name == "Transistor") return npn;
        if (name == "PNP") return pnp;
        return generic;
    }
};

enum class LogicLevel {
    LOW,
    HIGH,
    UNDEFINED
};

class LogicStandard {
public:
    static constexpr double lowMaximum = 0.8;
    static constexpr double highMinimum = 2.0;
    static constexpr double lowVoltage = 0.0;
    static constexpr double highVoltage = 5.0;

    static LogicLevel fromVoltage(double voltage) {
        if (std::isnan(voltage)) return LogicLevel::UNDEFINED;
        if (voltage <= lowMaximum) return LogicLevel::LOW;
        if (voltage >= highMinimum) return LogicLevel::HIGH;
        return LogicLevel::UNDEFINED;
    }

    static double toVoltage(LogicLevel level) {
        if (level == LogicLevel::LOW) return lowVoltage;
        if (level == LogicLevel::HIGH) return highVoltage;
        return std::numeric_limits<double>::quiet_NaN();
    }
};

struct placedComp {
    string name;
    int x = 0;
    int y = 0;
    int angle = 0;
    bool flipH = false;
    bool flipV = false;
    string label;
    string value;

    // Section 6 configuration/state saved with the project.
    int id = 0;
    int inputCount = 2;
    double propagationDelayMs = 10.0;
    bool switchState = false;
};

struct project {
    string name;
    string path;
    string lastP;
    int canvasW;
    int canvasH;
    vector<string> activeComponents;

    project(string n, string p , string date, int cw = 800, int ch = 600, vector<string> ac = {})
        : name(n), path(p), lastP(date), canvasW(cw), canvasH(ch), activeComponents (ac) {}
};

enum class app {
    STARTUP_MENU,
    NEW_PROJECT_DIALOG,
    CUSTOM_SIZE_DIALOG,
    PROJECT_NAME_DIALOG,
    WORKSPACE
};

enum class canvasPreset {
    A4,
    A3,
    CUSTOM
};

enum class Tool {
    SELECT,
    WIRE ,
    COMPONENT
};

class txtIn {

private:
    SDL_Rect box;
    string text;
    bool active ;
    bool numericOnly;
    SDL_Texture* txtTexture;
    TTF_Font* font;
    SDL_Renderer* renderer;

    void updateTexture() {
        if (txtTexture)
            SDL_DestroyTexture( txtTexture);

        if (!font)
            return;

        SDL_Color textColor = {20, 20, 20, 255};
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.empty() ? " ": text.c_str(), textColor);

        if (surf) {
            txtTexture = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        }
        else {
            txtTexture =nullptr;
        }
    }

public:
    txtIn( SDL_Renderer* rend, TTF_Font* f, int x, int y,int w, int h, bool numOnly= true)
        : text(""), active(false), numericOnly(numOnly), txtTexture(nullptr), font(f), renderer(rend) {
        box = {x, y, w, h};
        updateTexture ();
    }

    ~txtIn() {
        if (txtTexture)
            SDL_DestroyTexture (txtTexture);
    }

    void setTxt(const string& t) {
        text = t;
        updateTexture();
    }

    void setActive(bool a) {
        active = a;
        if (active) {
            SDL_StartTextInput();
        }
    }

    void handleEvent (const SDL_Event& e) {
        if ( !active)
            return;

        if (e.type == SDL_KEYDOWN && e.key.keysym.sym== SDLK_BACKSPACE && !text.empty()) {
            text.pop_back();
            updateTexture();
        }

        if(numericOnly) {
            if (e.type == SDL_KEYDOWN) {
                if ( e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9 ){
                    if (text.size() < 5) {
                        text += (char)('0'+ (e.key.keysym.sym - SDLK_0));
                        updateTexture();
                    }
                }
                else if (e.key.keysym.sym >= SDLK_KP_0 && e.key.keysym.sym <= SDLK_KP_9) {
                    if (text.size()< 5) {
                        text += (char)('0' + (e.key.keysym.sym - SDLK_KP_0));
                        updateTexture();
                    }
                }
            }
        }
        else {
            if (e.type == SDL_TEXTINPUT) {
                string input =e.text.text;
                for (char c : input) {
                    if (text.size () < 30) {
                        text += c;
                    }
                }
                updateTexture();
            }
        }
    }


    void draw (SDL_Renderer* rend) const {
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        SDL_RenderFillRect(rend, &box);
        if (active)
            SDL_SetRenderDrawColor( rend, 50, 180, 50, 255);
        else
            SDL_SetRenderDrawColor (rend, 120,120, 120, 255);
        SDL_RenderDrawRect(rend, &box);

        if (txtTexture) {
            SDL_Rect textRect;
            textRect.w = min(box.w - 10, 200);
            textRect.h = box.h -6;
            textRect.x = box.x + 5;
            textRect.y = box.y + (box.h - textRect.h) / 2;
            SDL_QueryTexture(txtTexture, nullptr, nullptr,&textRect.w, &textRect.h);
            SDL_RenderCopy(rend, txtTexture, nullptr, &textRect);
        }
    }

    void drawAt(SDL_Renderer* rend, int x, int y) const {
        SDL_Rect shiftedBox = {x, y, box.w, box.h};
        SDL_SetRenderDrawColor (rend, 255, 255, 255, 255);
        SDL_RenderFillRect(rend,&shiftedBox);
        if (active)
            SDL_SetRenderDrawColor( rend, 50, 180, 50, 255);
        else
            SDL_SetRenderDrawColor (rend, 120,120, 120, 255);
        SDL_RenderDrawRect(rend, &shiftedBox);

        if (txtTexture) {
            SDL_Rect textRect;
            textRect.w = min(shiftedBox.w - 10, 200);
            textRect.h = shiftedBox.h - 6 ;
            SDL_QueryTexture( txtTexture, nullptr, nullptr, &textRect.w, &textRect.h);
            textRect.x = shiftedBox.x + 5;
            textRect.y = shiftedBox.y + (shiftedBox.h - textRect.h) / 2;
            SDL_RenderCopy(rend, txtTexture, nullptr, &textRect);
        }
    }

    string gTxt() const { return text; }

    bool isActive() const {return active; }

    bool mouseIn (int mx, int my) const {
        return (mx >= box.x && mx <= box.x + box.w &&
                my >= box.y && my <= box.y + box.h );
    }

    SDL_Rect gBox() const { return box; }
    void setBox (int x, int y, int w, int h) { box = {x, y, w, h}; }
};


class BUTTONS {
private:
    SDL_Rect rect;
    SDL_Color normalColor;
    SDL_Color hoverColor;
    bool hover;

    SDL_Texture* txtTexture;
    SDL_Rect textRect;

public:
    BUTTONS(SDL_Renderer* renderer, TTF_Font* font, int x,int y, int w, int h,
            SDL_Color nColor, SDL_Color hColor, string text) {

        rect  ={x, y, w, h};
        normalColor = nColor;
        hoverColor = hColor;
        hover = false ;
        txtTexture = nullptr;


        if (font) {
            SDL_Color textColor = {20, 20, 20, 255} ;
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str (), textColor);

            if (textSurface) {
                txtTexture= SDL_CreateTextureFromSurface (renderer, textSurface);
                textRect.w = textSurface->w;
                textRect.h = textSurface-> h;

                textRect.x = x+ (w - textRect.w) / 2;
                textRect.y = y + (h - textRect.h) / 2;
                SDL_FreeSurface(textSurface);
            }
        }
    }

    ~BUTTONS() {
        if (txtTexture) {
            SDL_DestroyTexture( txtTexture );
        }
    }

    void events(const SDL_Event& e){
        if (e.type == SDL_MOUSEMOTION){
            int mx = e.motion.x;
            int my = e.motion.y;
            hover = (mx >= rect.x && mx<= rect.x + rect.w &&
                     my >= rect.y && my <= rect.y + rect.h);
        }
    }

    bool click (const SDL_Event& e) const {
        if (e.type ==SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x;
            int my = e.button.y;
            return (mx >= rect.x && mx<= rect.x + rect.w &&
                    my >= rect.y && my <= rect.y + rect.h);
        }
        return false;
    }

    void draw (SDL_Renderer* renderer) const {
        if (hover) {
            SDL_SetRenderDrawColor(renderer,hoverColor.r, hoverColor.g, hoverColor.b, hoverColor.a);
        }
        else {
            SDL_SetRenderDrawColor(renderer,normalColor.r, normalColor.g, normalColor.b, normalColor.a);
        }
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 80,80, 80, 255);
        SDL_RenderDrawRect(renderer, &rect);

        if (txtTexture) {
            SDL_RenderCopy(renderer, txtTexture , nullptr, &textRect);
        }
    }

    void drawAt(SDL_Renderer* renderer, int x, int y) const {
        SDL_Rect shiftedRect = { x, y, rect.w, rect.h};
        SDL_Rect shiftedTextRect = {
            x + (rect.w - textRect.w) / 2, y + (rect.h - textRect.h) / 2, textRect.w, textRect.h
        };

        if (hover) {
            SDL_SetRenderDrawColor(renderer,hoverColor.r, hoverColor.g,hoverColor.b, hoverColor.a);
        }
        else {
            SDL_SetRenderDrawColor(renderer,normalColor.r, normalColor.g, normalColor.b, normalColor.a);
        }
        SDL_RenderFillRect(renderer, &shiftedRect);
        SDL_SetRenderDrawColor(renderer, 80,80, 80, 255) ;
        SDL_RenderDrawRect(renderer, &shiftedRect);

        if (txtTexture) {
            SDL_RenderCopy(renderer , txtTexture, nullptr, &shiftedTextRect);
        }
    }

    SDL_Rect gRect() const { return rect;}
    void setRect(int x, int y, int w, int h ) { rect = {x, y, w, h}; textRect.x = x + (w - textRect.w)/2; textRect.y = y + (h - textRect.h)/2; }
};


class PROTEUS {
private:

    SDL_Window* window;
    SDL_Renderer*renderer;
    TTF_Font* font;
    TTF_Font* titleFont ;
    TTF_Font* libFont;
    bool running ;
    app currentState;

    BUTTONS* btnNewP;
    BUTTONS* btnOpenP;
    BUTTONS* btnRemRecents;
    vector<BUTTONS*> btnRecents;
    vector<project> recentPs;

    BUTTONS* btnPresetA4;
    BUTTONS* btnPresetA3;
    BUTTONS* btnPresetCustom;
    BUTTONS* btnCancelDialog;

    txtIn* txtWidth;
    txtIn* txtHeight;
    BUTTONS* btnCustomOK;
    BUTTONS* btnCustomCancel;

    txtIn* txtProjectName;
    BUTTONS* btnNameOK;
    BUTTONS*  btnNameCancel;

    BUTTONS* btnBack;

    int canvasWidth;
    int canvasHeight;
    string penProjectName;

    int worldMinX,worldMaxX;                                      
    int worldMinY,worldMaxY;                                      

    int grdSz;
    float zmLvl;
    int panX , panY;
    bool isPan;
    int panStartX, panStartY;
    int panOffXst, panOffYst;
    int statH ;
    int winW, winH;
    int mseX, mseY;

    SDL_Rect zoomRct ;

    bool showGrid;
    Tool currentTool;

    int tlbrH ;
    int pnlLW , pnlRW;
    int vpX, vpY, vpW , vpH;

    vector<BUTTONS*> tlbrBtns;
    vector<string> libItms;
    vector <SDL_Rect> libRcts;
    string selLibItm;

    struct tree {
        string name;
        vector<string> components;
        bool expanded;
        tree(string n, vector<string> comps) : name(n), components(comps), expanded(false) {}
    };
    vector<tree> libCategories;
    txtIn* searchBox;
    string searchFilter;
    vector<string> activeComps;
    SDL_Rect previewRect ;
    bool showLib;
    bool showProp;
    int activeCompsY;
    string currentProjectPath;

    vector <placedComp> placedComponents;

    vector<size_t> selectedIndices;
    bool draggingComponents;
    int dragStartX, dragStartY;
    vector<placedComp>  dragSnapshots;
    vector<placedComp>  preDragComponents;
    vector<vector<SDL_Point>> preDragWires;
    SDL_Rect selectionRect;
    bool drawingSelection;

    bool mouseHandled;

    bool wireStartActive;
    SDL_Point wireStartPoint;
    vector<vector<SDL_Point>> wires;

    vector< SDL_Point> junctions;                                    

    SDL_Point hoveredPin;
    bool hoveredPinActive;


    struct ComponentRuntime {
        vector<double> pinVoltages;
        double outputVoltage = std::numeric_limits<double>::quiet_NaN();
        double pendingOutput = std::numeric_limits<double>::quiet_NaN();
        Uint32 pendingDue = 0;
        bool hasPending = false;
        bool pressed = false;
        LogicLevel previousClock = LogicLevel::LOW;
        bool ledOn = false;
        vector<bool> segments = vector<bool>(8, false);
    };

    unordered_map<int, ComponentRuntime> componentRuntime;
    int nextComponentId;
    vector<double> wireVoltages;
    vector<string> simulationLog;
    set<string> lastSimulationWarnings;

    void updateWorldBounds() {                                      
        worldMinX = -canvasWidth/ 2;                               
        worldMaxX =  canvasWidth/ 2;                               
        worldMinY = -canvasHeight/ 2;                              
        worldMaxY =  canvasHeight/ 2;                              
    }                                                               

    int wldToScrX(int wx) const {return vpX + (int)((wx - worldMinX) * zmLvl + panX); }   
    int wldToScrY(int wy) const { return vpY + (int)((worldMaxY - wy) * zmLvl + panY); }   
    int scrToWldX(int sx) const{ return worldMinX + (int)((sx - vpX - panX)/ zmLvl); }   
    int scrToWldY(int sy) const { return worldMaxY - (int)((sy - vpY - panY) / zmLvl); }   

    int snapToGrid (int val) const{                                
        int offset = val - worldMinX;                               
        int snappedOffset =((offset + grdSz / 2)/ grdSz) * grdSz; 
        return worldMinX + snappedOffset;                           
    }                                                               

    void clampToCanvas (int& x, int& y) const{                     
        if (x < worldMinX) x = worldMinX;                           
        if (x > worldMaxX) x = worldMaxX;                           
        if (y < worldMinY) y= worldMinY;                           
        if (y > worldMaxY) y= worldMaxY;                           
    }                                                               

    void updateViewport (){
        int leftPanelWidth = showLib ? pnlLW :0;
        int rightPanelWidth = showProp ? pnlRW :0;
        vpX = leftPanelWidth;
        vpW = winW -leftPanelWidth - rightPanelWidth;
        vpY = tlbrH;
        vpH = winH - tlbrH - statH;
    }

    void fitWindowToCanvas () {                                      
        updateViewport();                                           
    }
    

    float pointToSegmentDist(int px, int py, int x1, int y1, int x2, int y2) {
        float dx = x2 - x1, dy = y2 - y1;
        if (dx == 0 && dy == 0) return sqrt((px-x1)*(px-x1) + (py-y1)*(py-y1));
        float t = ((px - x1)*dx + (py - y1)*dy) / (dx*dx + dy*dy);
        t = max(0.0f, min(1.0f, t));
        float nx = x1 + t*dx, ny = y1 + t*dy;
        return sqrt((px-nx)*(px-nx) + (py-ny)*(py-ny));
    }

    vector<SDL_Point> calcOrthoPath(SDL_Point start, SDL_Point end) {
        vector<SDL_Point> path;
        path.push_back(start);
        int dx = end.x - start.x, dy = end.y - start.y;
        if (dx == 0 || dy == 0) {
            path.push_back(end);
            return path;
        }
        if (abs(dx) > abs(dy)) {
            path.push_back({start.x + dx, start.y});
            path.push_back({start.x + dx, end.y});
        } else {
            path.push_back({start.x, start.y + dy});
            path.push_back({end.x, start.y + dy});
        }
        path.push_back(end);
        return path;
    }

    void updateWiresForMovingComp(){                          
        if (selectedIndices.empty()) return;                         
        for (size_t j = 0; j< selectedIndices.size(); ++j) {       
            size_t idx = selectedIndices [j];                         
            if (idx >= placedComponents.size()) continue;            
            placedComp oldComp = dragSnapshots[j];                   
            placedComp newComp = placedComponents [idx];              
            vector<SDL_Point> oldPins = gCompPinPos(oldComp);        
            vector<SDL_Point> newPins = gCompPinPos(newComp);        
            for (auto& wire :wires) {                               
                for (size_t pi = 0; pi < oldPins.size(); ++pi) {     
                    if (wire.front().x == oldPins[pi].x && wire.front().y == oldPins[pi].y) { 
                        wire.front() ={newPins[pi].x, newPins[pi].y}; 
                        break;                                       
                    }                                                
                    if (wire.back().x == oldPins[pi].x&& wire.back().y == oldPins[pi].y) { 
                        wire.back() = {newPins[pi].x, newPins[pi].y}; 
                        break;                                       
                    }                                                
                }                                                    
            }                                                        
        }                                                            
        for (auto& wire : wires){                                   
            if (wire.size() >=2) {                                  
                wire = calcOrthoPath(wire.front(), wire.back());     
            }                                                        
        }                                                            
    }                                                                

    void moveWiresSmooth( int deltaX,int deltaY ) {                 
        if (selectedIndices.empty()) return;                         
        set<int> movedWires;                                         
        for (size_t j = 0; j < selectedIndices.size(); ++j) {        
            size_t idx =selectedIndices[j];                         
            if (idx >= placedComponents.size()) continue;            
            placedComp oldComp = dragSnapshots[j];                   
            vector<SDL_Point> oldPins = gCompPinPos (oldComp);        
            for (size_t wi = 0; wi < wires.size() ; ++wi) {           
                if (movedWires.count(wi)) continue;                  
                auto& wire = wires[wi];                              
                bool found = false;                                  
                for (size_t pi = 0 ; pi < oldPins.size(); ++pi) {     
                    if (wire.front().x == oldPins[pi].x && wire.front().y == oldPins[pi].y) { 
                        for (auto& pt : wire) {                      
                            pt.x += deltaX;                          
                            pt.y += deltaY;                          
                        }                                            
                        found = true;                                
                        break;                                       
                    }                                                
                    if (wire.back().x == oldPins[pi] .x && wire.back().y == oldPins[pi].y) { 
                        for (auto& pt : wire) {                      
                            pt.x += deltaX;                          
                            pt.y +=deltaY;                          
                        }                                            
                        found = true;                                
                        break;                                       
                    }                                                
                }                                                    
                if (found) movedWires.insert(wi);                    
            }                                                        
        }                                                            
    }                                                                

    bool segmentsIntersect(SDL_Point p1 , SDL_Point p2, SDL_Point q1,SDL_Point q2, SDL_Point& intersection) { 
        int d1x = p2.x- p1.x, d1y = p2.y - p1.y;                   
        int d2x = q2.x - q1.x, d2y = q2.y - q1.y;                   
        int cross = d1x * d2y - d1y * d2x;                           
        if (cross ==0) return false;                                
        int dx = q1.x - p1.x, dy = q1.y - p1.y;                     
        float t = float(dx * d2y - dy * d2x) / cross;                
        float u = float(dx * d1y - dy * d1x) / cross;                
        if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {     
            intersection.x = p1.x + (int) (t * d1x + 0.5f);           
            intersection.y = p1.y + (int) (t * d1y + 0.5f);           
            return true;                                             
        }                                                            
        return false;                                                
    }                                                                

    bool pointNear(SDL_Point a , SDL_Point b, int threshold = 6){    
        int dx = a.x - b.x, dy =a.y - b.y;                          
        return (dx*dx + dy*dy) <= threshold*threshold;               
    }                                                                

    void removeJunctionsOnWire(const vector<SDL_Point>& wire) {      
        for (int i = (int) junctions.size () - 1; i >= 0; --i) {       
            SDL_Point jpt = junctions[i];                            
            for (size_t s = 0; s + 1 < wire.size(); ++s) {            
                if (pointToSegmentDist(jpt.x, jpt.y, wire[s].x, wire[s].y, wire[s+1].x, wire[s+1].y) < 4) { 
                    junctions.erase(junctions.begin() +i);           
                    break;                                           
                }                                                    
            }                                                        
        }                                                            
    }                                                                

    bool isPointInsideComponent ( const placedComp& comp, int wx, int wy ) const { 
        int dx = wx - comp.x;                                        
        int dy = wy - comp.y;                                        
        float rad = -comp.angle * 3.14159f / 180.0f;                 
        float s = sin(rad) , c = cos(rad);                            
        float rx = c*dx - s*dy;                                       
        float ry = s*dx + c*dy;                                       
        int lx = (int)(rx * (comp.flipH ? -1 : 1));                  
        int ly = (int)(ry * (comp.flipV ? -1 : 1));                  
        return (lx >= -40 && lx<= 40 && ly >= -20 && ly <= 20);     
    }                                                                

    void getCompCorners(const placedComp& comp, SDL_Point corners[4]) const { 
        auto transform = [&](int lx, int ly) -> SDL_Point {         
            int sx = comp.flipH? -1 : 1;                            
            int sy = comp.flipV? -1 : 1;                            
            int rx = lx * sx;                                        
            int ry = ly * sy;                                        
            float rad= comp.angle * 3.14159f / 180.0f;             
            float s = sin(rad), c = cos(rad);                       
            int fx = (int)(rx * c - ry * s);                         
            int fy = (int)(rx * s + ry * c);                         
            return {wldToScrX(comp.x + fx),wldToScrY(comp.y + fy)}; 
        };                                                            
        corners[0] = transform(-40, -20);                             
        corners[1] = transform(40, -20);                              
        corners[2] = transform(40, 20);                               
        corners[3] = transform(-40, 20);                              
    }                                                                

    void drawRotatedSelection(const placedComp& comp) const {        
        SDL_Point corners [4];                                        
        getCompCorners(comp, corners);                               

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 60);           
        SDL_Vertex vertices[5];                                      
        for (int i = 0; i < 4; i++) {                                
            vertices[i].position.x = (float)corners[i].x ;            
            vertices[i].position.y = (float)corners[i].y;            
            vertices[i].color = {255, 255, 0, 60};                   
        }                                                            
        int indices[] = {0, 1, 2, 2, 3, 0} ;                         
        if (SDL_RenderGeometry (renderer, nullptr, vertices, 4, indices, 6) < 0) { 
            SDL_Rect bb = {corners[0].x, corners[0].y, 0, 0};        
            for (int i = 1; i < 4;i++) {                            
                if (corners[i].x < bb.x) bb.x = corners[i].x;        
                if (corners[i].y < bb.y) bb.y = corners[i].y;        
            }                                                        
            bb.w = max(corners[1].x, corners [2].x) - bb.x;           
            bb.h = max(corners[2].y, corners[3].y) - bb.y;           
            SDL_RenderFillRect(renderer, &bb);                       
        }                                                            

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);            
        SDL_Point outline[5];                                        
        for (int i = 0; i < 4; i++) outline[i] = corners[i];         
        outline[4] = corners[0];                                     
        SDL_RenderDrawLines(renderer, outline,5);                   
    }                                                                

    enum UndoType {COMP_PLACE, COMP_DELETE , COMP_MOVE, ACTIVE_ADD, ACTIVE_REMOVE, COMP_EDIT, COMP_TRANSFORM,WIRE_ADD, WIRE_DELETE, WIRE_JUNCTION_ADD, WIRE_JUNCTION_REMOVE}; 
    struct UndoAction{
        UndoType type;
        placedComp comp;
        placedComp oldComp;
        string compName;
        int activeIndex;
        vector<placedComp> compsBefore;
        vector<placedComp> compsAfter ;
        vector<vector<SDL_Point>> wiresBefore;
        vector<vector <SDL_Point>> wiresAfter;
        vector<SDL_Point> junctionsBefore;                          
        vector<SDL_Point> junctionsAfter;                           
        SDL_Point junctionPoint;                                    
    };
    vector<UndoAction> undoStack;
    vector<UndoAction> redoStack;

    txtIn* propLabelInput;
    txtIn* propValueInput;
    BUTTONS*btnPropOK;
    BUTTONS* btnPropCancel;
    size_t editingIndex;
    size_t lastClickedIndex;
    Uint32 lastClickTime;

    void pushUndo (const UndoAction& action){
        undoStack.push_back(action);
        redoStack.clear();
    }

    void undo() {
        if (undoStack.empty()) return;
        UndoAction act = undoStack.back();
        undoStack.pop_back();
        if (act.type ==COMP_PLACE) {
            for (size_t i = 0; i < placedComponents.size(); ++i) {
                if (placedComponents[i].name == act.comp.name && placedComponents[i].x == act.comp.x && placedComponents[i].y == act.comp.y) {
                    placedComponents. erase(placedComponents.begin() + i);
                    break;
                }
            }
            wires = act.wiresBefore;
            junctions =act.junctionsBefore;                         
            redoStack.push_back(act);
        }
        else if (act.type== COMP_DELETE) {
            placedComponents.push_back(act.comp);
            wires = act.wiresBefore;
            junctions = act.junctionsBefore;                         
            redoStack.push_back(act) ;
        }
        else if (act.type == ACTIVE_ADD) {
            auto it = find(activeComps.begin( ), activeComps.end(), act.compName);
            if (it != activeComps.end()) {
                activeComps.erase(it);
                UndoAction redoAct;
                redoAct.type = ACTIVE_ADD ;
                redoAct.compName = act.compName;
                redoStack.push_back(redoAct);
            }
        }
        else if (act.type == ACTIVE_REMOVE) {
            if (act.activeIndex >= 0 && act.activeIndex<= (int)activeComps.size()) {
                activeComps.insert(activeComps.begin() + act.activeIndex, act.compName);
                UndoAction redoAct;
                redoAct.type = ACTIVE_REMOVE;
                redoAct.compName = act.compName;
                redoAct.activeIndex = act.activeIndex;
                redoStack.push_back(redoAct);
            }
        }
        else if (act.type == COMP_MOVE){
            placedComponents = act.compsBefore;
            wires = act.wiresBefore;
            redoStack.push_back(act);
        }
        else if (act.type == COMP_TRANSFORM) {
            placedComponents = act.compsBefore;
            wires = act.wiresBefore;
            redoStack.push_back (act);
        }
        else if (act.type == COMP_EDIT) {
            size_t idx = -1;
            for(size_t i = 0; i < placedComponents.size(); ++i){
                if (placedComponents[i].name== act.comp.name && placedComponents[i].x == act.comp.x && placedComponents[i].y == act.comp.y) {
                    idx = i;
                    break;
                }
            }
            if (idx < placedComponents.size() ) {
                placedComponents[idx] = act.oldComp;
                UndoAction redoAct;
                redoAct.type = COMP_EDIT;
                redoAct.oldComp = act.oldComp;
                redoAct.comp = act.comp;
                redoStack.push_back (redoAct);
            }
        }
        else if (act.type == WIRE_ADD) {
            wires = act.wiresBefore;
            junctions = act.junctionsBefore ;                         
            redoStack.push_back(act);
        }
        else if (act.type == WIRE_DELETE) {
            wires = act.wiresBefore;
            junctions = act.junctionsBefore;                         
            redoStack.push_back(act);
        }
        else if (act.type== WIRE_JUNCTION_ADD) {                   
            junctions = act.junctionsBefore;                         
            redoStack.push_back(act);                                
        }                                                            
        else if (act.type == WIRE_JUNCTION_REMOVE) {                 
            junctions = act.junctionsBefore;                         
            redoStack.push_back (act);                                
        }                                                            
    }

    void redo() {
        if (redoStack.empty()) return;
        UndoAction act = redoStack.back();
        redoStack.pop_back();
        if (act.type == COMP_PLACE){
            placedComponents.push_back(act.comp) ;
            wires = act.wiresAfter;
            junctions= act.junctionsAfter;                          
            undoStack.push_back(act);
        }
        else if (act.type == COMP_DELETE) {
            for(size_t i = 0; i< placedComponents.size(); ++i) {
                if (placedComponents[i].name == act.comp.name && placedComponents [i].x == act.comp.x && placedComponents[i].y == act.comp.y) {
                    placedComponents.erase(placedComponents.begin()+ i);
                    break;
                }
            }
            wires = act.wiresAfter;
            junctions = act.junctionsAfter;                          
            undoStack.push_back(act);
        }
        else if (act.type == ACTIVE_ADD) {
            activeComps.push_back(act.compName);
            UndoAction undoAct;
            undoAct.type =ACTIVE_ADD;
            undoAct.compName = act.compName;
            undoStack.push_back (undoAct);
        }
        else if (act.type == ACTIVE_REMOVE) {
            auto it = find(activeComps.begin() , activeComps.end(), act.compName);
            if (it != activeComps.end()) {
                int idx = it - activeComps.begin();
                activeComps.erase(it );
                UndoAction undoAct;
                undoAct.type = ACTIVE_REMOVE;
                undoAct.compName = act.compName;
                undoAct.activeIndex =  idx;
                undoStack.push_back(undoAct);
            }
        }
        else if (act.type ==COMP_MOVE) {
            placedComponents = act.compsAfter;
            wires = act.wiresAfter;
            undoStack.push_back(act);
        }
        else if (act.type == COMP_TRANSFORM ) {
            placedComponents = act.compsAfter;
            wires = act.wiresAfter;
            undoStack.push_back(act);
        }
        else if (act.type ==  COMP_EDIT) {
            size_t idx =-1;
            for (size_t i = 0; i < placedComponents.size();++i) {
                if (placedComponents[i].name == act.oldComp.name && placedComponents[i] .x == act.oldComp.x && placedComponents[i].y == act.oldComp.y) {
                    idx = i;
                    break;
                }
            }
            if (idx < placedComponents.size()){
                placedComponents[idx] = act.comp;
                undoStack.push_back(act);
            }
        }
        else if (act.type == WIRE_ADD) {
            wires =act.wiresAfter;
            junctions = act.junctionsAfter;                          
            undoStack.push_back(act);
        }
        else if (act.type == WIRE_DELETE) {
            wires = act.wiresAfter;
            junctions = act.junctionsAfter;                          
            undoStack.push_back(act);
        }
        else if (act.type == WIRE_JUNCTION_ADD){                   
            junctions = act .junctionsAfter;                          
            undoStack.push_back(act);                                
        }                                                            
        else if ( act.type == WIRE_JUNCTION_REMOVE) {                 
            junctions = act.junctionsAfter;                          
            undoStack.push_back(act);                                
        }                                                            
    }

    void resetView() {
        zmLvl = 1.0f;
        updateViewport();                                            
        panX = vpW / 2 - canvasWidth / 2;                            
        panY = vpH / 2 - canvasHeight/ 2;                           
    }

    void drawGrid() {
        if (!showGrid)
            return ;
        int wL = scrToWldX(vpX), wT = scrToWldY (vpY);                
        int wR= scrToWldX (vpX+vpW), wB = scrToWldY(vpY+vpH);         
        int startX = ((wL - worldMinX) / grdSz) * grdSz + worldMinX; 
        int startY = worldMinY + ((wT - worldMinY) / grdSz) * grdSz; 
        SDL_SetRenderDrawColor (renderer, 200,200,200, 60);
        for (int x = startX; x <= wR ; x += grdSz) {
            int sx = wldToScrX(x);
            if (sx >= vpX && sx < vpX+vpW)
                SDL_RenderDrawLine(renderer, sx, vpY, sx, vpY+vpH);
        }
        for (int y = startY; y >= wB ; y -= grdSz){
            int sy = wldToScrY(y) ;
            if (sy >= vpY && sy < vpY+vpH)
                SDL_RenderDrawLine(renderer, vpX, sy, vpX+ vpW, sy);
        }

        int ox = wldToScrX(0), oy = wldToScrY(0);
        if (ox >= vpX && ox < vpX+vpW && oy >= vpY && oy < vpY+vpH) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_Rect hBar = {ox - 6, oy - 1, 13, 3};
            SDL_Rect vBar= {ox - 1, oy - 6, 3, 13};
            SDL_RenderFillRect(renderer, &hBar);
            SDL_RenderFillRect(renderer, &vBar);
        }
    }

    void drawStatusBar() {
        SDL_Rect bar ={0, winH - statH,winW, statH};
        SDL_SetRenderDrawColor(renderer, 60, 60,60,255);
        SDL_RenderFillRect(renderer, &bar);
        if (mseX >= vpX && mseX < vpX+vpW && mseY >= vpY && mseY < vpY+vpH) {
            int wx = scrToWldX(mseX) , wy= scrToWldY(mseY);
            string txt = "X: " + to_string(wx)+ "  Y: " + to_string(wy);
            drawTxt(txt, 10, winH - statH + 5, {220,220,220,255});
        }
        else {
            drawTxt("Move mouse over canvas", 10, winH - statH + 5,  {180,180,180,255});
        }

        if (!simulationLog.empty()) {
            drawTxt(simulationLog.back(), 185, winH - statH + 5, {255,190,70,255}, libFont);
        }

        string zoomTxt = to_string((int)(zmLvl*100)) + "%" ;
        int tw, th ;
        TTF_SizeText(font, zoomTxt.c_str(), &tw, &th) ;
        int zx = winW - tw - 10 ;
        int zy = winH - statH + 5 ;
        zoomRct = {zx - 5, zy - 2, tw + 10, th + 4} ;
        drawTxt (zoomTxt, zx, zy, {220,220,220,255}) ;
    }

    void drawTxt(const string& str, int x, int y, SDL_Color color) const {
        if (!font)
            return;
        SDL_Surface* surf = TTF_RenderText_Blended(font, str.c_str(), color);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect dest ={x, y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, nullptr, &dest);
            SDL_FreeSurface (surf);
            SDL_DestroyTexture(tex);
        }
    }

    void drawTxt(const string& str, int x, int y, SDL_Color color, TTF_Font* fnt) const {
        if (!fnt)
            return;
        SDL_Surface* surf = TTF_RenderText_Blended (fnt, str.c_str(), color);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect dest ={x, y, surf-> w, surf->h};
            SDL_RenderCopy(renderer, tex, nullptr, &dest);
            SDL_FreeSurface (surf);
            SDL_DestroyTexture(tex);
        }
    }


    string upperCopy(string text) const {
        for (char& c : text) c = (char)std::toupper((unsigned char)c);
        return text;
    }

    string lowerCopy(string text) const {
        for (char& c : text) c = (char)std::tolower((unsigned char)c);
        return text;
    }

    double parseFirstNumber(const string& text, double fallback) const {
        for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (std::isdigit((unsigned char)c) || c == '-' || c == '+' || c == '.') {
                char* end = nullptr;
                double value = std::strtod(text.c_str() + i, &end);
                if (end != text.c_str() + i) return value;
            }
        }
        return fallback;
    }

    double parseNamedNumber(const string& text, const string& key, double fallback) const {
        string low = lowerCopy(text);
        string target = lowerCopy(key) + "=";
        size_t pos = low.find(target);
        if (pos == string::npos) return fallback;
        pos += target.size();
        char* end = nullptr;
        double value = std::strtod(text.c_str() + pos, &end);
        return end == text.c_str() + pos ? fallback : value;
    }

    int parseNamedInteger(const string& text, const string& key, int fallback) const {
        double value = parseNamedNumber(text, key, fallback);
        return (int)std::lround(value);
    }

    bool sameVoltage(double a, double b) const {
        if (std::isnan(a) && std::isnan(b)) return true;
        if (std::isnan(a) || std::isnan(b)) return false;
        return std::fabs(a - b) < 0.01;
    }

    void applyComponentConfig(placedComp& comp) {
        if (comp.name == "AND Gate" || comp.name == "OR Gate" ||
            comp.name == "NAND Gate" || comp.name == "XOR Gate") {
            comp.inputCount = std::max(2, std::min(8, parseNamedInteger(comp.value, "inputs", comp.inputCount)));
            comp.propagationDelayMs = std::max(0.0, parseNamedNumber(comp.value, "delay", comp.propagationDelayMs));
        } else if (comp.name == "NOT Gate" || comp.name == "D Flip-Flop") {
            comp.inputCount = comp.name == "NOT Gate" ? 1 : 2;
            comp.propagationDelayMs = std::max(0.0, parseNamedNumber(comp.value, "delay", comp.propagationDelayMs));
        } else if (comp.name == "Switch") {
            string state = upperCopy(comp.value);
            if (state.find("CLOSED") != string::npos || state.find("ON") != string::npos)
                comp.switchState = true;
            else if (state.find("OPEN") != string::npos || state.find("OFF") != string::npos)
                comp.switchState = false;
        }
    }

    void initializeComponent(placedComp& comp) {
        if (comp.id <= 0) comp.id = nextComponentId++;
        else nextComponentId = std::max(nextComponentId, comp.id + 1);
        if (comp.value.empty()) comp.value = ComponentLibrary::get(comp.name).defaultValue();
        applyComponentConfig(comp);
        if (componentRuntime.find(comp.id) == componentRuntime.end())
            componentRuntime[comp.id] = ComponentRuntime();
    }

    const ComponentRuntime* findRuntime(int id) const {
        auto it = componentRuntime.find(id);
        return it == componentRuntime.end() ? nullptr : &it->second;
    }

    void scheduleOutput(placedComp& comp, double target, Uint32 now) {
        ComponentRuntime& runtime = componentRuntime[comp.id];
        if (sameVoltage(runtime.outputVoltage, target)) {
            runtime.hasPending = false;
            return;
        }
        if (comp.propagationDelayMs <= 0.0) {
            runtime.outputVoltage = target;
            runtime.hasPending = false;
            return;
        }
        if (!runtime.hasPending || !sameVoltage(runtime.pendingOutput, target)) {
            runtime.pendingOutput = target;
            runtime.pendingDue = now + (Uint32)std::lround(comp.propagationDelayMs);
            runtime.hasPending = true;
        }
    }

    void applyPendingOutputs(Uint32 now) {
        for (auto& item : componentRuntime) {
            ComponentRuntime& runtime = item.second;
            if (runtime.hasPending && (int32_t)(now - runtime.pendingDue) >= 0) {
                runtime.outputVoltage = runtime.pendingOutput;
                runtime.hasPending = false;
            }
        }
    }

    void addSimulationWarning(set<string>& warnings, const string& warning) {
        warnings.insert(warning);
    }

    void drawFilledCircle(int cx, int cy, int radius, SDL_Color color) const {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        for (int y = -radius; y <= radius; ++y) {
            int x = (int)std::sqrt((double)(radius * radius - y * y));
            SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
        }
    }

    void updateSection6Simulation() {
        if (currentState != app::WORKSPACE) return;
        Uint32 now = SDL_GetTicks();
        for (auto& comp : placedComponents) initializeComponent(comp);
        applyPendingOutputs(now);

        struct Dsu {
            vector<int> parent;
            vector<int> rank;
            int add() {
                int id = (int)parent.size();
                parent.push_back(id);
                rank.push_back(0);
                return id;
            }
            int find(int x) {
                if (parent[x] != x) parent[x] = find(parent[x]);
                return parent[x];
            }
            void unite(int a, int b) {
                a = find(a); b = find(b);
                if (a == b) return;
                if (rank[a] < rank[b]) std::swap(a, b);
                parent[b] = a;
                if (rank[a] == rank[b]) rank[a]++;
            }
        } dsu;

        auto pointKey = [](int x, int y) -> int64_t {
            return (int64_t)((uint64_t)(uint32_t)x << 32 | (uint32_t)y);
        };
        unordered_map<int64_t, int> pointNodes;
        auto nodeFor = [&](SDL_Point point) -> int {
            int64_t key = pointKey(point.x, point.y);
            auto it = pointNodes.find(key);
            if (it != pointNodes.end()) return it->second;
            int id = dsu.add();
            pointNodes[key] = id;
            return id;
        };

        vector<vector<int>> componentPinNodes(placedComponents.size());
        vector<vector<SDL_Point>> componentPins(placedComponents.size());
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            componentPins[ci] = gCompPinPos(placedComponents[ci]);
            for (const SDL_Point& pin : componentPins[ci])
                componentPinNodes[ci].push_back(nodeFor(pin));
        }

        vector<int> wireNodes(wires.size(), -1);
        for (size_t wi = 0; wi < wires.size(); ++wi) {
            if (wires[wi].empty()) continue;
            int first = nodeFor(wires[wi].front());
            wireNodes[wi] = first;
            for (const SDL_Point& point : wires[wi]) dsu.unite(first, nodeFor(point));
        }

        // A pin can be attached to the middle of a wire; a junction explicitly joins crossings.
        for (size_t ci = 0; ci < componentPins.size(); ++ci) {
            for (size_t pi = 0; pi < componentPins[ci].size(); ++pi) {
                SDL_Point pin = componentPins[ci][pi];
                for (size_t wi = 0; wi < wires.size(); ++wi) {
                    if (wireNodes[wi] < 0) continue;
                    for (size_t si = 0; si + 1 < wires[wi].size(); ++si) {
                        if (pointToSegmentDist(pin.x, pin.y,
                            wires[wi][si].x, wires[wi][si].y,
                            wires[wi][si+1].x, wires[wi][si+1].y) < 0.75f) {
                            dsu.unite(componentPinNodes[ci][pi], wireNodes[wi]);
                            break;
                        }
                    }
                }
            }
        }
        for (const SDL_Point& junction : junctions) {
            int junctionNode = nodeFor(junction);
            for (size_t wi = 0; wi < wires.size(); ++wi) {
                if (wireNodes[wi] < 0) continue;
                for (size_t si = 0; si + 1 < wires[wi].size(); ++si) {
                    if (pointToSegmentDist(junction.x, junction.y,
                        wires[wi][si].x, wires[wi][si].y,
                        wires[wi][si+1].x, wires[wi][si+1].y) < 0.75f) {
                        dsu.unite(junctionNode, wireNodes[wi]);
                        break;
                    }
                }
            }
        }

        // Closed switches and held push buttons electrically merge their two terminals.
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            placedComp& comp = placedComponents[ci];
            if (componentPinNodes[ci].size() < 2) continue;
            bool closed = comp.name == "Switch" && comp.switchState;
            auto rtIt = componentRuntime.find(comp.id);
            bool pressed = comp.name == "Push Button" && rtIt != componentRuntime.end() && rtIt->second.pressed;
            if (closed || pressed) dsu.unite(componentPinNodes[ci][0], componentPinNodes[ci][1]);
        }

        struct NetValue {
            bool driven = false;
            bool undefined = false;
            double voltage = std::numeric_limits<double>::quiet_NaN();
        };
        unordered_map<int, NetValue> nets;
        set<string> warnings;

        auto drive = [&](int node, double voltage, const string& sourceName) {
            int root = dsu.find(node);
            NetValue& net = nets[root];
            if (std::isnan(voltage)) {
                net.driven = true;
                net.undefined = true;
                net.voltage = std::numeric_limits<double>::quiet_NaN();
                return;
            }
            if (!net.driven) {
                net.driven = true;
                net.voltage = voltage;
                return;
            }
            if (!net.undefined && std::fabs(net.voltage - voltage) > 0.4) {
                net.undefined = true;
                net.voltage = std::numeric_limits<double>::quiet_NaN();
                addSimulationWarning(warnings, "Conflicting voltage sources detected near " + sourceName + ".");
            }
        };

        auto readVoltage = [&](int node) -> double {
            int root = dsu.find(node);
            auto it = nets.find(root);
            if (it == nets.end() || !it->second.driven || it->second.undefined)
                return std::numeric_limits<double>::quiet_NaN();
            return it->second.voltage;
        };

        // Independent sources and previously settled digital outputs drive their nets.
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            placedComp& comp = placedComponents[ci];
            vector<int>& pins = componentPinNodes[ci];
            if (pins.empty()) continue;
            string displayName = comp.label.empty() ? comp.name : comp.label;
            if (comp.name == "Ground") drive(pins[0], 0.0, displayName);
            else if (comp.name == "VCC") drive(pins[0], 5.0, displayName);
            else if (comp.name == "DC Voltage Source" || comp.name == "Battery") {
                double sourceVoltage = parseFirstNumber(comp.value, comp.name == "Battery" ? 9.0 : 5.0);
                drive(pins[0], sourceVoltage, displayName);
                if (pins.size() > 1) drive(pins[1], 0.0, displayName);
            } else if (comp.name == "Clock Generator") {
                double frequency = std::max(0.01, parseFirstNumber(comp.value, 1.0));
                double phase = std::fmod((now / 1000.0) * frequency, 1.0);
                drive(pins[0], phase < 0.5 ? 0.0 : 5.0, displayName);
            } else if (comp.name == "AND Gate" || comp.name == "OR Gate" ||
                       comp.name == "NOT Gate" || comp.name == "NAND Gate" ||
                       comp.name == "XOR Gate" || comp.name == "D Flip-Flop") {
                ComponentRuntime& runtime = componentRuntime[comp.id];
                drive(pins.back(), runtime.outputVoltage, displayName);
            }
        }

        auto propagatePassive = [&]() {
            bool changed = false;
            for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
                const placedComp& comp = placedComponents[ci];
                if (comp.name != "Resistor" && comp.name != "Inductor") continue;
                const vector<int>& pins = componentPinNodes[ci];
                if (pins.size() < 2) continue;
                double a = readVoltage(pins[0]);
                double b = readVoltage(pins[1]);
                if (!std::isnan(a) && std::isnan(b)) { drive(pins[1], a, comp.name); changed = true; }
                if (std::isnan(a) && !std::isnan(b)) { drive(pins[0], b, comp.name); changed = true; }
            }
            return changed;
        };
        for (int pass = 0; pass < 6; ++pass) if (!propagatePassive()) break;

        // Evaluate gates. Undefined or floating inputs produce the required warning.
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            placedComp& comp = placedComponents[ci];
            vector<int>& pins = componentPinNodes[ci];
            bool isGate = comp.name == "AND Gate" || comp.name == "OR Gate" ||
                          comp.name == "NOT Gate" || comp.name == "NAND Gate" ||
                          comp.name == "XOR Gate";
            if (isGate && pins.size() >= 2) {
                vector<LogicLevel> inputs;
                bool undefined = false;
                for (size_t pi = 0; pi + 1 < pins.size(); ++pi) {
                    LogicLevel level = LogicStandard::fromVoltage(readVoltage(pins[pi]));
                    inputs.push_back(level);
                    if (level == LogicLevel::UNDEFINED) undefined = true;
                }
                LogicLevel output = LogicLevel::UNDEFINED;
                if (!undefined) {
                    if (comp.name == "NOT Gate") output = inputs[0] == LogicLevel::HIGH ? LogicLevel::LOW : LogicLevel::HIGH;
                    else if (comp.name == "AND Gate" || comp.name == "NAND Gate") {
                        bool high = true;
                        for (LogicLevel input : inputs) high = high && input == LogicLevel::HIGH;
                        output = high ? LogicLevel::HIGH : LogicLevel::LOW;
                        if (comp.name == "NAND Gate") output = output == LogicLevel::HIGH ? LogicLevel::LOW : LogicLevel::HIGH;
                    } else if (comp.name == "OR Gate") {
                        bool high = false;
                        for (LogicLevel input : inputs) high = high || input == LogicLevel::HIGH;
                        output = high ? LogicLevel::HIGH : LogicLevel::LOW;
                    } else if (comp.name == "XOR Gate") {
                        int highCount = 0;
                        for (LogicLevel input : inputs) if (input == LogicLevel::HIGH) highCount++;
                        output = (highCount % 2) ? LogicLevel::HIGH : LogicLevel::LOW;
                    }
                } else {
                    addSimulationWarning(warnings, "Floating input detected. [" + (comp.label.empty() ? comp.name : comp.label) + "]");
                       }
                scheduleOutput(comp, LogicStandard::toVoltage(output), now);
            }

            if (comp.name == "D Flip-Flop" && pins.size() >= 3) {
                ComponentRuntime& runtime = componentRuntime[comp.id];
                LogicLevel d = LogicStandard::fromVoltage(readVoltage(pins[0]));
                LogicLevel clock = LogicStandard::fromVoltage(readVoltage(pins[1]));
                if (d == LogicLevel::UNDEFINED || clock == LogicLevel::UNDEFINED) {
                    addSimulationWarning(warnings, "Floating input detected. [" + (comp.label.empty() ? comp.name : comp.label) + "]");
                    if (clock == LogicLevel::HIGH && runtime.previousClock == LogicLevel::LOW)
                        scheduleOutput(comp, std::numeric_limits<double>::quiet_NaN(), now);
                } else if (runtime.previousClock == LogicLevel::LOW && clock == LogicLevel::HIGH) {
                    scheduleOutput(comp, LogicStandard::toVoltage(d), now);
                }
                if (clock != LogicLevel::UNDEFINED) runtime.previousClock = clock;
            }
        }

        applyPendingOutputs(now);
        // Newly settled outputs are visible to downstream components immediately after their delay.
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            placedComp& comp = placedComponents[ci];
            vector<int>& pins = componentPinNodes[ci];
            if (pins.empty()) continue;
            if (comp.name == "AND Gate" || comp.name == "OR Gate" || comp.name == "NOT Gate" ||
                comp.name == "NAND Gate" || comp.name == "XOR Gate" || comp.name == "D Flip-Flop") {
                drive(pins.back(), componentRuntime[comp.id].outputVoltage,
                      comp.label.empty() ? comp.name : comp.label);
            }
        }
        for (int pass = 0; pass < 6; ++pass) if (!propagatePassive()) break;

        // Cache pin voltages for LEDs, seven-segment displays, pin coloring and inspection.
        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
            placedComp& comp = placedComponents[ci];
            ComponentRuntime& runtime = componentRuntime[comp.id];
            runtime.pinVoltages.clear();
            for (int pin : componentPinNodes[ci]) runtime.pinVoltages.push_back(readVoltage(pin));
            if (comp.name == "LED" && runtime.pinVoltages.size() >= 2) {
                double threshold = parseNamedNumber(comp.value, "threshold", 1.8);
                double forwardVoltage = runtime.pinVoltages[0] - runtime.pinVoltages[1];
                runtime.ledOn = !std::isnan(forwardVoltage) && forwardVoltage >= threshold;
            }
            if (comp.name == "7-Segment") {
                runtime.segments.assign(8, false);
                for (size_t i = 0; i < runtime.pinVoltages.size() && i < 8; ++i)
                    runtime.segments[i] = LogicStandard::fromVoltage(runtime.pinVoltages[i]) == LogicLevel::HIGH;
            }
        }

        wireVoltages.assign(wires.size(), std::numeric_limits<double>::quiet_NaN());
        for (size_t wi = 0; wi < wires.size(); ++wi)
            if (wireNodes[wi] >= 0) wireVoltages[wi] = readVoltage(wireNodes[wi]);

        for (const string& warning : warnings) {
            if (lastSimulationWarnings.find(warning) == lastSimulationWarnings.end()) {
                cerr << warning << endl;
                simulationLog.push_back(warning);
                if (simulationLog.size() > 20) simulationLog.erase(simulationLog.begin());
            }
        }
        lastSimulationWarnings = warnings;
    }

    vector<SDL_Point> gCompPinPos(const placedComp& comp) const {
        vector<SDL_Point> pins;
        vector<LocalPin> localPins = ComponentLibrary::get(comp.name).localPins(comp.inputCount);
        auto transform = [&](int lx, int ly) -> SDL_Point {
            int horizontal = comp.flipH ? -1 : 1;
            int vertical = comp.flipV ? -1 : 1;
            int rx = lx * horizontal;
            int ry = ly * vertical;
            float radians = comp.angle * 3.14159f / 180.0f;
            float sine = std::sin(radians);
            float cosine = std::cos(radians);
            int fx = (int)std::lround(rx * cosine - ry * sine);
            int fy = (int)std::lround(rx * sine + ry * cosine);
            return {comp.x + fx, comp.y + fy};
        };
        for (const LocalPin& pin : localPins) pins.push_back(transform(pin.x, pin.y));
        return pins;
    }

    void drawCompCanvas(const placedComp& comp) const{        
        auto toScreen = [&](int lx, int ly) -> SDL_Point {
            int sx = comp.flipH ? -1 : 1;
            int sy = comp.flipV ? -1 : 1;
            int fx = lx * sx;
            int fy = -ly *sy;
            float rad = comp.angle * 3.14159f / 180.0f;
            float s = sin(rad),c = cos(rad);
            int rx = (int)(fx * c - fy * s);
            int ry = (int)(fx * s + fy * c);
            int wx = comp.x + rx;
            int wy = comp.y + ry;
            return {wldToScrX(wx), wldToScrY(wy)};
        } ;

        if (comp.name == "Resistor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int x = -32, y = 0;
            for (int i=0; i<4; i++){
                auto p1 = toScreen(x, y);
                auto p2 = toScreen(x+8, y-8);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
                x+=8; y-=8;
                p1 = toScreen(x, y);
                p2 = toScreen(x+8, y+8);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
                x+=8; y+=8;
            }
        }
        else if (comp.name == "Capacitor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = toScreen(-10, -15);
            auto p2 = toScreen(-10, 15);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(10, -15);
            p2 = toScreen(10, 15);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }
        else if (comp.name == "Inductor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int x = -27, y = 0;
            for (int i=0; i<5; i++) {
                auto p1 = toScreen(x, y);
                auto p2 = toScreen(x+5, y-7);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
                x+=5; y-=7;
                p1 = toScreen(x, y);
                p2 = toScreen(x+5, y+7);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
                x+=5; y+=7;
            }
        }
        else if (comp.name == "LED") {
            const ComponentRuntime* runtime = findRuntime(comp.id);
            SDL_Color ledColor = {150, 30, 30, 255};
            string colorName = upperCopy(comp.value);
            if (colorName.find("GREEN") != string::npos) ledColor = {30, 190, 60, 255};
            else if (colorName.find("BLUE") != string::npos) ledColor = {40, 100, 220, 255};
            else if (colorName.find("YELLOW") != string::npos) ledColor = {220, 190, 20, 255};
            if (runtime && runtime->ledOn) {
                SDL_Point center = toScreen(0, 0);
                drawFilledCircle(center.x, center.y, 11, ledColor);
            }
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = toScreen(-8, -10);
            auto p2 = toScreen(-8, 10);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-8, -10);  p2 = toScreen(4, 0);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-8, 10);   p2 = toScreen(4, 0);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(4, -10);   p2 = toScreen(4, 10);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(10, -8);   p2 = toScreen(6, -4);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(10, -8);   p2 = toScreen(6, -2);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(10, 8);    p2 = toScreen(6, 4);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(10, 8);    p2 = toScreen(6, 2);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }
        else if (comp.name == "Transistor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = toScreen(0, -10);
            auto p2 = toScreen(0, 10);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-8, -4);  p2 = toScreen(8, -8);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-8, 4);   p2 = toScreen(8, 8);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            int cx = wldToScrX(comp.x), cy = wldToScrY(comp.y);
            SDL_Rect circle = {cx-12, cy-12, 24, 24};
            SDL_RenderDrawRect(renderer, &circle);
        }
        else if (comp.name == "NPN" || comp.name == "PNP") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int cx = wldToScrX(comp.x), cy = wldToScrY(comp.y);
            SDL_Rect circle = {cx-12, cy-12, 24, 24};
            SDL_RenderDrawRect(renderer, &circle);
            auto coll = toScreen(0, -12);
            auto emit = toScreen(0, 12);
            auto base = toScreen(-12, 0);
            auto center = toScreen(0, 0);
            SDL_RenderDrawLine(renderer, coll.x, coll.y, center.x, center.y);
            SDL_RenderDrawLine(renderer, emit.x, emit.y, center.x, center.y);
            SDL_RenderDrawLine(renderer, base.x, base.y, center.x, center.y);
            vector<pair<int,int>> arrow;
            if(comp.name == "NPN") arrow = {{0,8}, {-3,5}, {3,5}};
            else arrow = {{0,-8},{-3,-5},{3,-5}};
            auto tp = toScreen(arrow[0].first, arrow[0].second);
            auto ap1 = toScreen(arrow[1].first, arrow[1].second);
            auto ap2 = toScreen(arrow[2].first, arrow[2].second);
            SDL_RenderDrawLine(renderer, tp.x, tp.y, ap1.x, ap1.y);
            SDL_RenderDrawLine(renderer, ap1.x, ap1.y, ap2.x, ap2.y);
            SDL_RenderDrawLine(renderer, ap2.x, ap2.y, tp.x, tp.y);
        }
        else if (comp.name == "Ground") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = toScreen(0, -10);
            auto p2 = toScreen(0, 0);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-8, 0); p2 = toScreen(8, 0);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-5, 5); p2 = toScreen(5, 5);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            p1 = toScreen(-3, 10); p2 = toScreen(3, 10);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }
        else if (comp.name == "VCC") {
            SDL_SetRenderDrawColor(renderer, 180,20,20,255);
            auto p1 = toScreen(0, -12);
            auto p2 = toScreen(0, 8);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            auto p3 = toScreen(-5, 1); auto p4 = toScreen(5, 1);
            SDL_RenderDrawLine(renderer, p3.x, p3.y, p4.x, p4.y);
            auto p5 = toScreen(-3, -6); auto p6 = toScreen(3, -6);
            SDL_RenderDrawLine(renderer, p5.x, p5.y, p6.x, p6.y);
        }
        else if (comp.name == "DC Voltage Source") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto top = toScreen(0, -18); auto circleTop = toScreen(0, -12);
            auto bottom = toScreen(0, 18); auto circleBottom = toScreen(0, 12);
            SDL_RenderDrawLine(renderer, top.x, top.y, circleTop.x, circleTop.y);
            SDL_RenderDrawLine(renderer, circleBottom.x, circleBottom.y, bottom.x, bottom.y);
            SDL_Point center = toScreen(0,0);
            for (int angle = 0; angle < 360; angle += 15) {
                double a1 = angle * 3.14159 / 180.0, a2 = (angle + 15) * 3.14159 / 180.0;
                SDL_RenderDrawLine(renderer, center.x + (int)(12*cos(a1)), center.y + (int)(12*sin(a1)),
                                  center.x + (int)(12*cos(a2)), center.y + (int)(12*sin(a2)));
            }
            auto h1=toScreen(-4,-5), h2=toScreen(4,-5), v1=toScreen(0,-9), v2=toScreen(0,-1);
            SDL_RenderDrawLine(renderer,h1.x,h1.y,h2.x,h2.y); SDL_RenderDrawLine(renderer,v1.x,v1.y,v2.x,v2.y);
            h1=toScreen(-4,6); h2=toScreen(4,6); SDL_RenderDrawLine(renderer,h1.x,h1.y,h2.x,h2.y);
        }
        else if (comp.name == "Clock Generator") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto a=toScreen(-22,8), b=toScreen(-12,8), c=toScreen(-12,-8), d=toScreen(0,-8);
            auto e=toScreen(0,8), f=toScreen(12,8), g=toScreen(12,-8), h=toScreen(20,-8), o=toScreen(24,0);
            SDL_RenderDrawLine(renderer,a.x,a.y,b.x,b.y); SDL_RenderDrawLine(renderer,b.x,b.y,c.x,c.y);
            SDL_RenderDrawLine(renderer,c.x,c.y,d.x,d.y); SDL_RenderDrawLine(renderer,d.x,d.y,e.x,e.y);
            SDL_RenderDrawLine(renderer,e.x,e.y,f.x,f.y); SDL_RenderDrawLine(renderer,f.x,f.y,g.x,g.y);
            SDL_RenderDrawLine(renderer,g.x,g.y,h.x,h.y); SDL_RenderDrawLine(renderer,h.x,h.y,o.x,o.y);
        }
        else if (comp.name == "Switch") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto left=toScreen(-28,0), leftContact=toScreen(-12,0), rightContact=toScreen(12,0), right=toScreen(28,0);
            SDL_RenderDrawLine(renderer,left.x,left.y,leftContact.x,leftContact.y);
            SDL_RenderDrawLine(renderer,rightContact.x,rightContact.y,right.x,right.y);
            auto armEnd = comp.switchState ? rightContact : toScreen(10,-12);
            SDL_RenderDrawLine(renderer,leftContact.x,leftContact.y,armEnd.x,armEnd.y);
            drawFilledCircle(leftContact.x,leftContact.y,3,{0,0,0,255});
            drawFilledCircle(rightContact.x,rightContact.y,3,{0,0,0,255});
        }
        else if (comp.name == "Push Button") {
            const ComponentRuntime* runtime = findRuntime(comp.id);
            bool pressed = runtime && runtime->pressed;
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto left=toScreen(-28,0), lc=toScreen(-12,0), rc=toScreen(12,0), right=toScreen(28,0);
            SDL_RenderDrawLine(renderer,left.x,left.y,lc.x,lc.y); SDL_RenderDrawLine(renderer,rc.x,rc.y,right.x,right.y);
            auto bridge1=toScreen(-10, pressed ? 0 : -9), bridge2=toScreen(10, pressed ? 0 : -9);
            SDL_RenderDrawLine(renderer,bridge1.x,bridge1.y,bridge2.x,bridge2.y);
            auto stem1=toScreen(0,-18), stem2=toScreen(0,pressed ? -9 : -13);
            SDL_RenderDrawLine(renderer,stem1.x,stem1.y,stem2.x,stem2.y);
            drawFilledCircle(lc.x,lc.y,3,{0,0,0,255}); drawFilledCircle(rc.x,rc.y,3,{0,0,0,255});
        }
        else if (comp.name == "Battery") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = toScreen(0, -12);
            auto p2 = toScreen(0, -4);
            SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            auto p3 = toScreen(-4, -4); auto p4 = toScreen(4, -4);
            SDL_RenderDrawLine(renderer, p3.x, p3.y, p4.x, p4.y);
            auto p5 = toScreen(-8, 1); auto p6 = toScreen(8, 1);
            SDL_RenderDrawLine(renderer, p5.x, p5.y, p6.x, p6.y);
            auto p7 = toScreen(0, 1); auto p8 = toScreen(0, 10);
            SDL_RenderDrawLine(renderer, p7.x, p7.y, p8.x, p8.y);
        }
        else if (comp.name == "AND Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            vector<pair<int,int>> points = {
                {-20,-12}, {-20,12},{-20,-12}, {6,-12}, {-20,12}, {6,12}, {6,-12}, {10,-8},
                {10,-8}, {12,-4},{12,-4}, {12,4},{12,4}, {10,8}, {10,8}, {6,12},
                {12,0},{18,0}, {-26,-8}, {-20,-8},{-26,8}, {-20,8}
            };
            for (size_t i=0; i<points.size(); i+=2) {
                auto p1 = toScreen(points[i].first, points[i].second);
                auto p2 = toScreen(points[i+1].first, points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
            vector<LocalPin> local = ComponentLibrary::get(comp.name).localPins(comp.inputCount);
            for (size_t i = 0; i + 1 < local.size(); ++i) {
                auto outer = toScreen(local[i].x, local[i].y);
                auto inner = toScreen(-20, local[i].y);
                SDL_RenderDrawLine(renderer, outer.x, outer.y, inner.x, inner.y);
            }
            auto outInner = toScreen(12,0), outOuter = toScreen(local.back().x,0);
            SDL_RenderDrawLine(renderer,outInner.x,outInner.y,outOuter.x,outOuter.y);
        }
        else if (comp.name == "OR Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            vector<pair<int,int>> points = {
                {-12,-12}, {6,-12},{-12,12}, {6,12}, {-12,-12}, {-8,-10},{-8,-10}, {-6,-4},
                {-6,-4}, {-6,4},{-6,4}, {-8,10}, {-8,10}, {-12,12}, {6,-12}, {10,-8},
                {10,-8}, {12,-4},{12,-4}, {12,4},{12,4}, {10,8},{10,8}, {6,12},
                {12,0}, {18,0},{-22,-8}, {-12,-8}, {-22,8}, {-12,8}
            };
            for (size_t i=0; i<points.size(); i+=2) {
                auto p1 = toScreen(points[i].first, points[i].second);
                auto p2 = toScreen(points[i+1].first, points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
            vector<LocalPin> local = ComponentLibrary::get(comp.name).localPins(comp.inputCount);
            for (size_t i = 0; i + 1 < local.size(); ++i) {
                auto outer = toScreen(local[i].x, local[i].y);
                auto inner = toScreen(-12, local[i].y);
                SDL_RenderDrawLine(renderer, outer.x, outer.y, inner.x, inner.y);
            }
            auto outInner = toScreen(12,0), outOuter = toScreen(local.back().x,0);
            SDL_RenderDrawLine(renderer,outInner.x,outInner.y,outOuter.x,outOuter.y);
        }
        else if (comp.name == "NOT Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            vector<pair<int,int>> points = {
                {-12,-12}, {-12,12},{-12,-12}, {8,0}, {-12,12}, {8,0}, {-18,0}, {-12,0}
            };
            for (size_t i=0; i<points.size(); i+=2) {
                auto p1 = toScreen(points[i].first, points[i].second);
                auto p2 = toScreen(points[i+1].first, points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
            auto bubbleCenter = toScreen(12, 0);
            int r =3;
            for (int a=0; a<360; a+=30) {
                float a1 = a*3.14159f/180.0f, a2 = (a+30)*3.14159f/180.0f;
                int x1 = bubbleCenter.x + (int)(r*cos(a1)), y1 = bubbleCenter.y + (int)(r*sin(a1));
                int x2 = bubbleCenter.x + (int)(r*cos(a2)), y2 = bubbleCenter.y + (int)(r*sin(a2));
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
            auto pOut1 = toScreen(15, 0);
            auto pOut2 = toScreen(22, 0);
            SDL_RenderDrawLine(renderer, pOut1.x, pOut1.y, pOut2.x, pOut2.y);
        }
        else if (comp.name == "NAND Gate" || comp.name == "XOR Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            bool nand = comp.name == "NAND Gate";
            SDL_Rect body = {wldToScrX(comp.x)-18, wldToScrY(comp.y)-14, 36, 28};
            SDL_RenderDrawRect(renderer, &body);
            drawTxt(nand ? "NAND" : "XOR", body.x+2, body.y+6, {0,0,0,255}, libFont);
            vector<SDL_Point> pins = gCompPinPos(comp);
            for (size_t i=0;i+1<pins.size();++i) {
                auto edge=toScreen(-18, ComponentLibrary::get(comp.name).localPins(comp.inputCount)[i].y);
                SDL_RenderDrawLine(renderer,wldToScrX(pins[i].x),wldToScrY(pins[i].y),edge.x,edge.y);
            }
            if (!pins.empty()) {
                auto edge=toScreen(18,0);
                SDL_RenderDrawLine(renderer,edge.x,edge.y,wldToScrX(pins.back().x),wldToScrY(pins.back().y));
            }
        }
        else if (comp.name == "D Flip-Flop") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto tl=toScreen(-20,-18), br=toScreen(20,18);
            SDL_Rect box={std::min(tl.x,br.x),std::min(tl.y,br.y),std::abs(br.x-tl.x),std::abs(br.y-tl.y)};
            SDL_RenderDrawRect(renderer,&box);
            drawTxt("D", box.x+3, box.y+1, {0,0,0,255}, libFont);
            drawTxt("CLK", box.x+3, box.y+20, {0,0,0,255}, libFont);
            drawTxt("Q", box.x+box.w-14, box.y+10, {0,0,0,255}, libFont);
            auto d1=toScreen(-30,-10), d2=toScreen(-20,-10); SDL_RenderDrawLine(renderer,d1.x,d1.y,d2.x,d2.y);
            auto c1=toScreen(-30,10), c2=toScreen(-20,10); SDL_RenderDrawLine(renderer,c1.x,c1.y,c2.x,c2.y);
            auto q1=toScreen(20,0), q2=toScreen(30,0); SDL_RenderDrawLine(renderer,q1.x,q1.y,q2.x,q2.y);
        }
        else if (comp.name == "7-Segment") {
            const ComponentRuntime* runtime = findRuntime(comp.id);
            vector<bool> on(8, false);
            if (runtime) on = runtime->segments;
            auto segment = [&](int index, int x1, int y1, int x2, int y2) {
                SDL_Color color = index < (int)on.size() && on[index] ? SDL_Color{235,25,25,255} : SDL_Color{75,75,75,255};
                SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
                auto p1=toScreen(x1,y1), p2=toScreen(x2,y2);
                for (int offset=-1; offset<=1; ++offset)
                    SDL_RenderDrawLine(renderer,p1.x+offset,p1.y,p2.x+offset,p2.y);
            };
            segment(0,-11,-13,11,-13); segment(1,-12,-11,-12,-2); segment(2,12,-11,12,-2);
            segment(3,-11,0,11,0); segment(4,-12,2,-12,11); segment(5,12,2,12,11);
            segment(6,-11,13,11,13);
            SDL_Color dotColor = on.size()>7 && on[7] ? SDL_Color{235,25,25,255} : SDL_Color{75,75,75,255};
            auto dot=toScreen(17,13); drawFilledCircle(dot.x,dot.y,2,dotColor);
        }
        else {
            drawTxt(comp.name.substr(0,4), wldToScrX(comp.x)-20, wldToScrY(comp.y)-10, {0,0,0,255});
        }
    }

    void drawCompPreviewLib(const string& compName, SDL_Rect area) const {
        SDL_SetRenderDrawColor(renderer, 245,245,245,255);
        SDL_RenderFillRect(renderer, &area);
        SDL_SetRenderDrawColor(renderer, 100,100,100,255);
        SDL_RenderDrawRect(renderer, &area);

        int cx = area.x + area.w / 2, cy = area.y + area.h/ 2;
        auto rotatePoint = [&](int px, int py, float rad) -> pair<int,int> {
            float s = std::sin(rad) , c = std::cos(rad);
            int dx = px - cx, dy = py - cy;
            int rx = (int)(dx * c - dy * s ) + cx;
            int ry = (int)(dx * s + dy * c ) + cy;
            return {rx, ry};
        };
        float rad = 0.0f;
        int sx = 1, sy = 1;
        auto flipPoint = [&](int px, int py) -> pair<int,int> {
            int dx = px - cx, dy = py - cy ;
            return {cx + dx * sx, cy + dy * sy};
        };
        auto transform = [&] (int x, int y) -> pair<int,int> {
            auto [fx, fy] = flipPoint(x, y);
            return rotatePoint(fx, fy,rad);
        };

        if (compName == "Resistor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int totalW = min(60, area.w- 10);
            int startX = area.x + (area.w - totalW) / 2;
            int x = startX, y = cy;
            for (int i=0; i<4; i++){
                auto p1 = transform(x, y);
                auto p2 = transform(x+8, y-8);
                x+=8;y-=8;
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
                p1 = transform(x, y);
                p2 = transform(x+8, y+8);
                x+=8; y+=8;
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first,  p2.second);
            }
        }
        else if (compName == "Capacitor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = transform (cx-10, cy-15);
            auto p2 = transform (cx-10, cy+15);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx+10, cy-15);
            p2 = transform(cx+10, cy+15);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
        }
        else if (compName == "Inductor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int totalW = min (50, area.w - 10);
            int startX = area.x + (area.w - totalW) /2;
            int x = startX, y = cy;
            for (int i=0; i< 5; i++) {
                auto p1 = transform(x, y);
                auto p2 = transform(x+5, y-7);
                x+=5; y-=7 ;
                SDL_RenderDrawLine(renderer, p1.first, p1.second,p2.first, p2.second);
                p1 = transform (x, y);
                p2 = transform (x+5, y+7);
                x+=5; y+=7;
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            }
        }
        else if (compName == "LED") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 =transform(cx-8, cy-10);
            auto p2 =transform(cx-8, cy+10);
            SDL_RenderDrawLine (renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-8, cy-10);
            p2 = transform(cx+4, cy);
            SDL_RenderDrawLine(renderer,p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-8, cy+10);
            p2 = transform(cx+4, cy) ;
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx +4, cy-10);
            p2 = transform(cx+4, cy+10);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx+10,cy-8);
            p2 = transform(cx+6, cy-4);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx+10,  cy-8);
            p2 =transform(cx+6, cy-2);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx+10, cy+8);
            p2 = transform(cx+6, cy+4);
            SDL_RenderDrawLine (renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx+10, cy+8);
            p2 = transform(cx+6, cy+2);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
        }
        else if (compName ==  "Transistor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1= transform(cx, cy-10);
            auto p2= transform(cx, cy+10);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-8, cy-4 );
            p2 = transform(cx+8, cy-8);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-8, cy+4);
            p2 = transform(cx+8, cy+8);
            SDL_RenderDrawLine(renderer, p1.first,p1.second, p2.first, p2.second);
            SDL_Rect circle ={cx-12, cy-12, 24,24};
            SDL_RenderDrawRect(renderer, &circle);
        }
        else if (compName == "NPN"|| compName == "PNP") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_Rect circle = {cx-12, cy-12, 24, 24} ;
            SDL_RenderDrawRect(renderer, &circle);

            auto coll  = transform(cx, cy-12);
            auto emit  = transform(cx, cy+12);
            auto base  = transform(cx-12, cy);
            auto center= transform(cx, cy);
            SDL_RenderDrawLine(renderer, coll.first, coll.second, center.first, center.second);
            SDL_RenderDrawLine(renderer, emit.first, emit.second, center.first, center.second);
            SDL_RenderDrawLine(renderer, base.first, base.second, center.first, center.second);

            vector<pair<int,int>> arrow;
            if(compName == "NPN") {
                arrow = {{0,8}, {-3,5}, {3,5}};
            }
            else {
                arrow = {{0,-8},{-3,-5},{3,-5}};
            }
            auto tp  = transform(cx + arrow[0].first, cy + arrow[0].second);
            auto ap1 = transform(cx + arrow[1].first, cy + arrow [1].second);
            auto ap2 = transform(cx + arrow[2].first, cy + arrow[2].second);
            SDL_RenderDrawLine(renderer, tp.first, tp.second, ap1.first, ap1.second);
            SDL_RenderDrawLine(renderer, ap1.first, ap1.second, ap2.first, ap2.second);
            SDL_RenderDrawLine (renderer, ap2.first, ap2.second, tp.first, tp.second);
        }
        else if (compName == "Ground" ) {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = transform(cx, cy-10);
            auto p2 = transform(cx, cy);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2. first, p2.second);
            p1 = transform(cx-8, cy);
            p2  = transform(cx+8, cy);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-5, cy+5);
            p2 = transform(cx+5, cy+5) ;
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            p1 = transform(cx-3, cy+10);
            p2 = transform(cx+3,cy+10);
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
        }
        else if ( compName == "VCC") {
            SDL_SetRenderDrawColor( renderer, 180,20,20,255);
            auto p1 = transform(cx, cy-12);
            auto p2 = transform(cx, cy+8) ;
            SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            auto p3 = transform (cx-5, cy+1);
            auto p4 = transform(cx+5, cy+1);
            SDL_RenderDrawLine(renderer , p3.first, p3.second, p4.first, p4.second);
            auto p5 = transform(cx-3, cy-6);
            auto p6 = transform(cx+3, cy-6);
            SDL_RenderDrawLine(renderer, p5.first, p5.second, p6.first, p6.second);
        }
        else if (compName == "DC Voltage Source") {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_Rect circle={cx-12,cy-12,24,24}; SDL_RenderDrawRect(renderer,&circle);
            SDL_RenderDrawLine(renderer,cx,cy-18,cx,cy-12); SDL_RenderDrawLine(renderer,cx,cy+12,cx,cy+18);
            SDL_RenderDrawLine(renderer,cx-4,cy-5,cx+4,cy-5); SDL_RenderDrawLine(renderer,cx,cy-9,cx,cy-1);
            SDL_RenderDrawLine(renderer,cx-4,cy+6,cx+4,cy+6);
        }
        else if (compName == "Clock Generator") {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            int pts[][2]={{cx-25,cy+8},{cx-15,cy+8},{cx-15,cy-8},{cx,cy-8},{cx,cy+8},{cx+15,cy+8},{cx+15,cy-8},{cx+25,cy-8}};
            for(int i=0;i<7;++i) SDL_RenderDrawLine(renderer,pts[i][0],pts[i][1],pts[i+1][0],pts[i+1][1]);
        }
        else if (compName == "Switch" || compName == "Push Button") {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_RenderDrawLine(renderer,cx-28,cy,cx-12,cy); SDL_RenderDrawLine(renderer,cx+12,cy,cx+28,cy);
            SDL_RenderDrawLine(renderer,cx-12,cy,cx+10,cy-11);
            if (compName == "Push Button") SDL_RenderDrawLine(renderer,cx,cy-20,cx,cy-11);
        }
        else if (compName == "Battery"){
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto p1 = transform(cx, cy-12);
            auto p2 = transform(cx, cy-4);
            SDL_RenderDrawLine (renderer, p1.first, p1.second, p2.first, p2.second);
            auto p3 = transform(cx-4, cy-4);
            auto p4 = transform(cx+4, cy-4);
            SDL_RenderDrawLine(renderer, p3.first, p3.second , p4.first, p4.second);
            auto p5 = transform(cx-8, cy+1);
            auto p6 = transform(cx+8, cy+1);
            SDL_RenderDrawLine (renderer, p5.first, p5.second, p6.first, p6.second);
            auto p7 = transform(cx, cy+1);
            auto p8 = transform(cx, cy+10);
            SDL_RenderDrawLine(renderer, p7.first, p7.second, p8.first, p8.second);
        }
        else if (compName =="AND Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            vector<pair <int,int>> points = {
                {-20,-12}, {-20,12},{-20,-12}, {6,-12}, {-20,12}, {6,12}, {6,-12}, {10,-8}, {10,-8}, {12,-4},
                {12,-4}, {12,4},{12,4}, {10,8}, {10,8}, {6,12}, {12,0},{18 ,0}, {-26,-8}, {-20,-8},
                {-26,8}, {-20,8}
            };
            for (size_t i= 0; i < points.size(); i+=2) {
                auto p1 = transform(cx + points[i].first, cy + points[i].second);
                auto p2 = transform(cx + points[i+1].first, cy+ points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            }
        }
        else if(compName == "OR Gate") {
            SDL_SetRenderDrawColor(renderer, 0 ,0,0,255);
            vector<pair<int,int>> points = {
                {-12,-12}, {6,-12},{-12,12}, {6,12}, {-12,-12}, {-8,-10},{-8,-10}, {-6,-4}, {-6,-4}, {-6,4},
                {-6,4}, {-8,10}, {-8,10}, {-12,12}, {6,-12}, {10,-8}, {10,-8}, {12,-4}, {12,-4}, {12,4},
                {12,4}, {10,8},{10,8}, {6,12},{12,0}, {18,0},{-22,-8}, {-12,-8}, {-22,8}, {-12,8}
            };
            for (size_t i = 0; i < points.size(); i+=2) {
                auto p1 = transform(cx + points[i].first, cy + points[i].second);
                auto p2 = transform(cx + points[i+1].first, cy + points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first , p2.second);
            }
        }
        else if(compName == "NOT Gate") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            vector <pair<int,int>> points = {
                {-12,-12}, {-12,12},{-12,-12}, {8,0}, {-12,12}, {8,0}, {-18,0}, {-12,0}
            };
            for (size_t i = 0; i < points.size(); i+=2) {
                auto p1 = transform(cx + points[i].first, cy + points[i].second);
                auto p2 = transform(cx + points [i+1].first, cy + points[i+1].second);
                SDL_RenderDrawLine(renderer, p1.first, p1.second, p2.first, p2.second);
            }

            auto bubbleCenter = transform(cx + 12, cy);
            int r =3;
            for (int a = 0; a < 360; a += 30) {
                float angle1 = a * 3.14159f / 180.0f;
                float angle2 = (a + 30)* 3.14159f / 180.0f;
                int x1 = bubbleCenter.first + (int)(r * cos(angle1));
                int y1 = bubbleCenter.second + (int)(r * sin(angle1));
                int x2 = bubbleCenter.first + (int)(r *  cos(angle2));
                int y2 = bubbleCenter.second + (int)(r * sin(angle2));
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
            }
            auto pOut1 =transform(cx + 15, cy);
            auto pOut2 =transform(cx + 22, cy);
            SDL_RenderDrawLine(renderer, pOut1.first, pOut1.second , pOut2.first, pOut2.second);
        }
        else if (compName == "NAND Gate" || compName == "XOR Gate" || compName == "D Flip-Flop") {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_Rect box={cx-24,cy-15,48,30}; SDL_RenderDrawRect(renderer,&box);
            string text = compName == "D Flip-Flop" ? "DFF" : (compName == "NAND Gate" ? "NAND" : "XOR");
            drawTxt(text,cx-18,cy-8,{0,0,0,255},libFont);
        }
        else if (compName == "7-Segment") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto topL   = transform(cx-12, cy-10);
            auto topR   = transform(cx+12, cy-10);
            auto mid    = transform(cx, cy);
            auto botL   = transform(cx-12, cy+10);
            auto botR   = transform(cx+12, cy+10);
            auto upL1   = transform(cx-12, cy-10);
            auto upL2   = transform(cx-12, cy-2);
            auto upR1   = transform(cx+12, cy-10);
            auto upR2   = transform(cx+12 , cy -2);
            auto lowL1  = transform(cx-12, cy+2);
            auto lowL2  = transform(cx-12, cy+10);
            auto lowR1  = transform(cx+12, cy+2) ;
            auto lowR2  = transform(cx+12, cy+10);

            SDL_RenderDrawLine(renderer, topL.first, topL.second, topR.first, topR.second);
            SDL_RenderDrawLine(renderer, upL1.first,upL1.second, upL2.first, upL2.second);
            SDL_RenderDrawLine(renderer, upR1.first, upR1.second, upR2.first, upR2.second);
            SDL_RenderDrawLine(renderer, mid.first-10, mid.second, mid.first+10, mid.second);
            SDL_RenderDrawLine(renderer, lowL1.first, lowL1.second, lowL2.first, lowL2.second);
            SDL_RenderDrawLine(renderer, lowR1.first, lowR1.second, lowR2.first, lowR2.second);
            SDL_RenderDrawLine(renderer, botL.first, botL.second, botR.first , botR.second);
        }
        else {
            drawTxt(compName.substr(0,4), area.x+5, area.y+5, {0,0,0,255});
        }
    }

    string toLower(const string& s) const {
        string lower = s;
        transform (lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    void resetLibExpanded() {
        for (auto& cat : libCategories) cat.expanded = false;
        searchFilter = "" ;
        showLib = false;
        showProp= false;
        undoStack.clear();
        redoStack.clear();
        selectedIndices.clear ();
        editingIndex = -1;
        lastClickedIndex = -1;
        lastClickTime =0;
        if (searchBox)
            searchBox->setTxt( "Search...");
    }

    string fileDialog() {
        char filename [MAX_PATH ] = "";

        OPENFILENAMEA ofn;
        ZeroMemory( &ofn, sizeof( ofn ) );
        ofn.lStructSize = sizeof(ofn );
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = "Proteus Project Files\0*.proj \0All Files\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile =MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "proj";

        if (GetOpenFileNameA( &ofn )) {
            return string( filename );
        }
        return "";
    }

    string projNameFromPath( const string& path ) {
        size_t pos = path.find_last_of( "\\/" );
        string name = (pos != string::npos ) ? path.substr( pos + 1 ) : path;
        pos = name.find_last_of('.');
        if (pos !=string::npos)
            name = name.substr( 0,pos );
        return name;
    }

    string diffName (const string& base) {
        string candidate = base;
        int counter= 1;
        bool exists = true;
        while (exists) {
            exists = false;
            for (const auto& p: recentPs) {
                if (p.name == candidate) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                candidate =base + " " + to_string(counter);
                counter++;
            }
        }
        return candidate;
    }

    string joinStrings(const vector <string>& vec) const {
        string result;
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += ",";
            result +=vec[i];
        }
        return result;
    }

    void loadRecentPs() {
        ifstream file("recents.txt");
        if (file.is_open( )) {
            string line;
            while (getline(file, line)) {
                if (line.empty())
                    continue;
                stringstream ss(line);
                string n, p, d;
                int cw = 800, ch = 600;
                getline( ss, n, '|');
                getline(ss, p, '|');
                getline(ss, d, '|');
                ss >> cw >> ch;
                vector<string> ac;
                if (ss.peek() == '|') {
                    ss.ignore ();
                    string compList;
                    getline(ss, compList);
                    stringstream cs(compList);
                    string comp;
                    while (getline(cs, comp, ',')) {
                        if (!comp.empty()) ac.push_back(comp);
                    }
                }
                if (!n.empty() && !p.empty() && !d.empty())
                    recentPs.push_back(project(n, p, d, cw, ch, ac));
            }
            file.close();
        }

        int limit= min((int)recentPs.size(), 5);
        for (int i = 0; i< limit; i++) {
            string displayText = recentPs [i].name + "  (" +recentPs[i].lastP + ")";
            btnRecents.push_back (new BUTTONS( renderer, font, 450, 190 + (i *65), 340, 50,{230, 230,230, 255}, {200, 230, 255, 255},
                displayText));
        }
    }

    void saveRecentPs() {
        ofstream file("recents.txt");
        if ( !file.is_open())
            return;
        for (const auto& p :recentPs) {
            file << p.name << "|" <<p.path << "|" << p.lastP << "|" <<p.canvasW << "|" << p.canvasH;
            if (!p.activeComponents.empty ()) {
                file << "|" << joinStrings(p.activeComponents);
            }
            file << "\n";
        }
        file.close();
    }

    void addToRecent(const project& prj) {
        recentPs.erase (remove_if(recentPs.begin(), recentPs.end() ,[&] (const project& p ) { return p.path == prj.path;}), recentPs.end());

        recentPs.insert(recentPs.begin(), prj);

        while (recentPs.size()> 5) {
            recentPs.pop_back();
        }

        saveRecentPs();

        for (auto btn : btnRecents)
            delete btn;
        btnRecents.clear ();

        int limit = min((int)recentPs.size(), 5);
        for (int i = 0; i < limit;i++) {
            string displayText = recentPs[i].name + "  (" + recentPs [i].lastP + ")";
            btnRecents.push_back(new BUTTONS (renderer, font, 450, 190 +(i * 65), 340, 50,{230, 230, 230, 255}, {200, 230, 255, 255},
                displayText));
        }
    }

    void clearRecents() {
        recentPs.clear();
        saveRecentPs ();
        for (auto btn : btnRecents)
            delete btn ;
        btnRecents.clear();
    }

    string gDate() {
        time_t now = time( 0);
        tm* ltm = localtime(&now);
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d/%02d/%02d", 1900 + ltm->tm_year, 1 + ltm-> tm_mon, ltm->tm_mday);
        return string(buf);
    }

    void openNameDialog () {
        if (!txtProjectName) {
            txtProjectName = new txtIn(renderer,font, 275, 240, 300, 35, false);
            btnNameOK =new BUTTONS(renderer, font, 275, 340, 120, 40,{255, 255, 255, 255}, {240, 240, 240, 255}, "OK");
            btnNameCancel = new BUTTONS(renderer, font, 455, 340, 120,40,{255, 255, 255, 255}, {240, 240, 240, 255}, "Cancel");
        }
        txtProjectName-> setTxt(penProjectName);
        txtProjectName->setActive(true);
        currentState = app::PROJECT_NAME_DIALOG;
    }

    void saveProjectToFile(const string& path){
        ofstream file(path);
        if (!file.is_open() )
            return;
        file << canvasWidth << " " << canvasHeight << "\n";
        file << joinStrings(activeComps) << "\n";
        file << placedComponents. size() << "\n";
        for (const auto& pc : placedComponents) {
            file << pc.name << "|" << pc.x << "|" << pc.y << "|"<< pc.angle << "|" << pc.flipH << "|" << pc.flipV << "|" << pc.label << "|" << pc.value
                 << "|" << pc.id << "|" << pc.inputCount << "|" << pc.propagationDelayMs << "|" << pc.switchState << "\n";
        }
        file << wires.size() << "\n";
        for (const auto& wire : wires) {
            file << wire.size();
            for (const auto& pt : wire) {
                file << " " << pt.x << " " << pt.y;
            }
            file << "\n";
        }
        file << junctions.size() << "\n" ;
        for (const auto& j : junctions) {
            file << j.x << " " << j.y << "\n";
        }

        file.close();
    }

    bool loadProjectFromFile(const string& path) {
        ifstream file (path);
        if (!file.is_open())
            return false;
        file >> canvasWidth >> canvasHeight;
        file.ignore();
        string line;
        if (getline(file, line)) {
            activeComps.clear();
            stringstream ss(line);
            string comp;
            while (getline(ss, comp, ',')) {
                if (!comp.empty()) activeComps.push_back(comp);
            }
        }
        placedComponents.clear();
        componentRuntime.clear();
        nextComponentId = 1;
        int count = 0;
        if (file >> count) {
            file.ignore();
            for (int i = 0; i < count; ++i) {
                if (!getline(file, line) ) break;
                stringstream ss (line);
                string token;
                placedComp pc;
                if ( getline(ss, token, '|')) pc.name = token;
                if ( getline(ss, token, '|')) pc.x = stoi(token);
                if ( getline(ss, token, '|')) pc.y = stoi(token);
                if ( getline(ss, token, '|')) pc.angle = stoi(token );
                if ( getline(ss, token, '|')) pc.flipH = (token == "1");
                if ( getline(ss, token, '|')) pc.flipV = (token == "1");
                if ( getline(ss, token, '|')) pc.label =token;
                if ( getline(ss, token, '|')) pc.value =token;
                if ( getline(ss, token, '|') && !token.empty()) pc.id = stoi(token);
                if ( getline(ss, token, '|') && !token.empty()) pc.inputCount = stoi(token);
                if ( getline(ss, token, '|') && !token.empty()) pc.propagationDelayMs = stod(token);
                if ( getline(ss, token, '|') && !token.empty()) pc.switchState = (token == "1");
                clampToCanvas (pc.x, pc.y);
                initializeComponent(pc);
                placedComponents.push_back(pc);
            }
        }
        wires.clear();
        int wireCount = 0;
        if (file >> wireCount) {
            file.ignore();
            for (int i = 0; i < wireCount; ++i) {
                if (!getline(file, line)) break;
                stringstream ss(line);
                int ptCount;
                ss >> ptCount;
                vector<SDL_Point> wire;
                for (int j = 0; j < ptCount; ++j) {
                    SDL_Point pt;
                    ss >> pt.x >> pt.y;
                    wire.push_back(pt);
                }
                wires.push_back(wire);
            }
        }
        junctions.clear();
        int junctionCount =  0;
        if (file >> junctionCount) {
            file.ignore ();
            for (int i = 0; i < junctionCount; ++i) {
                if (!getline(file, line)) break;
                stringstream ss (line);
                SDL_Point pt;
                ss >> pt.x >> pt.y;
                junctions.push_back(pt);
            }
        }
        file.close();
        selectedIndices .clear();
        editingIndex = -1;
        lastClickedIndex = -1;
        lastClickTime = 0;
        updateWorldBounds ();
        return true ;
    }

public:
    PROTEUS() {
        window = nullptr;
        renderer = nullptr;
        font = nullptr;
        titleFont =nullptr;
        libFont = nullptr;
        running = false;
        currentState = app::STARTUP_MENU;
        canvasWidth = 800;
        canvasHeight =600;
        updateWorldBounds();
        txtWidth = nullptr;
        txtHeight = nullptr;
        btnCustomOK= nullptr;
        btnCustomCancel = nullptr;
        btnPresetCustom = nullptr;
        txtProjectName =nullptr;
        btnNameOK = nullptr;
        btnNameCancel = nullptr;
        btnBack = nullptr;
        btnRemRecents = nullptr ;
        penProjectName =  "Untitled";
        grdSz = 20;
        zmLvl =1.0f;
        panX = 0;
        panY = 0;
        isPan = false;
        statH = 30 ;
        winW = 850;winH = 600;
        mseX = 0 ; mseY = 0;
        zoomRct = {0,0,0,0} ;
        showGrid = true ;
        currentTool = Tool::SELECT;
        tlbrH = 40 ;
        pnlLW = 220 ; pnlRW = 140;
        vpX = pnlLW; vpY = tlbrH;
        vpW = winW - pnlLW - pnlRW;
        vpH = winH - tlbrH - statH ;
        selLibItm = "" ;
        showLib = false;
        showProp = false;
        activeCompsY = 0;
        currentProjectPath = "";
        draggingComponents =false;
        drawingSelection = false;
        selectionRect = {0,0,0,0};
        propLabelInput = nullptr;
        propValueInput = nullptr;
        btnPropOK =nullptr;
        btnPropCancel = nullptr;
        editingIndex = -1;
        lastClickedIndex = -1;
        lastClickTime = 0;
        mouseHandled= false;
        wireStartActive = false;
        hoveredPinActive = false;
        nextComponentId = 1;
        wireVoltages.clear();
        simulationLog.clear();
        lastSimulationWarnings.clear();

        libItms = {"Resistor","Capacitor","LED","Transistor","Ground","VCC"};
        for (size_t i=0; i<libItms.size(); i++) {
            libRcts.push_back({5, tlbrH + 55 + (int)i*40, pnlLW-10, 28});
        }

        searchBox = nullptr;
        searchFilter ="";
        showLib = true;

        libCategories.push_back(tree("Sources", {"Ground","VCC","DC Voltage Source","Battery","Clock Generator"}));
        libCategories.push_back(tree("Passive", {"Resistor","Capacitor","Inductor"}));
        libCategories.push_back(tree("Interactive", {"Switch","Push Button"}));
        libCategories.push_back(tree("Digital Logic", {"AND Gate","OR Gate","NOT Gate","NAND Gate","XOR Gate","D Flip-Flop"}));
        libCategories.push_back(tree("Display", {"LED","7-Segment"}));
        libCategories.push_back(tree("Transistor", {"NPN","PNP"}));
    }

    ~PROTEUS() {
        delete btnNewP;
        delete btnOpenP;
        delete btnRemRecents;
        delete btnPresetA4;
        delete btnPresetA3;
        delete btnCancelDialog;
        delete btnPresetCustom;
        delete txtWidth;
        delete txtHeight ;
        delete btnCustomOK;
        delete btnCustomCancel;
        delete txtProjectName;
        delete btnNameOK;
        delete btnNameCancel;
        delete btnBack;
        delete searchBox;
        delete propLabelInput ;
        delete propValueInput;
        delete btnPropOK;
        delete btnPropCancel;
        if (libFont)
            TTF_CloseFont(libFont);
        for (auto btn :btnRecents)
            delete btn;
        for (auto btn : tlbrBtns)
            delete btn;
        if (titleFont)
            TTF_CloseFont(titleFont);
        if (font)
            TTF_CloseFont(font);
        if (renderer )
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);

        TTF_Quit();
        SDL_Quit();
    }

    bool initial() {
        SDL_SetMainReady();
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr <<"SDL Init failed: " <<SDL_GetError() << endl;
            return false;
        }
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        if (TTF_Init() ==-1){
            cerr << "SDL_ttf Init failed: " << TTF_GetError() << endl;
            return false;
        }
        window =SDL_CreateWindow("Proteus Clone - Startup Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 850, 600, SDL_WINDOW_SHOWN| SDL_WINDOW_RESIZABLE);
        if (!window)
            return false;
        renderer =SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            cerr << "Accelerated renderer unavailable; using software renderer. " << SDL_GetError() << endl;
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
        if (!renderer) {
            cerr << "Renderer creation failed: " << SDL_GetError() << endl;
            return false;
        }

        font = openApplicationFont(20);
        titleFont = openApplicationFont(30);
        libFont = openApplicationFont(14);
        if (!font || !titleFont || !libFont) {
            const char* message = "No usable Windows font was found. Expected Arial, Segoe UI, or Tahoma in C:\\Windows\\Fonts.";
            cerr << message << endl;
            MessageBoxA(nullptr, message, "Proteus Clone - Font Error", MB_OK | MB_ICONERROR);
            return false;
        }

        btnNewP  = new BUTTONS(renderer, font, 60, 190, 300, 60, {100, 200,150, 255}, {120, 220, 170, 255}, "Create New Project") ;
        btnOpenP = new BUTTONS(renderer, font, 60, 280, 300,60,{144, 238, 144, 255}, {152, 251, 152, 255}, "Open Existing Project");
        btnRemRecents = new BUTTONS (renderer, font, 60, 500, 200, 40, {255, 204, 153, 255}, {255, 178, 102, 255}, "Remove Recents");
        btnPresetA4 = new BUTTONS (renderer, font,213, 180, 180, 45, {200,155, 240, 255}, {180, 130, 225, 255}, "A4 (800x600)");
        btnPresetA3 =new BUTTONS(renderer, font, 456, 180,180, 45,{200, 155, 240, 255}, {180, 130, 225, 255},"A3 (1200x800)");
        btnPresetCustom = new BUTTONS (renderer, font, 325, 245, 200,45,{200, 155, 240, 255}, {180, 130,225, 255}, "Custom Size...");
        btnCancelDialog = new BUTTONS(renderer,font, 325, 390, 200, 45, {255, 255, 255, 255}, {240,240, 240, 255},"Cancel");

        btnBack = new BUTTONS(renderer, font, 5, 8, 60, 24, {255,255,255,255}, {230,230,230,255}, "Back");

        int xpos= 120;
        tlbrBtns.push_back (new BUTTONS(renderer, font, xpos, 8, 70,24, {255,255,255,255},{230,230,230,255}, "Select")); xpos += 80;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 70,24, {255,255,255,255},{230,230,230,255}, "Wire")); xpos+= 80;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 180,24,{255,255,255,255},{230,230,230,255}, "Component Library")) ; xpos+=190;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Save")); xpos += 65;
        tlbrBtns.push_back(new BUTTONS (renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Load")); xpos+= 65;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Undo")); xpos+=65;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24,  {255,255,255,255},{230,230,230,255}, "Redo"));

        searchBox = new txtIn (renderer,font, 5, tlbrH+5, pnlLW-10, 25, false);
        searchBox->setTxt("Search...");

        loadRecentPs();
        running =true;
        return true;
    }

    void handleE() {
        SDL_Event ev;
        while (SDL_PollEvent(&ev) !=0) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
            if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED){
                    winW = ev.window.data1;
                    winH = ev.window.data2;
                }
                else if (ev.window.event == SDL_WINDOWEVENT_RESTORED ||ev.window.event == SDL_WINDOWEVENT_MAXIMIZED){
                    SDL_GetWindowSize (window, &winW, &winH);
                }
            }
            if (ev.type == SDL_MOUSEMOTION) {
                SDL_GetMouseState(&mseX , &mseY);
            }

            int adjX = mseX, adjY = mseY;

            if (currentState == app:: NEW_PROJECT_DIALOG){
                int dlgW = 550,dlgH = 420;
                int offsetX = (winW - dlgW) / 2 - 150;
                int offsetY = (winH - dlgH) / 2 - 80;
                adjX = mseX - offsetX;
                adjY = mseY - offsetY;
            }
            else if (currentState == app:: CUSTOM_SIZE_DIALOG) {
                int dlgW = 550, dlgH = 420;
                int offsetX = (winW - dlgW) / 2 - 150;
                int offsetY = (winH - dlgH) / 2 - 80;
                adjX = mseX - offsetX;
                adjY = mseY - offsetY ;
            }
            else if (currentState == app:: PROJECT_NAME_DIALOG) {
                int dlgW = 450, dlgH = 300;
                int offsetX = (winW - dlgW) / 2 - 200;
                int offsetY = (winH - dlgH) / 2 - 150;
                adjX = mseX - offsetX;
                adjY = mseY - offsetY;
            }

            if (currentState == app:: STARTUP_MENU) {
                btnNewP->events(ev);
                btnOpenP->events(ev);
                btnRemRecents->events (ev);
                for (auto btn: btnRecents) btn->events(ev);
                if (btnNewP-> click(ev)) {
                    currentState = app::NEW_PROJECT_DIALOG;
                }
                else if (btnOpenP->click(ev)){
                    string chosen = fileDialog();
                    if (!chosen.empty()) {
                        if (loadProjectFromFile (chosen)) {
                            currentProjectPath = chosen;
                            fitWindowToCanvas() ;
                            SDL_SetWindowMinimumSize (window, 400, 300);
                            SDL_SetWindowMaximumSize (window, 0, 0);
                            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
                            resetLibExpanded() ;
                            resetView();
                            selLibItm = "";
                            currentTool = Tool::SELECT;
                            SDL_FlushEvent( SDL_MOUSEBUTTONDOWN );
                            currentState = app::WORKSPACE;
                            cout << "Opened project: " << chosen << endl;
                        } else {
                            cout << "Failed to load project file." << endl;
                        }
                    }
                }
                else if (btnRemRecents->click(ev)) {
                    clearRecents(); cout << "Recent projects cleared."<< endl;
                }
                else {
                    for (size_t i = 0; i < btnRecents.size(); i++) {
                        if (btnRecents [i]->click(ev)) {
                            cout << "Loading project: " << recentPs[i].name << endl;
                            string path= recentPs[i].path;
                            if (loadProjectFromFile(path)) {
                                currentProjectPath = path;
                            } else {
                                canvasWidth = recentPs[i].canvasW; canvasHeight = recentPs[i].canvasH;
                                activeComps = recentPs[i].activeComponents;
                                placedComponents.clear();
                                componentRuntime.clear();
                                nextComponentId = 1;
                                currentProjectPath = path;
                            }
                            fitWindowToCanvas();
                            SDL_SetWindowMinimumSize(window, 400, 300);
                            SDL_SetWindowMaximumSize(window, 0, 0) ;
                            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                            resetLibExpanded ();
                            resetView();
                            selLibItm = "";
                            currentTool = Tool::SELECT;
                            SDL_FlushEvent (SDL_MOUSEBUTTONDOWN);
                            currentState = app::WORKSPACE;
                        }
                    }
                }
            }
            else if (currentState == app::NEW_PROJECT_DIALOG ) {
                SDL_Event adjEv = ev;
                if (ev.type ==SDL_MOUSEBUTTONDOWN) {
                    adjEv.button.x = adjX;
                    adjEv.button.y = adjY;
                }
                else if (ev.type == SDL_MOUSEMOTION) {
                    adjEv.motion.x =adjX;
                    adjEv.motion.y =adjY;
                }
                btnPresetA4->events(adjEv); btnPresetA3->events(adjEv); btnPresetCustom-> events(adjEv); btnCancelDialog->events(adjEv);
                if (btnPresetA4->click(adjEv)) {
                    canvasWidth = 800; canvasHeight = 600; penProjectName =diffName("Untitled"); openNameDialog();
                }
                else if (btnPresetA3->click(adjEv)) {
                    canvasWidth = 1200; canvasHeight = 800; penProjectName = diffName("Untitled"); openNameDialog();
                }
                else if (btnPresetCustom->click(adjEv)) {
                    if ( !txtWidth){
                        txtWidth = new txtIn(renderer, font, 285, 225, 100, 35, true);
                        txtHeight = new txtIn(renderer,font, 465, 225, 100, 35, true);
                        btnCustomOK = new BUTTONS(renderer, font, 315, 390, 100, 40, {255,255, 255, 255}, {240, 240, 240, 255}, "OK");
                        btnCustomCancel= new BUTTONS(renderer, font, 435, 390,100, 40,{255, 255, 255, 255}, {240, 240, 240, 255}, "Cancel");
                    }
                    txtWidth->setTxt("");
                    txtHeight->setTxt("");
                    txtWidth->setActive(true); txtHeight->setActive(false);
                    currentState =app::CUSTOM_SIZE_DIALOG;
                }
                else if (btnCancelDialog->click(adjEv)) { currentState = app::STARTUP_MENU;}
            }
            else if (currentState == app::CUSTOM_SIZE_DIALOG) {
                SDL_Event adjEv = ev;
                if (ev.type == SDL_MOUSEBUTTONDOWN) {
                    adjEv.button.x  = adjX;
                    adjEv.button.y = adjY;
                } else if (ev.type == SDL_MOUSEMOTION) {
                    adjEv.motion.x = adjX;
                    adjEv.motion.y = adjY;
                }
                if (txtWidth && txtWidth->isActive ())
                    txtWidth-> handleEvent(adjEv);
                if (txtHeight && txtHeight->isActive())
                    txtHeight->handleEvent(adjEv);
                if (btnCustomOK)
                    btnCustomOK->events(adjEv);
                if (btnCustomCancel)
                    btnCustomCancel->events(adjEv);
                if (adjEv.type == SDL_MOUSEBUTTONDOWN && adjEv.button.button== SDL_BUTTON_LEFT) {
                    int mx = adjEv.button.x, my = adjEv.button.y;
                    if (txtWidth && txtWidth->mouseIn(mx, my)) {
                        txtWidth->setActive( true);
                        if (txtHeight)
                            txtHeight-> setActive(false);
                    }
                    else if (txtHeight && txtHeight->mouseIn(mx, my)) {
                        txtHeight->setActive( true );
                        if (txtWidth)
                            txtWidth->setActive(false);
                    }
                    else { if (txtWidth) txtWidth->setActive (false);
                        if (txtHeight)
                            txtHeight->setActive(false); }
                }
                if (btnCustomOK && btnCustomOK->click(adjEv) ) {
                    string wStr= txtWidth  ? txtWidth->gTxt() : ""; string hStr = txtHeight ? txtHeight->gTxt(): "";
                    if (!wStr.empty() && !hStr.empty()) { canvasWidth = stoi (wStr); canvasHeight = stoi(hStr); }
                    else { canvasWidth = 800; canvasHeight = 600; }
                    if (txtWidth) txtWidth->setActive(false);
                    if (txtHeight) txtHeight->setActive(false);
                    updateWorldBounds ();
                    penProjectName = diffName("Untitled"); openNameDialog();
                }
                else if ( btnCustomCancel && btnCustomCancel->click(adjEv)) {
                    if (txtWidth)
                        txtWidth->setActive (false);
                    if (txtHeight)
                        txtHeight->setActive(false);
                    currentState =  app ::NEW_PROJECT_DIALOG;
                }
            }
            else if (currentState == app::PROJECT_NAME_DIALOG) {
                SDL_Event adjEv = ev;
                if (ev.type == SDL_MOUSEBUTTONDOWN){
                    adjEv.button.x = adjX;
                    adjEv.button.y = adjY;
                } else if (ev.type == SDL_MOUSEMOTION) {
                    adjEv.motion.x = adjX;
                    adjEv.motion.y = adjY;
                }
                SDL_StartTextInput();
                if (txtProjectName && txtProjectName->isActive())
                    txtProjectName->handleEvent(adjEv);
                if (btnNameOK)
                    btnNameOK->events(adjEv );
                if (btnNameCancel)
                    btnNameCancel->events(adjEv);
                if (adjEv.type == SDL_MOUSEBUTTONDOWN && adjEv.button.button ==SDL_BUTTON_LEFT){
                    int mx = adjEv.button.x, my = adjEv.button.y;
                    if (txtProjectName && txtProjectName->mouseIn(mx, my)) txtProjectName->setActive (true);
                    else {
                        if (txtProjectName)
                            txtProjectName->setActive(false);
                    }
                }
                if(btnNameOK && btnNameOK->click(adjEv)) {
                    string name = txtProjectName ? txtProjectName->gTxt() : "";
                    if (name.empty()) name = "Untitled";
                    string finalName = diffName(name);
                    string path = "./" + finalName  + ".proj";
                    activeComps.clear();
                    placedComponents.clear();
                    componentRuntime.clear();
                    nextComponentId = 1;
                    wires.clear();
                    junctions.clear();
                    undoStack.clear() ;
                    redoStack.clear();
                    selectedIndices.clear();
                    editingIndex =-1;
                    lastClickedIndex = -1;
                    lastClickTime = 0;
                    updateWorldBounds ();
                    addToRecent(project(finalName, path, gDate( ), canvasWidth, canvasHeight, activeComps));
                    saveProjectToFile (path);
                    currentProjectPath = path;
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    fitWindowToCanvas ();
                    SDL_SetWindowMinimumSize(window, 400, 300);
                    SDL_SetWindowMaximumSize(window, 0, 0);
                    SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    resetLibExpanded();
                    resetView();
                    selLibItm = "";
                    currentTool =Tool::SELECT;
                    SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
                    currentState = app::WORKSPACE;
                    cout << "New project created: " << finalName <<" Canvas: " << canvasWidth << "x" << canvasHeight << endl;
                }
                else if(btnNameCancel && btnNameCancel->click(adjEv)) {
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    currentState = app::STARTUP_MENU;
                }
            }
            else if (currentState == app::WORKSPACE){
                updateViewport();

                mouseHandled = false;

                btnBack->events(ev);
                if (btnBack->click(ev) ) {
                    currentState = app::STARTUP_MENU;
                    SDL_SetWindowMinimumSize ( window, 0, 0);
                    SDL_SetWindowMaximumSize ( window, 0, 0);
                    selLibItm = "";
                    currentTool = Tool::SELECT;
                    wireStartActive = false;
                    selectedIndices. clear();
                    mouseHandled =true;
                }

                for (auto b : tlbrBtns) b->events(ev);
                if (!mouseHandled && tlbrBtns[0]-> click(ev)) {
                    currentTool = Tool::SELECT; selLibItm = "";
                    wireStartActive = false;
                    mouseHandled = true;
                    cout << "Select tool\n";
                }
                else if(!mouseHandled && tlbrBtns[1]->click(ev)) {
                    if (currentTool == Tool::WIRE){
                        currentTool = Tool ::SELECT;
                        wireStartActive = false;
                    }
                    else {
                        currentTool = Tool::WIRE;
                        selLibItm = "" ;
                        wireStartActive = false;
                    }
                    mouseHandled = true;
                    cout << "Wire tool\n";
                }
                else if (!mouseHandled && tlbrBtns[2]->click(ev)){
                    showLib = !showLib;
                    if (showLib) {
                        currentTool =Tool::COMPONENT;
                        searchBox->setTxt ("Search...");
                        searchFilter = "";
                    }
                    else {
                        currentTool = Tool::SELECT;
                        selLibItm = "";
                    }
                    wireStartActive = false;
                    updateViewport() ;
                    mouseHandled = true;
                    cout<< "Component Library toggled\n";
                }
                else if (!mouseHandled && tlbrBtns [3]->click(ev)){
                    if (!currentProjectPath.empty()) {
                        saveProjectToFile (currentProjectPath);
                        addToRecent(project(projNameFromPath(currentProjectPath ), currentProjectPath, gDate(), canvasWidth, canvasHeight, activeComps));
                        cout << "Project saved.\n";
                    }
                    selLibItm = "";
                    mouseHandled = true;
                }
                else if(!mouseHandled && tlbrBtns[4]->click(ev)) {
                    string chosen = fileDialog();
                    if (!chosen.empty ()) {
                        if (loadProjectFromFile(chosen)) {
                            currentProjectPath = chosen;
                            fitWindowToCanvas();
                            resetLibExpanded();
                            resetView();
                            selLibItm = "" ;
                            currentTool = Tool::SELECT;
                            wireStartActive = false;
                            SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
                            currentState = app::WORKSPACE;
                            cout << "Loaded: " << chosen << endl;
                        }
                    }
                    selLibItm = "";
                    mouseHandled =true;
                }
                else if (!mouseHandled && tlbrBtns[5]->click (ev)) {
                    undo();
                    mouseHandled = true;
                    cout << "Undo\n";
                }
                else if (!mouseHandled && tlbrBtns[6]->click(ev)) {
                    redo();
                    mouseHandled = true;
                    cout << "Redo \n";
                }

                if (showLib) {
                    bool propsActive = (propLabelInput && propLabelInput->isActive())|| (propValueInput && propValueInput->isActive());
                    if (!propsActive){
                        searchBox->handleEvent(ev);
                        if(searchBox->isActive())
                            searchFilter = searchBox->gTxt();
                        if (!searchBox->isActive() && searchBox->gTxt().empty())
                            searchBox->setTxt("Search...");
                    }
                }

                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_l && SDL_GetModState() & KMOD_CTRL) {
                    showLib = !showLib ;
                    if (showLib) {
                        currentTool = Tool::COMPONENT;
                        searchBox->setTxt("Search...");
                        searchFilter = "";
                    } else {
                        currentTool = Tool::SELECT ;
                        selLibItm = "";
                    }
                    updateViewport();
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button== SDL_BUTTON_LEFT && !mouseHandled) {
                    bool canvasClickHandled = false;
                    bool ctrlHeld = SDL_GetModState() &KMOD_CTRL;
                    if (showLib && searchBox && searchBox->mouseIn(ev.button.x, ev.button.y)) {
                        searchBox->setActive(true);
                        if (searchBox->gTxt()== "Search...")
                            searchBox->setTxt("");
                        if (propLabelInput) propLabelInput->setActive(false);
                        if (propValueInput ) propValueInput->setActive(false);
                        canvasClickHandled = true;
                    }
                    else {
                        if (searchBox && searchBox->isActive()) {
                            searchBox->setActive(false);
                            if (searchBox->gTxt(). empty())
                                searchBox->setTxt("Search...");
                        }
                    }

                    if (!canvasClickHandled && showLib && activeCompsY > 0 && mseX >= 0 && mseX< pnlLW &&
                        mseY >= activeCompsY && mseY < activeCompsY + (int)activeComps.size() * 26) {
                        int index = (mseY- activeCompsY) / 26;
                        if (index >= 0 && index <(int)activeComps.size()){
                            SDL_Rect r =  {5, activeCompsY + index * 26, pnlLW - 10, 20};
                            SDL_Rect xRect = {r.x + r.w - 15, r.y + (r.h - 10)/2, 10, 10};
                            if (mseX >= xRect.x && mseX  <= xRect.x + xRect.w && mseY >= xRect. y && mseY <= xRect.y + xRect.h) {
                                UndoAction  act;
                                act.type = ACTIVE_REMOVE;
                                act.compName = activeComps[index];
                                act.activeIndex = index ;
                                pushUndo(act);
                                activeComps.erase(activeComps.begin() + index);
                                canvasClickHandled = true;
                            }
                            else if (mseX >= r.x && mseX<= r.x + r.w &&mseY >= r.y && mseY <= r.y + r.h) {
                                selLibItm = activeComps [index];
                                currentTool = Tool:: COMPONENT;
                                canvasClickHandled = true;
                            }
                        }
                    }

                    if (!canvasClickHandled && showLib && mseX>=0 && mseX < pnlLW && mseY >= tlbrH && mseY < winH - statH) {
                        string lowerFilter = toLower (searchFilter);
                        int yOff = tlbrH +45;
                        bool clickedOnComponent = false;
                        for (auto& cat : libCategories) {
                            yOff += 30;
                            bool showComponents = cat.expanded || !searchFilter.empty();
                            if(showComponents){
                                for (auto& comp : cat.components) {
                                    if (searchFilter.empty() || toLower(cat.name).find(lowerFilter) != string::npos || toLower(comp).find(lowerFilter) != string::npos) {
                                        SDL_Rect compRect = {15, yOff, pnlLW-20, 20};
                                        if (mseX >= compRect.x && mseX <= compRect.x+compRect.w && mseY >= compRect.y && mseY <= compRect.y+compRect.h) {
                                            selLibItm = comp;
                                            currentTool = Tool::COMPONENT;
                                            if (ev.button.clicks == 2) {
                                                if (find(activeComps.begin(), activeComps.end(),comp)== activeComps.end()) {
                                                    UndoAction act;
                                                    act.type = ACTIVE_ADD;
                                                    act.compName = comp;
                                                    pushUndo (act);
                                                    activeComps.push_back (comp);
                                                }
                                            }
                                            clickedOnComponent = true;
                                            canvasClickHandled= true;
                                            break;
                                        }
                                        yOff += 28;
                                    }
                                }
                                yOff += 10;
                            }
                            if (clickedOnComponent) break;
                        }
                        if (!clickedOnComponent) {
                            yOff = tlbrH + 45;
                            for (auto& cat : libCategories) {
                                SDL_Rect catRect = {5, yOff, pnlLW-10, 20};
                                if (mseX >= catRect.x && mseX <= catRect.x + catRect.w && mseY >= catRect.y && mseY <= catRect.y+catRect.h) {
                                    cat.expanded = !cat.expanded;
                                    canvasClickHandled = true;
                                    break;
                                }
                                yOff += 30;
                                bool showComponents = cat.expanded || !searchFilter.empty();
                                if (showComponents) {
                                    for (auto& comp: cat.components) {
                                        if (searchFilter.empty() ||toLower (cat.name).find(lowerFilter) != string::npos ||
                                            toLower(comp).find(lowerFilter) != string::npos) {
                                            yOff += 28;
                                        }
                                    }
                                    yOff += 10;
                                }
                            }
                        }
                    }
                    else if ( !canvasClickHandled && mseX >= zoomRct.x && mseX <= zoomRct.x + zoomRct.w &&
                        mseY >= zoomRct.y && mseY<=zoomRct.y + zoomRct.h){
                        resetView ();
                        selLibItm =  "";
                        canvasClickHandled = true;
                    }
                    else if (!canvasClickHandled && mseX >= vpX && mseX < vpX +vpW && mseY >= vpY && mseY < vpY+ vpH) {
                        if (propLabelInput) propLabelInput->setActive (false);
                        if (propValueInput) propValueInput->setActive (false);
                        if (!btnBack-> click (ev)) {
                            if (ctrlHeld){
                                drawingSelection = false;
                                isPan = true ;
                                panStartX = ev.button.x ; panStartY = ev.button.y;
                                panOffXst = panX; panOffYst = panY;
                            }
                            else if (currentTool == Tool::WIRE) {
                                int wx = snapToGrid(scrToWldX(mseX));
                                int wy = snapToGrid(scrToWldY(mseY));
                                clampToCanvas(wx, wy);
                                SDL_Point clickPoint = {wx, wy};

                                bool foundPin = false;
                                for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
                                    auto pins = gCompPinPos(placedComponents[ci]);
                                    for (size_t pi = 0; pi < pins.size(); ++pi) {
                                        int px = pins[pi].x, py = pins[pi].y;
                                        int dx = wx - px, dy = wy - py;
                                        if (dx*dx + dy*dy <= 100) {
                                            clickPoint = {px, py};
                                            foundPin = true;
                                            break;
                                        }
                                    }
                                    if (foundPin) break;
                                }

                                if (!wireStartActive) {
                                    wireStartPoint = clickPoint;
                                    wireStartActive = true;
                                } else {
                                    vector<SDL_Point> path = calcOrthoPath(wireStartPoint, clickPoint);
                                    UndoAction act;
                                    act.type = WIRE_ADD;
                                    act.wiresBefore = wires;
                                    act.junctionsBefore= junctions;
                                    wires.push_back(path);
                                    act.wiresAfter = wires;
                                    act.junctionsAfter= junctions;
                                    pushUndo(act);
                                    wireStartActive = false;
                                }
                                canvasClickHandled = true;
                            }
                            else if (currentTool == Tool::COMPONENT && !selLibItm.empty()){
                                int wx = snapToGrid(scrToWldX(mseX));
                                int wy = snapToGrid(scrToWldY(mseY));
                                clampToCanvas (wx, wy);
                                placedComp pc ={selLibItm, wx, wy, 0, false, false, "", ""} ;
                                initializeComponent(pc);
                                UndoAction act;
                                act.type = COMP_PLACE;
                                act.comp = pc ;
                                act.wiresBefore = wires;
                                act.junctionsBefore = junctions;
                                placedComponents.push_back (pc);
                                act.wiresAfter = wires;
                                act.junctionsAfter = junctions;
                                pushUndo(act);
                            } else {
                                bool hitComponent =false;
                                for (size_t i = 0; i < placedComponents.size(); ++i) {
                                    if (isPointInsideComponent(placedComponents[i], scrToWldX(mseX), scrToWldY(mseY)) ) {
                                        initializeComponent(placedComponents[i]);
                                        if (placedComponents[i].name == "Push Button" && !(SDL_GetModState() & KMOD_ALT)) {
                                            componentRuntime[placedComponents[i].id].pressed = true;
                                            selectedIndices.clear(); selectedIndices.push_back(i);
                                            hitComponent = true;
                                            break;
                                        }
                                        if (placedComponents[i].name == "Switch" && ev.button.clicks >= 2 && !(SDL_GetModState() & KMOD_ALT)) {
                                            placedComponents[i].switchState = !placedComponents[i].switchState;
                                            placedComponents[i].value = placedComponents[i].switchState ? "CLOSED" : "OPEN";
                                            selectedIndices.clear(); selectedIndices.push_back(i);
                                            hitComponent = true;
                                            lastClickedIndex = -1; lastClickTime = 0;
                                            break;
                                        }
                                        Uint32 now = SDL_GetTicks();
                                        if (lastClickedIndex == i && (now - lastClickTime) < 400) {
                                            editingIndex = i;
                                            showProp = true;
                                            if (!propLabelInput){
                                                propLabelInput = new txtIn(renderer, font, 0, 0, 100, 25, false);
                                                propValueInput = new txtIn (renderer, font, 0, 0, 100, 25, false);
                                                btnPropOK = new BUTTONS(renderer, libFont, 0, 0, 60, 20, {255,255,255,255}, {240,240,240,255}, "Apply");
                                                btnPropCancel =  new BUTTONS(renderer, libFont, 0, 0, 60, 20, {255,255,255,255}, {240,240,240,255}, "Cancel");
                                            }
                                            propLabelInput->setTxt(placedComponents[i].label);
                                            propValueInput-> setTxt(placedComponents[i].value);
                                            propLabelInput->setActive(true);
                                            propValueInput->setActive(false);
                                            if (searchBox && searchBox->isActive()){
                                                searchBox->setActive(false);
                                                searchBox->setTxt ("Search...");
                                            }
                                            selectedIndices.clear();
                                            selectedIndices.push_back (i);
                                            lastClickedIndex = -1;
                                            lastClickTime = 0;
                                            hitComponent = true;
                                            break;
                                        }
                                        lastClickedIndex =i;
                                        lastClickTime = now;
                                        bool alreadySelected = find(selectedIndices.begin(),selectedIndices.end(),i) != selectedIndices.end();
                                        if (SDL_GetModState() & KMOD_SHIFT){
                                            auto it = find (selectedIndices.begin(), selectedIndices.end(), i);
                                            if (it != selectedIndices.end()) {
                                                selectedIndices.erase(it);
                                                hitComponent = true ;
                                                break;
                                            }
                                            else {
                                                selectedIndices.push_back(i);
                                                alreadySelected =true;
                                            }
                                        }
                                        else {
                                            if (!alreadySelected){
                                                selectedIndices.clear ();
                                                selectedIndices.push_back(i);
                                                alreadySelected = true;
                                            }
                                        }
                                        if (alreadySelected) {
                                            draggingComponents = true;
                                            dragStartX =mseX;
                                            dragStartY =mseY;
                                            preDragComponents = placedComponents;
                                            preDragWires = wires;
                                            dragSnapshots.clear();
                                            for (auto idx :selectedIndices) {
                                                dragSnapshots.push_back(placedComponents[idx]);
                                            }
                                        }
                                        hitComponent = true;
                                        break ;
                                    }
                                }
                                if (!hitComponent){
                                    if (!(SDL_GetModState() & KMOD_SHIFT)) {
                                        selectedIndices.clear() ;
                                    }
                                    bool junctionToggled= false;
                                    if (currentTool == Tool::SELECT) {
                                        int wx = scrToWldX (mseX);
                                        int wy = scrToWldY (mseY);
                                        for (size_t i = 0; i < wires.size(); ++i){
                                            for (size_t j = i+1; j < wires.size(); ++j) {
                                                for (size_t s1 = 0; s1+1 < wires[i] .size(); ++s1) {
                                                    for (size_t s2 = 0; s2+1 < wires[j].size(); ++s2){
                                                        SDL_Point inter;
                                                        if (segmentsIntersect(wires[i][s1], wires[i] [s1+1], wires[j][s2], wires[j][s2+1], inter)) {
                                                            if (pointNear({wx, wy}, inter, 8 )) {
                                                                bool exists = false;
                                                                for (auto& jpt: junctions) {
                                                                    if (pointNear(jpt, inter, 6)) { exists = true ; break; }
                                                                }
                                                                if(!exists) {
                                                                    UndoAction act;
                                                                    act.type = WIRE_JUNCTION_ADD;
                                                                    act.junctionsBefore= junctions;
                                                                    junctions.push_back(inter);
                                                                    act.junctionsAfter = junctions;
                                                                    pushUndo(act);
                                                                }
                                                                else {
                                                                    for (auto it  = junctions.begin(); it != junctions.end(); ++it) {
                                                                        if (pointNear(*it, inter, 6)){
                                                                            UndoAction act;
                                                                            act.type = WIRE_JUNCTION_REMOVE;
                                                                            act. junctionsBefore = junctions;
                                                                            act.junctionPoint = *it;
                                                                            junctions.erase(it);
                                                                            act.junctionsAfter = junctions;
                                                                            pushUndo(act);
                                                                            break ;
                                                                        }
                                                                    }
                                                                }
                                                                junctionToggled = true;
                                                                goto junction_done  ;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        junction_done:;
                                    }
                                    if (!junctionToggled){
                                        drawingSelection = true;
                                        selectionRect ={mseX, mseY, 0, 0} ;
                                        lastClickedIndex = -1;
                                        lastClickTime = 0;
                                    }
                                }
                            }
                        }
                        selLibItm ="";
                        canvasClickHandled = true ;
                    }
                    else if (!canvasClickHandled && showProp &&editingIndex < placedComponents.size()) {
                        if (propLabelInput && propLabelInput->mouseIn(mseX, mseY) ) {
                            propLabelInput->setActive (true);
                            propValueInput->setActive(false);
                            if (searchBox && searchBox->isActive()) {
                                searchBox->setActive(false);
                                searchBox->setTxt( "Search...");
                            }
                            canvasClickHandled = true ;
                        }
                        else if (propValueInput &&   propValueInput->mouseIn(mseX, mseY)) {
                            propValueInput-> setActive(true);
                            propLabelInput-> setActive(false);
                            if (searchBox && searchBox->isActive()) {
                                searchBox-> setActive(false);
                                searchBox-> setTxt("Search...");
                            }
                            canvasClickHandled = true;
                        }
                        else {
                            if (propLabelInput) propLabelInput->setActive(false);
                            if (propValueInput)  propValueInput->setActive(false);
                        }
                        if  (!canvasClickHandled && btnPropOK && btnPropOK->click(ev)){
                            if (editingIndex <placedComponents.size()) {
                                UndoAction act;
                                act.type =COMP_EDIT;
                                act.oldComp = placedComponents [editingIndex];
                                placedComponents[editingIndex].label = propLabelInput->gTxt();
                                placedComponents[editingIndex].value = propValueInput->gTxt();
                                applyComponentConfig(placedComponents[editingIndex]);
                                act.comp = placedComponents[editingIndex];
                                pushUndo (act);
                            }
                            editingIndex = -1;
                            showProp = false;
                            updateViewport();
                            if (propLabelInput) propLabelInput->setActive(false);
                            if (propValueInput) propValueInput->setActive(false);
                            canvasClickHandled = true ;
                        }
                        else if (!canvasClickHandled && btnPropCancel && btnPropCancel->click(ev) ) {
                            editingIndex = -1;
                            showProp = false;
                            updateViewport ();
                            if (propLabelInput) propLabelInput->setActive (false);
                            if (propValueInput) propValueInput->setActive (false);
                            canvasClickHandled = true;
                        }
                    }
                }
                else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_MIDDLE) {
                    if (mseX >= vpX && mseX < vpX+vpW && mseY >= vpY && mseY < vpY+ vpH) {
                        drawingSelection = false;
                        isPan = true ;
                        panStartX = ev.button.x ; panStartY = ev.button.y;
                        panOffXst = panX; panOffYst = panY;
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button== SDL_BUTTON_RIGHT) {
                    if (mseX >= vpX && mseX < vpX + vpW && mseY >= vpY && mseY < vpY +vpH) {
                        bool removedWire = false;
                        for (size_t wi = 0; wi < wires.size(); ++wi) {
                            for (size_t si = 0; si < wires[wi].size() - 1; ++si) {
                                int x1 = wldToScrX(wires[wi][si].x);
                                int y1 = wldToScrY(wires[wi][si].y);
                                int x2 = wldToScrX(wires[wi][si+1].x);
                                int y2 = wldToScrY(wires[wi][si+1].y);
                                if (pointToSegmentDist(mseX, mseY, x1, y1, x2, y2) < 6) {
                                    UndoAction act;
                                    act.type = WIRE_DELETE;
                                    act.wiresBefore = wires;
                                    act.junctionsBefore = junctions ;
                                    vector<SDL_Point> removedWireData = wires[wi];
                                    wires.erase(wires.begin() + wi);
                                    removeJunctionsOnWire (removedWireData);
                                    act.wiresAfter = wires;
                                    act.junctionsAfter = junctions;
                                    pushUndo(act);
                                    removedWire = true;
                                    break;
                                }
                            }
                            if (removedWire) break;
                        }
                        if (!removedWire) {
                            for (size_t i = 0; i < placedComponents.size(); ++i) {
                                if (isPointInsideComponent(placedComponents[i], scrToWldX(mseX), scrToWldY(mseY))){
                                    UndoAction act;
                                    act.type =  COMP_DELETE;
                                    act.comp = placedComponents[i];
                                    act.wiresBefore = wires;
                                    act.junctionsBefore = junctions;
                                    pushUndo(act);
                                    placedComponents.erase (placedComponents.begin() + i);
                                    act.wiresAfter = wires;
                                    act.junctionsAfter =junctions;
                                    selectedIndices.clear();
                                    if (editingIndex == i) editingIndex= -1;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                    for (auto& runtimeItem : componentRuntime) runtimeItem.second.pressed = false;
                    if (draggingComponents){
                        if (dragStartX != mseX ||dragStartY != mseY){
                            updateWiresForMovingComp () ;
                            UndoAction act;
                            act.type = COMP_MOVE;
                            act.compsBefore= preDragComponents;
                            act.compsAfter = placedComponents;
                            act.wiresBefore = preDragWires;
                            act.wiresAfter = wires;
                            pushUndo(act);
                        }
                        draggingComponents = false;
                    }
                    if (drawingSelection){
                        drawingSelection = false;
                        if (selectionRect.w > 0 &&selectionRect.h > 0) {
                            SDL_Rect sr= selectionRect;
                            for (size_t i = 0; i < placedComponents.size(); ++i){
                                int sx = wldToScrX(placedComponents[i].x);
                                int sy = wldToScrY(placedComponents[i].y);
                                if (sx >= sr.x && sx <= sr.x + sr.w && sy >= sr.y && sy<= sr.y + sr.h) {
                                    if (find(selectedIndices.begin(), selectedIndices.end(), i) == selectedIndices.end())
                                        selectedIndices.push_back(i);
                                }
                            }
                        }
                        selectionRect = {0,0,0,0};
                    }
                    isPan =false;
                }
                else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_MIDDLE){
                    if (isPan) isPan = false;
                }
                if (isPan && ev.type== SDL_MOUSEMOTION) {
                    panX = panOffXst + (ev.motion.x- panStartX);
                    panY = panOffYst + (ev.motion.y- panStartY);
                }
                else if (ev.type == SDL_MOUSEMOTION) {
                    if (draggingComponents && (ev.motion.state & SDL_BUTTON_LMASK)) {
                        int deltaWX = scrToWldX(mseX) - scrToWldX(dragStartX);
                        int deltaWY = scrToWldY(mseY) - scrToWldY(dragStartY);
                        for (size_t j = 0; j < selectedIndices.size(); ++j) {
                            size_t idx = selectedIndices[j];
                            placedComponents[idx].x = snapToGrid(dragSnapshots[j].x + deltaWX);
                            placedComponents[idx].y = snapToGrid(dragSnapshots[j].y + deltaWY);
                        }
                        wires = preDragWires;
                        vector<pair<SDL_Point, SDL_Point>> pinMappings;
                        for (size_t j = 0; j < selectedIndices.size(); ++j) {
                            size_t idx = selectedIndices[j];
                            vector<SDL_Point> oldPins = gCompPinPos(dragSnapshots[j]);
                            vector<SDL_Point> newPins = gCompPinPos(placedComponents[idx]);
                            for (size_t pi = 0; pi < oldPins.size(); ++pi) {
                                pinMappings.push_back({oldPins[pi], newPins[pi]});
                            }
                        }
                        for (auto& wire : wires) {
                            if (wire.size() < 2) continue;
                            SDL_Point& start = wire.front();
                            SDL_Point& end = wire.back();
                            for (const auto& mapping : pinMappings) {
                                if (start.x == mapping.first.x && start.y == mapping.first.y) {
                                    start = mapping.second;
                                    break;
                                }
                            }
                            for (const auto& mapping : pinMappings) {
                                if (end.x == mapping.first.x && end.y == mapping.first.y) {
                                    end = mapping.second;
                                    break;
                                }
                            }
                            wire = calcOrthoPath(start, end);
                        }
                    } else if (drawingSelection){
                        int x = min(mseX, selectionRect.x);
                        int y = min(mseY, selectionRect.y);
                        int w = abs(mseX - selectionRect.x) ;
                        int h = abs(mseY - selectionRect.y);
                        selectionRect = {x, y, w, h};
                    }

                    if (currentTool == Tool::WIRE && !draggingComponents && !drawingSelection) {
                        hoveredPinActive = false;
                        int bestDist = 100;
                        for (size_t ci = 0; ci < placedComponents.size(); ++ci) {
                            auto pins = gCompPinPos(placedComponents[ci]);
                            for (size_t pi = 0; pi < pins.size(); ++pi) {
                                int px = pins[pi].x, py = pins[pi].y;
                                int dx = mseX - wldToScrX(px);
                                int dy = mseY - wldToScrY(py);
                                int dist = dx*dx + dy*dy;
                                if (dist < bestDist) {
                                    bestDist = dist;
                                    hoveredPin = {px, py};
                                    hoveredPinActive = true;
                                }
                            }
                        }
                        if (bestDist >= 100) hoveredPinActive = false;
                    }
                }
                if (ev.type == SDL_MOUSEWHEEL && mseX>=vpX && mseX<vpX+vpW && mseY >=vpY && mseY<vpY+vpH){
                    int mx = mseX, my = mseY;
                    float oldZm =zmLvl;
                    if (ev.wheel.y >0)
                        zmLvl *= 1.1f;
                    else if (ev.wheel.y < 0)
                        zmLvl /= 1.1f ;
                    if (zmLvl< 0.2f)
                        zmLvl = 0.2f;
                    if (zmLvl > 5.0f)
                        zmLvl = 5.0f;
                    panX = (mx - vpX) - (int)(((mx - vpX) - panX) *(zmLvl / oldZm));
                    panY = (my - vpY) - (int)(((my - vpY) - panY) *(zmLvl / oldZm));
                }

                if (showProp && editingIndex < placedComponents.size( )) {
                    propLabelInput->handleEvent(ev);
                    propValueInput->handleEvent(ev);
                    btnPropOK->events(ev);
                    btnPropCancel->events(ev);
                }

                if (ev.type == SDL_KEYDOWN) {
                    bool textFieldActive = (propLabelInput && propLabelInput->isActive())|| (propValueInput && propValueInput->isActive()) ||(searchBox && searchBox->isActive());

                    if (ev.key.keysym.sym == SDLK_z && (SDL_GetModState() & KMOD_CTRL)) { undo();}
                    else if (ev.key.keysym.sym == SDLK_y && (SDL_GetModState() & KMOD_CTRL)) { redo(); }
                    else if (ev.key.keysym.sym == SDLK_s && (SDL_GetModState() & KMOD_CTRL)) {
                        if (!currentProjectPath.empty()) {
                            saveProjectToFile(currentProjectPath) ;
                            addToRecent(project(projNameFromPath(currentProjectPath), currentProjectPath, gDate(), canvasWidth, canvasHeight, activeComps));
                            cout << "Project saved (Ctrl+S).\n";
                        }
                        selLibItm = "" ;
                    }
                    else if (ev.key.keysym.sym == SDLK_o && (SDL_GetModState() & KMOD_CTRL)) {
                        string chosen = fileDialog();
                        if (!chosen.empty()) {
                            if (loadProjectFromFile(chosen)) {
                                currentProjectPath = chosen;
                                fitWindowToCanvas() ;
                                resetLibExpanded ();
                                resetView();
                                selLibItm = "";
                                currentTool =Tool::SELECT;
                                SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
                                currentState = app::WORKSPACE;
                                cout << "Loaded: " << chosen << endl;
                            }
                        }
                        selLibItm = "";
                    }
                    else if (ev.key.keysym.sym == SDLK_0 && (SDL_GetModState() & KMOD_CTRL)){
                        resetView ();
                        selLibItm = "";
                    }
                    else if (!textFieldActive) {
                        if (ev.key.keysym.sym == SDLK_1) {
                            currentTool = Tool::SELECT;
                            cout << "Select tool (keyboard)\n";
                            selLibItm = "";
                        }
                        else if (ev.key.keysym.sym == SDLK_2) {
                            currentTool = Tool::WIRE;
                            cout << "Wire tool (keyboard)\n";
                            selLibItm = "";
                        }
                        else if (ev.key.keysym.sym == SDLK_3) {
                            showLib =true;
                            currentTool = Tool::COMPONENT;
                            searchBox->setTxt("Search...");
                            searchFilter = "";
                            cout <<"Component Library (keyboard)\n";
                        }
                        else if (ev.key.keysym.sym == SDLK_4) {
                            showProp =!showProp;
                            cout << "Properties panel toggled\n";
                        }
                        else if (ev.key.keysym.sym == SDLK_DELETE) {
                            for (auto it = selectedIndices.rbegin(); it != selectedIndices.rend() ; ++it) {
                                size_t i = *it;
                                if (i < placedComponents.size()) {
                                    UndoAction act;
                                    act.type = COMP_DELETE;
                                    act.comp = placedComponents [i];
                                    act.wiresBefore = wires;
                                    act.junctionsBefore = junctions;
                                    pushUndo(act);
                                    placedComponents.erase(placedComponents.begin() + i);
                                    act.wiresAfter = wires;
                                    act.junctionsAfter = junctions ;
                                    if (editingIndex == i) editingIndex = -1;
                                }
                            }
                            selectedIndices.clear();
                        }
                        else if(ev.key.keysym.sym == SDLK_r) {
                            vector<placedComp>before = placedComponents;
                            auto wiresBeforeRotate = wires;
                            auto junctionsBeforeRotate = junctions ;
                            for (size_t idx : selectedIndices){
                                placedComponents[idx].angle = (placedComponents[idx].angle + 90) %360;
                            }
                            updateWiresForTransform(before, placedComponents);
                            UndoAction act;
                            act.type = COMP_TRANSFORM ;
                            act.compsBefore = before;
                            act.compsAfter = placedComponents;
                            act.wiresBefore = wiresBeforeRotate;
                            act.wiresAfter = wires;
                            act.junctionsBefore= junctionsBeforeRotate;
                            act.junctionsAfter = junctions;
                            pushUndo(act);
                        }
                        else if (ev.key.keysym.sym== SDLK_h){
                            vector <placedComp> before = placedComponents;
                            auto wiresBeforeFlip = wires;
                            auto junctionsBeforeFlip  = junctions;
                            for (size_t idx: selectedIndices) {
                                placedComponents[idx].flipH = !placedComponents[idx].flipH;
                            }
                            updateWiresForTransform(before, placedComponents);
                            UndoAction act;
                            act.type =COMP_TRANSFORM;
                            act.compsBefore = before;
                            act.compsAfter =placedComponents;
                            act.wiresBefore = wiresBeforeFlip;
                            act.wiresAfter = wires;
                            act.junctionsBefore= junctionsBeforeFlip;
                            act.junctionsAfter = junctions;
                            pushUndo(act);
                        }
                        else if(ev.key.keysym.sym == SDLK_v) {
                            vector<placedComp> before = placedComponents;
                            auto wiresBeforeFlipV = wires;
                            auto junctionsBeforeFlipV = junctions;
                            for (size_t idx : selectedIndices) {
                                placedComponents [idx].flipV = !placedComponents [idx].flipV;
                            }
                            updateWiresForTransform(before, placedComponents);
                            UndoAction act;
                            act.type = COMP_TRANSFORM;
                            act.compsBefore= before;
                            act.compsAfter = placedComponents;
                            act.wiresBefore = wiresBeforeFlipV;
                            act.wiresAfter = wires;
                            act.junctionsBefore = junctionsBeforeFlipV;
                            act.junctionsAfter  = junctions;
                            pushUndo(act);
                        }
                        else if (ev.key.keysym.sym == SDLK_PLUS || ev.key.keysym.sym  == SDLK_KP_PLUS){
                            float oldZm = zmLvl; zmLvl *= 1.1f; if (zmLvl > 5.0f) zmLvl =5.0f ;
                            int cx = vpW/2 ,cy = vpH/2;
                            panX = cx - (int) ((cx - panX) * (zmLvl / oldZm));
                            panY = cy - (int) ((cy - panY) * (zmLvl / oldZm));
                        }
                        else if (ev.key.keysym.sym == SDLK_MINUS || ev.key.keysym.sym == SDLK_KP_MINUS){
                            float oldZm = zmLvl; zmLvl/= 1.1f;
                            if (zmLvl < 0.2f)
                                zmLvl = 0.2f;
                            int cx = vpW/2 , cy = vpH/2;
                            panX = cx - (int) ((cx - panX) * (zmLvl / oldZm));
                            panY = cy - (int) ((cy - panY) * (zmLvl / oldZm));
                        }
                        else if(ev.key.keysym.sym == SDLK_0 && SDL_GetModState() & KMOD_CTRL){
                        }
                        else if(ev.key.keysym.sym == SDLK_s && SDL_GetModState() & KMOD_CTRL) {
                        }
                        else if(ev.key.keysym.sym == SDLK_o && SDL_GetModState() & KMOD_CTRL){
                        }
                    }
                }
            }
        }
    }

    void updateWiresForTransform(const vector<placedComp>& oldComps, const vector<placedComp>& newComps){
        for (auto& wire : wires) {
            bool changed = false;
            for (size_t i = 0; i < oldComps.size(); ++i) {
                if (i>= newComps.size()) break;
                vector<SDL_Point> oldPins = gCompPinPos(oldComps [i]);
                vector<SDL_Point> newPins = gCompPinPos(newComps [i]);
                for (size_t pi = 0;pi < oldPins.size(); ++pi) {
                    if (wire.front().x == oldPins[pi].x && wire.front().y == oldPins[pi].y) {
                        wire.front() = newPins[pi];
                        changed =true;
                        break;
                    }
                    if (wire.back().x == oldPins[pi].x && wire.back().y == oldPins[pi].y) {
                        wire.back() =newPins[pi];
                        changed = true;
                        break;
                    }
                }
                if (changed)  break;
            }
            if (changed) wire = calcOrthoPath(wire.front (), wire.back());
        }
    }

    void render(){
        SDL_SetRenderDrawColor( renderer, 245, 245, 245, 255);
        SDL_RenderClear (renderer);

        if (currentState == app::STARTUP_MENU) {
            drawTxt("Proteus", 60, 70, {0,0, 0, 255}, titleFont);
            drawTxt("Recent Projects:", 450, 155, {80, 80, 80, 255} );
            btnNewP->draw(renderer) ; btnOpenP->draw (renderer); btnRemRecents->draw (renderer);
            if (btnRecents.empty()) {
                drawTxt("(no recent projects)", 470, 205, {150,150, 150, 255 });
            }
            else { for (auto btn : btnRecents) {
                btn->draw(renderer);
            } }
        }
        else if (currentState == app::NEW_PROJECT_DIALOG) {
            int dlgW =550, dlgH = 420;
            int offsetX = (winW - dlgW) / 2 - 150;
            int offsetY = (winH - dlgH) / 2 -80;
            SDL_Rect dialogBox ={150 + offsetX, 80 + offsetY, dlgW, dlgH};
            SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255); SDL_RenderFillRect(renderer,&dialogBox);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect( renderer, &dialogBox);
            drawTxt("New Project - Select Canvas Size", 285 + offsetX, 100 + offsetY, {20, 20, 20, 255});
            btnPresetA4->drawAt(renderer, 213 + offsetX, 180 + offsetY);
            btnPresetA3->drawAt(renderer, 456 + offsetX, 180 + offsetY );
            btnPresetCustom->drawAt(renderer, 325 + offsetX, 245 + offsetY);
            btnCancelDialog->drawAt(renderer, 325 + offsetX, 390 + offsetY);
        }
        else if (currentState == app::CUSTOM_SIZE_DIALOG) {
            int dlgW = 550, dlgH = 420;
            int offsetX = (winW - dlgW)/ 2 - 150;
            int offsetY = (winH - dlgH) / 2 - 80;
            SDL_Rect dlg = { 150 + offsetX, 80 + offsetY, dlgW, dlgH};
            SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255); SDL_RenderFillRect( renderer, &dlg);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect(renderer, &dlg );
            int titleW, titleH; if (font) TTF_SizeText(font, "Enter canvas dimensions:",&titleW, &titleH); else titleW= 250;
            int titleX = dlg.x + (dlg.w- titleW) / 2 ;
            drawTxt ("Enter canvas dimensions:", titleX, 100 + offsetY, {20, 20, 20, 255});
            drawTxt("Width:", 250 + offsetX, 200 + offsetY, {20, 20, 20,255} );
            drawTxt("Height:", 430 + offsetX, 200 + offsetY, {20, 20, 20, 255});
            if (txtWidth)
                txtWidth->drawAt(renderer, 285 + offsetX, 225 + offsetY);
            if (txtHeight)
                txtHeight->drawAt(renderer, 465 + offsetX, 225 + offsetY);
            if (btnCustomOK)
                btnCustomOK->drawAt( renderer, 315 + offsetX, 390 + offsetY);
            if (btnCustomCancel)
                btnCustomCancel->drawAt(renderer, 435 + offsetX, 390 + offsetY);
        }
        else if (currentState == app::PROJECT_NAME_DIALOG) {
            int dlgW = 450, dlgH = 300;
            int offsetX = (winW - dlgW) / 2 - 200;
            int offsetY = (winH - dlgH) / 2 - 150;
            SDL_Rect dlg = {200 + offsetX, 150 + offsetY, dlgW, dlgH};
            SDL_SetRenderDrawColor (renderer, 210, 210, 230, 255); SDL_RenderFillRect(renderer, &dlg);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect(renderer, &dlg);
            int titleW, titleH;
            if (font)
                TTF_SizeText(font, "Enter project name:", &titleW, &titleH); else titleW = 200;
            int titleX = dlg.x+(dlg.w - titleW) / 2;
            drawTxt("Enter project name:", titleX, 180 + offsetY, {20, 20, 20, 255});
            if (txtProjectName)
                txtProjectName->drawAt(renderer, 275 + offsetX, 240 + offsetY);
            if (btnNameOK)
                btnNameOK->drawAt(renderer, 275 + offsetX, 340 + offsetY);
            if (btnNameCancel)
                btnNameCancel->drawAt(renderer, 455 + offsetX, 340 + offsetY);
        }
        else if (currentState ==app::WORKSPACE){
            updateSection6Simulation();
            SDL_SetRenderDrawColor ( renderer, 255, 255, 255, 255); SDL_RenderClear (renderer);

            updateViewport();

            int leftPanelWidth = showLib ?pnlLW : 0;
            int rightPanelWidth = showProp ? pnlRW : 0;
            vpX = leftPanelWidth;
            vpW = winW - leftPanelWidth - rightPanelWidth;
            vpY = tlbrH;
            vpH = winH - tlbrH - statH;

            SDL_Rect tlbrBg = {0,0,winW, tlbrH};
            SDL_SetRenderDrawColor(renderer, 150,170,200,255); SDL_RenderFillRect(renderer, &tlbrBg);
            btnBack->draw(renderer);
            for (auto b : tlbrBtns) b->draw(renderer);

            if (showLib) {
                SDL_Rect libBg = { 0, tlbrH, pnlLW, vpH};
                SDL_SetRenderDrawColor(renderer, 235,235,245,255);
                SDL_RenderFillRect(renderer, &libBg) ;
                SDL_SetRenderDrawColor(renderer, 170,170,180,255);
                SDL_RenderDrawLine(renderer, pnlLW, tlbrH, pnlLW, tlbrH +vpH);
                drawTxt("Library" , 8, tlbrH+ 5, {0,0,0,255});

                searchBox->draw(renderer);

                string lowerFilter = toLower(searchFilter);
                int yOff = tlbrH + 45;
                bool anyShown = false;
                for (auto& cat : libCategories){
                    SDL_Rect catRect = {5, yOff, pnlLW-10, 20};
                    SDL_SetRenderDrawColor(renderer, cat.expanded ? 200 : 220, 210, 230, 255);
                    SDL_RenderFillRect(renderer, &catRect);
                    SDL_SetRenderDrawColor(renderer, 80,80,80,255);
                    SDL_RenderDrawRect(renderer, &catRect);
                    int catTextW, catTextH;
                    TTF_SizeText(font, cat.name.c_str(), &catTextW, &catTextH);
                    int catTx = catRect.x + (catRect.w - catTextW)/2;
                    int catTy = catRect.y + (catRect.h - catTextH)/2;
                    drawTxt(cat.name, catTx, catTy, {0,0,0,255});
                    yOff += 30;
                    bool showComponents = cat.expanded || !searchFilter.empty();
                    if (showComponents){
                        for (auto& comp : cat.components) {
                            if (searchFilter.empty() || toLower(cat.name).find(lowerFilter) != string::npos || toLower(comp).find(lowerFilter)!= string::npos) {
                                SDL_Rect compRect = {15, yOff, pnlLW-20, 20};
                                SDL_SetRenderDrawColor(renderer, selLibItm==comp ?180 : 235, 230, 245, 255);
                                SDL_RenderFillRect(renderer, &compRect);
                                SDL_SetRenderDrawColor(renderer, 160,160,160,255);
                                SDL_RenderDrawRect (renderer, &compRect);
                                int compTextW, compTextH;
                                TTF_SizeText(libFont, comp.c_str(), &compTextW, &compTextH);
                                int compTx = compRect.x + (compRect.w - compTextW)/2;
                                int compTy = compRect.y + (compRect.h - compTextH)/2;
                                drawTxt(comp, compTx, compTy, {0,0,0,255}, libFont);
                                yOff += 28;
                                anyShown = true;
                            }
                        }
                        yOff += 10;
                    }
                }
                if (!anyShown && !searchFilter.empty()) {
                    drawTxt("No results", 10, yOff, {150,0,0 ,255});
                    yOff += 22;
                }

                if (!selLibItm.empty( )) {
                    previewRect = {5, yOff+5, pnlLW-10, 60};
                    drawCompPreviewLib(selLibItm, previewRect);
                    yOff += 70;
                }


                yOff += 15;
                int titleY = yOff;
                drawTxt("Active Comps", 8, titleY, {0,0,0,255});
                yOff += 25;
                activeCompsY = yOff;
                for (size_t i=0; i<activeComps.size(); i++) {
                    SDL_Rect r = {5, yOff + (int)i*26, pnlLW-10, 20};
                    SDL_SetRenderDrawColor(renderer, 235, 230, 245, 255);
                    SDL_RenderFillRect(renderer, &r);
                    SDL_SetRenderDrawColor(renderer, 150,150,150,255);
                    SDL_RenderDrawRect(renderer, &r);
                    int textW, textH;
                    TTF_SizeText(libFont, activeComps[i].c_str(), &textW, &textH);
                    int textX = r.x + (r.w - textW) / 2;
                    int textY = r.y + (r.h - textH) / 2;
                    drawTxt(activeComps[i], textX, textY, {0,0,0,255}, libFont);
                    SDL_SetRenderDrawColor(renderer, 200,100,100,255);
                    int xY = r.y + (r.h - 10) / 2;
                    SDL_Rect xRect = {r.x+r.w-15, xY, 10,10};
                    SDL_RenderDrawRect(renderer, &xRect);
                }
            }

            if (showProp){
                int px = winW -pnlRW;
                SDL_Rect propBg = {px, tlbrH, pnlRW , vpH};
                SDL_SetRenderDrawColor(renderer, 225,230,240,255); SDL_RenderFillRect(renderer, &propBg);
                SDL_SetRenderDrawColor(renderer, 170,170,180,255);
                SDL_RenderDrawLine (renderer, px, tlbrH, px, tlbrH + vpH);
                int propTitleW, propTitleH;
                TTF_SizeText(font, "Properties", &propTitleW, &propTitleH);
                int propTitleX = px +(pnlRW - propTitleW) /2;
                drawTxt("Properties", propTitleX, tlbrH+5, {0,0,0,255});

                if (editingIndex < placedComponents.size() && propLabelInput && propValueInput && btnPropOK && btnPropCancel){
                    propLabelInput->setBox (px + 10, tlbrH + 90, pnlRW - 20, 25);
                    propValueInput->setBox(px + 10, tlbrH + 160, pnlRW - 20, 25);
                    btnPropOK->setRect(px + 10, tlbrH + 250, 60, 25);
                    btnPropCancel->setRect(px+ 75, tlbrH + 250, 60, 25);

                    propLabelInput->draw(renderer);
                    propValueInput->draw(renderer);
                    btnPropOK->draw (renderer);
                    btnPropCancel->draw(renderer);

                    drawTxt("Label", px+10, tlbrH+70 , {0,0,0,255}, libFont);
                    drawTxt("Value", px+10, tlbrH+140, {0,0,0,255}, libFont);
                } else if (!selLibItm.empty()) {
                    int itemW, itemH;
                    TTF_SizeText(font, selLibItm.c_str(), &itemW, &itemH);
                    int itemX = px + (pnlRW - itemW)/ 2;
                    drawTxt(selLibItm, itemX, tlbrH +45, {0,0,0,255});
                }
            }

            SDL_Rect vpRect = {vpX, vpY, vpW, vpH};

            int cLeft  = wldToScrX(worldMinX);
            int cTop  = wldToScrY(worldMaxY);
            int cRight  = wldToScrX(worldMaxX);
            int cBottom = wldToScrY(worldMinY);
            SDL_Rect canvasScreenRect= {cLeft, cTop, cRight - cLeft, cBottom - cTop};

            SDL_Rect canvasClip;
            SDL_IntersectRect (&vpRect, &canvasScreenRect, &canvasClip);
            SDL_RenderSetClipRect(renderer, &canvasClip);

            drawGrid();
            for (size_t i = 0; i < placedComponents.size(); ++i) {
                const auto& pc =placedComponents[i];
                int sx = wldToScrX(pc.x);
                int sy = wldToScrY(pc.y);
                bool isSelected = find(selectedIndices.begin() , selectedIndices.end(), i) != selectedIndices.end();
                if (isSelected) {
                    drawRotatedSelection(pc) ;
                }
                drawCompCanvas(pc);
                SDL_Point corners[4];
                getCompCorners(pc, corners) ;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
                SDL_Point lines [5];
                for (int c = 0; c < 4; c++) lines[c] = corners[c];
                lines[4] = corners[0] ;
                SDL_RenderDrawLines(renderer, lines, 5);
                if (!pc.label.empty()) {
                    drawTxt(pc.label, sx - 30, sy + 20, {0,0,0,255}, libFont );
                }
                if (!pc.value.empty()) {
                    drawTxt(pc.value, sx - 36, sy + 34, {70,70,70,255}, libFont );
                }
                auto pins = gCompPinPos(pc);
                const ComponentRuntime* runtime = findRuntime(pc.id);
                for (size_t pi = 0; pi < pins.size(); ++pi) {
                    double voltage = runtime && pi < runtime->pinVoltages.size() ? runtime->pinVoltages[pi] : std::numeric_limits<double>::quiet_NaN();
                    LogicLevel level = LogicStandard::fromVoltage(voltage);
                    if (level == LogicLevel::HIGH) SDL_SetRenderDrawColor(renderer, 220, 30, 30, 230);
                    else if (level == LogicLevel::LOW) SDL_SetRenderDrawColor(renderer, 30, 80, 220, 230);
                    else SDL_SetRenderDrawColor(renderer, 220, 150, 20, 230);
                    int px = wldToScrX(pins[pi].x);
                    int py = wldToScrY(pins[pi].y);
                    SDL_Rect pinRect = {px - 3, py - 3, 6, 6};
                    SDL_RenderFillRect(renderer, &pinRect);
                }
            }

            if (hoveredPinActive && currentTool == Tool::WIRE) {
                int hx = wldToScrX(hoveredPin.x);
                int hy = wldToScrY(hoveredPin.y);
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 200);
                for (int r = 5; r <= 7; ++r) {
                    SDL_Rect dot = {hx - r, hy - r, r*2, r*2};
                    SDL_RenderDrawRect(renderer, &dot);
                }
            }

            for (size_t wi = 0; wi < wires.size(); ++wi) {
                double voltage = wi < wireVoltages.size() ? wireVoltages[wi] : std::numeric_limits<double>::quiet_NaN();
                LogicLevel level = LogicStandard::fromVoltage(voltage);
                if (level == LogicLevel::HIGH) SDL_SetRenderDrawColor(renderer, 220, 35, 35, 220);
                else if (level == LogicLevel::LOW) SDL_SetRenderDrawColor(renderer, 25, 100, 210, 220);
                else SDL_SetRenderDrawColor(renderer, 110, 110, 110, 190);
                const auto& wire = wires[wi];
                for (size_t si = 0; si + 1 < wire.size(); ++si) {
                    int x1 = wldToScrX(wire[si].x);
                    int y1 = wldToScrY(wire[si].y);
                    int x2 = wldToScrX(wire[si+1].x);
                    int y2 = wldToScrY(wire[si+1].y);
                    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                }
            }

            for (auto& jpt : junctions){
                int sx = wldToScrX(jpt.x);
                int sy = wldToScrY (jpt.y);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                for (int r = 3; r <= 4; ++r){
                    SDL_Rect dot = {sx - r, sy - r, r*2, r*2} ;
                    SDL_RenderFillRect(renderer, &dot);
                }
            }

            if (wireStartActive) {
                int mx = snapToGrid(scrToWldX(mseX));
                int my = snapToGrid(scrToWldY(mseY));
                clampToCanvas(mx, my);
                SDL_Point cur = {mx, my};
                vector<SDL_Point> preview = calcOrthoPath(wireStartPoint, cur);
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 120);
                for (size_t i = 0; i < preview.size() - 1; ++i) {
                    int x1 = wldToScrX(preview[i].x), y1 = wldToScrY(preview[i].y);
                    int x2 = wldToScrX(preview[i+1].x), y2 = wldToScrY(preview[i+1].y);
                    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                }
            }

            if (drawingSelection && selectionRect.w > 0 && selectionRect.h > 0) {
                SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 120, 215, 30);
                SDL_RenderFillRect (renderer, &selectionRect);
                SDL_SetRenderDrawColor(renderer, 0, 120, 215, 80);
                SDL_RenderDrawRect(renderer, &selectionRect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }

            SDL_RenderSetClipRect(renderer,&vpRect);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer,&canvasScreenRect);

            SDL_RenderSetClipRect( renderer, nullptr);

            drawTxt("Workspace - Canvas: " +to_string(canvasWidth) + "x" + to_string(canvasHeight), vpX+10, vpY+10, {0, 0, 0, 255});
            drawStatusBar() ;
        }

        SDL_RenderPresent(renderer);
    }

    bool active() const{
        return running;
    }
};


int main(){

    PROTEUS app;

    if ( !app.initial() ){
        return -1;
    }

    while (app.active()) {
        app.handleE();
        app.render ();
        SDL_Delay(16);
    }


    return 0;
}