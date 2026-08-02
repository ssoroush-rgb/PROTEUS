#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <cmath>




using namespace std;

struct placedComp {

    string name;
    int x, y;
    int angle;
    bool flipH,flipV;
    string label;
    string value;
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
        :font(f), renderer(rend), active(false), numericOnly (numOnly), text(""), txtTexture(nullptr) {
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
    SDL_Rect selectionRect;
    bool drawingSelection;

    bool mouseHandled;

    int wldToScrX(int wx) const { return vpX+ (int)(wx * zmLvl + panX);}
    int wldToScrY(int wy) const { return vpY + (int)(panY + (canvasHeight - wy) * zmLvl);}
    int scrToWldX(int sx) const { return (int)((sx - vpX - panX) / zmLvl); }
    int scrToWldY (int sy) const { return canvasHeight - (int)((sy - vpY - panY) / zmLvl) ;}

    int snapToGrid (int val) const {return ((val + grdSz /2) / grdSz) * grdSz;}

    void clampToCanvas (int& x, int& y) const {
        if (x < 0) x= 0;
        if (x > canvasWidth) x = canvasWidth;
        if (y < 0) y= 0;
        if (y > canvasHeight) y =canvasHeight;
    }

    void updateViewport (){
        int leftPanelWidth = showLib ? pnlLW :0;
        int rightPanelWidth = showProp ? pnlRW :0;
        vpX = leftPanelWidth;
        vpW = winW -leftPanelWidth - rightPanelWidth;
        vpY = tlbrH;
        vpH = winH - tlbrH - statH;
    }

    void fitWindowToCanvas() {
        int newW = canvasWidth + (showLib? pnlLW : 0) + (showProp ? pnlRW : 0);
        int newH  = canvasHeight + tlbrH + statH;
        if (newW < 400) newW = 400 ;
        if (newH < 300) newH = 300 ;
        SDL_SetWindowSize(window, newW, newH);
        winW = newW;winH = newH;
        SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        updateViewport();
    }

    enum UndoType {COMP_PLACE, COMP_DELETE , COMP_MOVE, ACTIVE_ADD, ACTIVE_REMOVE, COMP_EDIT, COMP_TRANSFORM};
    struct UndoAction{
        UndoType type;
        placedComp comp;
        placedComp oldComp;
        string compName;
        int activeIndex;
        vector<placedComp> compsBefore;
        vector<placedComp> compsAfter ;
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
            redoStack.push_back(act);
        }
        else if (act.type== COMP_DELETE) {
            placedComponents.push_back(act.comp);
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
            redoStack.push_back(act);
        }
        else if (act.type == COMP_TRANSFORM) {
            placedComponents = act.compsBefore;
            redoStack.push_back (act);
        }
        else if (act.type == COMP_EDIT) {
            size_t idx = -1;
            for(size_t i = 0; i < placedComponents.size(); ++i){
                if (placedComponents[i].name== act.oldComp.name && placedComponents[i].x == act.oldComp.x && placedComponents[i].y == act.oldComp.y) {
                    idx = i;
                    break;
                }
            }
            if (idx < placedComponents.size() ) {
                placedComponents[idx] =act.comp;
                UndoAction redoAct;
                redoAct.type = COMP_EDIT;
                redoAct.oldComp = act.comp;
                redoAct.comp = act.oldComp;
                redoStack.push_back (redoAct);
            }
        }
    }

    void redo() {
        if (redoStack.empty()) return;
        UndoAction act = redoStack.back();
        redoStack.pop_back();
        if (act.type == COMP_PLACE){
            placedComponents.push_back(act.comp) ;
            undoStack.push_back(act);
        }
        else if (act.type == COMP_DELETE) {
            for(size_t i = 0; i< placedComponents.size(); ++i) {
                if (placedComponents[i].name == act.comp.name && placedComponents [i].x == act.comp.x && placedComponents[i].y == act.comp.y) {
                    placedComponents.erase(placedComponents.begin()+ i);
                    break;
                }
            }
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
            undoStack.push_back(act); 
        }
        else if (act.type == COMP_TRANSFORM ) { 
            placedComponents = act.compsAfter; 
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
    }

    void resetView() {
        zmLvl =1.0f;
        panX = 1;
        panY = (winH -tlbrH - statH) - canvasHeight - 1;
    }

    void drawGrid() {
        if (!showGrid)
            return ;
        int wL = scrToWldX(vpX), wT = scrToWldY (vpY);
        int wR= scrToWldX(vpX+vpW), wB = scrToWldY(vpY+vpH);
        int startX = (wL / grdSz) * grdSz ;
        int startY = (wT / grdSz) * grdSz;
        SDL_SetRenderDrawColor(renderer, 200,200,200, 60);
        for (int x = startX; x <= wR ; x += grdSz) {
            int sx = wldToScrX(x);
            if (sx >= vpX && sx < vpX+vpW)
                SDL_RenderDrawLine(renderer, sx, vpY, sx, vpY+vpH);
        }
        for (int y = startY; y >= wB ;y -= grdSz){
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

    vector <SDL_Point> gCompPinPos(const placedComp& comp) const {
        vector<SDL_Point> pins;
        auto transform = [&] (int lx, int ly) -> pair<int,int> {
            int sx = comp.flipH ? -1 : 1;
            int sy = comp.flipV ? -1 : 1;
            int rx = lx *sx;
            int ry = ly * sy;
            float rad = comp.angle * 3.14159f / 180.0f;
            float s = std::sin (rad), c = std::cos(rad);
            int fx = (int)(rx * c -ry * s);
            int fy = (int)(rx * s +ry * c);
            return {comp.x + fx, comp.y + fy};
        };
        if (comp.name == "Resistor") {
            auto p1 = transform(-32, 0);  auto p2 = transform(32, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
        }
        else if (comp.name == "Capacitor") {
            auto p1 = transform(0, -18); auto p2 = transform(0, 18);
            pins.push_back ({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
        }
        else if (comp.name =="Inductor") {
            auto p1 = transform(-27, 0); auto p2 = transform(27, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
        }
        else if (comp.name == "LED") {
            auto p1 = transform(0, -12); auto p2 = transform(0, 12);
            pins.push_back({p1.first, p1.second});pins.push_back({p2.first, p2.second});
        }
        else if (comp.name == "Transistor" || comp.name == "NPN"|| comp.name == "PNP") {
            auto p1 = transform(0, -14); auto p2 = transform(0, 14); auto p3= transform(-12, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second}); pins.push_back({p3.first, p3.second});
        }
        else if (comp.name == "Ground") {
            auto p1 = transform(0, - 12);
            pins.push_back({p1.first, p1.second});
        }
        else if (comp.name == "VCC") {
            auto p1 = transform(0, -14);
            pins.push_back({p1. first, p1.second});
        }
        else if (comp.name == "Battery") {
            auto p1  = transform(0, -14); auto p2 = transform(0, 14);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
        }
        else if (comp.name == "AND Gate"){
            auto p1 = transform(-26, -8); auto p2 = transform(-26, 8); auto p3 = transform(18, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back( {p2.first, p2.second}); pins.push_back({p3.first, p3.second});
        }
        else if(comp.name == "OR Gate") {
            auto p1 = transform( -22, -8); auto p2 = transform(-22, 8); auto p3 = transform(18, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second}); pins.push_back({p3.first, p3.second});
        }
        else if (comp.name == "NOT Gate" ) {
            auto p1 = transform(-18, 0); auto p2 = transform(22, 0);
            pins.push_back({p1.first, p1.second}) ; pins.push_back({p2.first, p2.second});
        }
        else if (comp.name == "7-Segment"){
            auto p1 = transform(-16, -10); auto p2 = transform(-16, -2);
            auto p3 = transform(-16, 6);  auto p4 = transform(-16, 14);
            auto p5 = transform(16, -10); auto p6 = transform(16, -2);
            auto p7 = transform(16, 6);   auto p8 = transform (16, 14);
            pins.push_back ({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
            pins.push_back({p3.first, p3.second}); pins.push_back({p4.first, p4.second});
            pins.push_back({p5.first, p5.second}); pins.push_back({p6.first, p6.second});
            pins.push_back({p7.first, p7.second}); pins.push_back({p8.first, p8.second});
        }
        else {
            auto p1 = transform(-15 , 0); auto p2 = transform(15, 0);
            pins.push_back({p1.first, p1.second}); pins.push_back({p2.first, p2.second});
        }
        return pins;
    }

    void drawCompPreview(const string& compName, SDL_Rect area,const placedComp& comp, bool drawBg = true){
        if (drawBg){
            SDL_SetRenderDrawColor(renderer, 245,245,245,255);
            SDL_RenderFillRect(renderer , &area);
            SDL_SetRenderDrawColor(renderer, 100,100,100,255) ;
            SDL_RenderDrawRect(renderer, &area);
        }

        int cx = area.x + area.w / 2, cy = area.y + area.h/ 2;
        auto rotatePoint = [&](int px, int py, float rad) -> pair<int,int> {
            float s = std::sin(rad) , c = std::cos(rad);
            int dx = px - cx, dy = py - cy;
            int rx = (int)(dx * c - dy * s ) + cx;
            int ry = (int)(dx * s + dy * c ) + cy;
            return {rx, ry};
        };
        float rad = comp.angle *3.14159f / 180.0f;
        int sx = comp.flipH ? -1 : 1;
        int sy = comp.flipV ? -1 : 1;
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
        else if (compName == "7-Segment") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            auto top    = transform(cx, cy-14);
            auto topL   = transform(cx-12, cy-10);
            auto topR   = transform(cx+12, cy-10);
            auto mid    = transform(cx, cy);
            auto botL   = transform(cx-12, cy+10);
            auto botR   = transform(cx+12, cy+10);
            auto bot    = transform(cx, cy+ 14);
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
            file << pc.name << "|" << pc.x << "|" << pc.y << "|"<< pc.angle << "|" << pc.flipH << "|" << pc.flipV << "|" << pc.label << "|" << pc.value << "\n";
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
        placedComponents.clear() ;
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
                clampToCanvas (pc.x, pc.y);
                placedComponents.push_back(pc);
            }
        }
        file.close();
        selectedIndices.clear();
        editingIndex = -1;
        lastClickedIndex = -1;
        lastClickTime = 0;
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
        panX = 1;
        panY = (winH - tlbrH - statH)- canvasHeight - 1;
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

        libItms = {"Resistor","Capacitor","LED","Transistor","Ground","VCC"};
        for (size_t i=0; i<libItms.size(); i++) {
            libRcts.push_back({5, tlbrH + 55 + (int)i*40, pnlLW-10, 28});
        }

        searchBox = nullptr;
        searchFilter ="";
        showLib = true;

        libCategories.push_back(tree("Analog", {"Resistor","Capacitor","Inductor"}));
        libCategories.push_back(tree("Digital", {"AND Gate","OR Gate","NOT Gate"}));
        libCategories.push_back(tree("Power", {"Ground","VCC","Battery"}));
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
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr <<"SDL Init failed: " <<SDL_GetError() << endl;
            return false;
        }
        if (TTF_Init() ==-1){
            cerr << "SDL_ttf Init failed: " << TTF_GetError() << endl;
            return false;
        }
        window =SDL_CreateWindow("Proteus Clone - Startup Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 850, 600, SDL_WINDOW_SHOWN| SDL_WINDOW_RESIZABLE);
        if (!window)
            return false;
        renderer =SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED);
        if (!renderer)
            return false;
        font = TTF_OpenFont ("arial.ttf", 20);
        if (!font) {
            cerr << "Warning: Failed to load arial.ttf" << endl;
        }
        titleFont = TTF_OpenFont("arial.ttf", 30);
        libFont =TTF_OpenFont("arial.ttf", 14);

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
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 105,24,{255,255,255,255},{230,230,230,255}, "Comp/Lib")) ; xpos+=115;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Prop")); xpos += 65;
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
                            resetView();
                            resetLibExpanded();
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
                                currentProjectPath = path;
                            }
                            fitWindowToCanvas();
                            SDL_SetWindowMinimumSize(window, 400, 300);
                            SDL_SetWindowMaximumSize(window, 0, 0) ;
                            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                            resetView();
                            resetLibExpanded ();
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
                    if (txtWidth) txtWidth->setActive (false); if (txtHeight) txtHeight->setActive(false);
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
                    undoStack.clear() ;
                    redoStack.clear();
                    selectedIndices.clear();
                    editingIndex =-1;
                    lastClickedIndex = -1;
                    lastClickTime = 0;
                    addToRecent(project(finalName, path, gDate( ), canvasWidth, canvasHeight, activeComps));
                    saveProjectToFile (path);
                    currentProjectPath = path;
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    fitWindowToCanvas ();
                    SDL_SetWindowMinimumSize(window, 400, 300);
                    SDL_SetWindowMaximumSize(window, 0, 0);
                    SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    resetView();
                    resetLibExpanded();
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
                    selectedIndices. clear();
                    mouseHandled =true;
                }

                for (auto b : tlbrBtns) b->events(ev);
                if (!mouseHandled && tlbrBtns[0]-> click(ev)) {
                    currentTool = Tool::SELECT; selLibItm = "";
                    mouseHandled = true;
                    cout << "Select tool\n";
                }
                else if(!mouseHandled && tlbrBtns[1]->click(ev)) {
                    currentTool = Tool::WIRE; selLibItm = "";
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
                    updateViewport() ;
                    mouseHandled = true;
                    cout<< "Component Library toggled\n";
                }
                else if (!mouseHandled && tlbrBtns [3]->click(ev)) {
                    showProp = !showProp;
                    if (!showProp) editingIndex = -1;
                    updateViewport() ;
                    mouseHandled = true;
                    cout << "Properties panel toggled\n";
                }
                else if (!mouseHandled && tlbrBtns [4]->click(ev)){
                    if (!currentProjectPath.empty()) {
                        saveProjectToFile (currentProjectPath);
                        addToRecent(project(projNameFromPath(currentProjectPath ), currentProjectPath, gDate(), canvasWidth, canvasHeight, activeComps));
                        cout << "Project saved.\n";
                    }
                    selLibItm = "";
                    mouseHandled = true;
                }
                else if(!mouseHandled && tlbrBtns[5]->click(ev)) {
                    string chosen = fileDialog();
                    if (!chosen.empty ()) {
                        if (loadProjectFromFile(chosen)) {
                            currentProjectPath = chosen;
                            fitWindowToCanvas();
                            resetView();
                            resetLibExpanded();
                            selLibItm = "" ;
                            currentTool = Tool::SELECT;
                            SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
                            currentState = app::WORKSPACE;
                            cout << "Loaded: " << chosen << endl;
                        }
                    }
                    selLibItm = "";
                    mouseHandled =true;
                }
                else if (!mouseHandled && tlbrBtns[6]->click (ev)) {
                    undo();
                    mouseHandled = true;
                    cout << "Undo\n";
                }
                else if (!mouseHandled && tlbrBtns[7]->click(ev)) {
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
                            else if (currentTool == Tool::COMPONENT && !selLibItm.empty()){
                                int wx = snapToGrid(scrToWldX(mseX));
                                int wy = snapToGrid(scrToWldY(mseY));
                                clampToCanvas (wx, wy);
                                placedComp pc ={selLibItm, wx, wy, 0, false, false, "", ""} ;
                                placedComponents.push_back(pc);
                                UndoAction act;
                                act.type = COMP_PLACE;
                                act.comp = pc ;
                                pushUndo(act);
                            } else {
                                bool hitComponent =false;
                                for (size_t i = 0; i < placedComponents.size(); ++i) {
                                    int sx = wldToScrX (placedComponents[i].x);
                                    int sy = wldToScrY( placedComponents[i].y);
                                    SDL_Rect compRect = { sx - 40, sy - 20, 80, 40};
                                    if (mseX >= compRect.x && mseX < compRect.x +compRect.w && mseY >= compRect.y && mseY < compRect.y +compRect.h) {
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
                                    drawingSelection = true;
                                    selectionRect = {mseX, mseY, 0, 0} ;
                                    lastClickedIndex = -1;
                                    lastClickTime = 0;
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
                        for (size_t i = 0; i < placedComponents.size(); ++i) {
                            int sx = wldToScrX(placedComponents[i].x);
                            int sy = wldToScrY(placedComponents[i].y);
                            SDL_Rect compRect = { sx - 40, sy - 20, 80, 40};
                            if (mseX >= compRect.x && mseX < compRect.x + compRect.w &&
                                mseY >= compRect.y && mseY < compRect.y + compRect.h) {
                                UndoAction act;
                                act.type =  COMP_DELETE;
                                act.comp = placedComponents[i];
                                pushUndo(act);
                                placedComponents.erase (placedComponents.begin() + i);
                                selectedIndices.clear();
                                if (editingIndex == i) editingIndex= -1;
                                break;
                            }
                        }
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                    if (draggingComponents){
                        if (dragStartX != mseX ||dragStartY != mseY){
                            UndoAction act;
                            act.type = COMP_MOVE;
                            act.compsBefore= dragSnapshots;
                            act.compsAfter = placedComponents;
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
                        int deltaWY = scrToWldY(mseY) -scrToWldY(dragStartY);
                        for (size_t j = 0; j < selectedIndices.size(); ++j) {
                            size_t idx = selectedIndices [j];
                            placedComponents[idx].x = snapToGrid(dragSnapshots[j].x+ deltaWX);
                            placedComponents[idx].y = snapToGrid(dragSnapshots[j].y+ deltaWY);
                        }
                    } else if (drawingSelection){
                        int x = min(mseX, selectionRect.x);
                        int y = min(mseY, selectionRect.y);
                        int w = abs(mseX - selectionRect.x) ;
                        int h = abs(mseY - selectionRect.y);
                        selectionRect = {x, y, w, h};
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
                                resetView();
                                resetLibExpanded();
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
                                    pushUndo(act);
                                    placedComponents.erase(placedComponents.begin() + i);
                                    if (editingIndex == i) editingIndex = -1;
                                }
                            }
                            selectedIndices.clear();
                        }
                        else if(ev.key.keysym.sym == SDLK_r) {
                            vector<placedComp>before = placedComponents;
                            for (size_t idx : selectedIndices){
                                placedComponents[idx].angle = (placedComponents[idx].angle + 90) %360;
                            }
                            UndoAction act;
                            act.type = COMP_TRANSFORM ;
                            act.compsBefore = before;
                            act.compsAfter = placedComponents;
                            pushUndo(act);
                        }
                        else if (ev.key.keysym.sym== SDLK_h){
                            vector <placedComp> before = placedComponents;
                            for (size_t idx: selectedIndices) {
                                placedComponents[idx].flipH = !placedComponents[idx].flipH;
                            }
                            UndoAction act;
                            act.type =COMP_TRANSFORM;
                            act.compsBefore = before;
                            act.compsAfter =placedComponents;
                            pushUndo(act);
                        }
                        else if(ev.key.keysym.sym == SDLK_v) {
                            vector<placedComp> before = placedComponents;
                            for (size_t idx : selectedIndices) {
                                placedComponents[idx].flipV = !placedComponents[idx].flipV;
                            }
                            UndoAction act;
                            act.type = COMP_TRANSFORM;
                            act.compsBefore= before;
                            act.compsAfter = placedComponents;
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
                    placedComp dummy= {selLibItm,0,0,0,false,false,"",""};
                    drawCompPreview (selLibItm, previewRect, dummy);
                    yOff += 70;
                }


                yOff += 15;
                int titleY = yOff;
                drawTxt("Active Comps", 8, titleY, {0,0,0,255});
                yOff += 25;
                activeCompsY = yOff;
                for (size_t i=0; i<activeComps.size(); i++) {
                    SDL_Rect r = {5, yOff + (int)i*26, pnlLW-10, 20};
                    SDL_SetRenderDrawColor(renderer, selLibItm == activeComps[i] ? 180 : 235, 230, 245, 255);
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
            SDL_RenderSetClipRect (renderer, &vpRect);

            drawGrid();
            for (size_t i = 0; i < placedComponents.size(); ++i) {
                const auto& pc =placedComponents[i];
                int sx = wldToScrX(pc.x);
                int sy = wldToScrY(pc.y);
                SDL_Rect compRect = {sx- 40, sy - 20, 80, 40};
                bool isSelected = find(selectedIndices.begin() , selectedIndices.end(), i) != selectedIndices.end();
                if (isSelected) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 80);
                    SDL_RenderFillRect(renderer, &compRect );
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    SDL_RenderDrawRect(renderer, &compRect);
                }
                drawCompPreview (pc.name, compRect, pc, false) ;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
                SDL_RenderDrawRect(renderer, &compRect);
                if (!pc.label.empty()) {
                    drawTxt(pc.label, sx - 30, sy + 20, {0,0,0,255}, libFont );
                }
                auto pins = gCompPinPos(pc);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 200);
                for (const auto& p : pins) {
                    int px = wldToScrX(p.x) ;
                    int py = wldToScrY(p.y);
                    SDL_Rect pinRect = {px - 3, py - 3, 6, 6};
                    SDL_RenderFillRect(renderer, &pinRect);
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

            SDL_RenderSetClipRect(renderer , nullptr);

            drawTxt("Workspace - Canvas: " +to_string(canvasWidth) + "x" + to_string(canvasHeight), vpX+10, vpY+10, {0, 0, 0, 255});
            drawStatusBar() ;
        }

        SDL_RenderPresent(renderer);
    }

    bool active() const{
        return running;
    }
};


int main (int argc, char* argv[ ]){

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