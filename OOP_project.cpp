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



using namespace std;

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
        else{
            SDL_StopTextInput() ;
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

    SDL_Rect getRect() const { return rect;}
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

    struct placedComp {
        string name;
        int x, y;
    };
    vector<placedComp> placedComponents;                          

    int wldToScrX(int wx) const { return vpX+ (int)(wx * zmLvl + panX);}
    int wldToScrY(int wy) const { return vpY + (int)(panY + (canvasHeight - wy) * zmLvl);}
    int scrToWldX(int sx) const { return (int)((sx - vpX - panX) / zmLvl); }
    int scrToWldY (int sy) const { return canvasHeight - (int)((sy - vpY - panY) / zmLvl) ;}

    int snapToGrid (int val) const {return ((val + grdSz /2) / grdSz) * grdSz;}

    enum UndoType {COMP_PLACE, COMP_DELETE, ACTIVE_ADD, ACTIVE_REMOVE };
    struct UndoAction{
        UndoType type;
        placedComp comp;
        string compName;
        int activeIndex;
    };
    vector<UndoAction> undoStack;
    vector<UndoAction> redoStack;

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

    void drawCompPreview(const string& compName, SDL_Rect area, bool drawBg = true) {
        if (drawBg){
            SDL_SetRenderDrawColor(renderer, 245,245,245,255);
            SDL_RenderFillRect(renderer , &area);
            SDL_SetRenderDrawColor(renderer, 100,100,100,255) ;
            SDL_RenderDrawRect(renderer, &area);
        }

        int cx = area.x + area.w/2, cy = area.y + area.h/2;
        if (compName == "Resistor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int totalW = min(60, area.w - 10);
            int startX =area.x + (area.w - totalW) / 2;
            int x = startX, y = cy;
            for (int i=0; i<4; i++) {
                SDL_RenderDrawLine(renderer, x, y, x+ 8, y-8);
                x+=8; y-=8;
                SDL_RenderDrawLine(renderer, x, y, x+8, y+8);
                x+=8; y+=8;
            }
        }
        else if (compName == "Capacitor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawLine(renderer, cx-10, cy-15, cx -10, cy+15);
            SDL_RenderDrawLine(renderer, cx+10, cy-15, cx+10, cy+15);
        }
        else if (compName == "Inductor") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            int totalW = min (50, area.w - 10);
            int startX = area.x + (area.w - totalW) / 2;
            int x = startX, y = cy;
            for (int i=0; i< 5; i++) {
                SDL_RenderDrawLine(renderer, x, y, x+5, y-7);
                x+=5; y-=7 ;
                SDL_RenderDrawLine(renderer, x, y, x+5, y+7);
                x+=5; y+=7;
            }
        }
        else if (compName == "LED") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawLine(renderer, cx-8, cy-10, cx-8, cy+10);
            SDL_RenderDrawLine(renderer, cx-8, cy-10, cx+4, cy);
            SDL_RenderDrawLine(renderer, cx-8, cy+10, cx+4, cy) ;
            SDL_RenderDrawLine(renderer, cx +4, cy-10, cx+4, cy+10);
            SDL_RenderDrawLine(renderer, cx+10, cy-8, cx+6, cy-4);
            SDL_RenderDrawLine(renderer, cx+10,  cy-8, cx+6, cy-2);
            SDL_RenderDrawLine(renderer, cx+10, cy+8, cx+6, cy+4);
            SDL_RenderDrawLine(renderer, cx+10, cy+8, cx+6, cy+2);
        }
        else if (compName == "Transistor" || compName == "NPN" || compName == "PNP") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawLine(renderer, cx, cy-10, cx, cy+10);
            SDL_RenderDrawLine(renderer, cx-8, cy-4, cx+8, cy-8);
            SDL_RenderDrawLine(renderer, cx-8, cy+4, cx+8, cy+8);
            SDL_Rect circle = {cx-12, cy-12, 24,24};
            SDL_RenderDrawRect(renderer, &circle);
        }
        else if (compName == "Ground") {
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            SDL_RenderDrawLine(renderer, cx, cy-10, cx, cy);
            SDL_RenderDrawLine(renderer, cx-8, cy, cx+8, cy);
            SDL_RenderDrawLine(renderer, cx-5, cy+5, cx+5, cy+5);
            SDL_RenderDrawLine(renderer, cx-3, cy+10, cx+3, cy+10);
        }
        else if (compName == "VCC" || compName == "Battery") {
            SDL_SetRenderDrawColor( renderer, 200,0,0,255);
            SDL_RenderDrawLine(renderer,cx, cy-10, cx, cy+5);
            SDL_RenderDrawLine(renderer, cx-5, cy, cx+5, cy);
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
            file << pc.name << " " << pc.x << " " << pc.y << "\n";
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
                string cname; int cx, cy;
                if (file >> cname >> cx >> cy) {
                    placedComponents.push_back({cname, cx, cy});
                }
            }
        }
        file.close();
        return true;
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
                            SDL_SetWindowMinimumSize (window, 400, 300);
                            SDL_SetWindowMaximumSize (window, 0, 0);
                            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
                            resetView();
                            resetLibExpanded();
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
                            SDL_SetWindowMinimumSize(window, 400, 300);
                            SDL_SetWindowMaximumSize(window, 0, 0) ;
                            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                            resetView();
                            resetLibExpanded ();
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
                    addToRecent(project(finalName, path, gDate( ), canvasWidth, canvasHeight, activeComps));
                    currentProjectPath = path;
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    SDL_SetWindowMinimumSize(window, 400, 300);
                    SDL_SetWindowMaximumSize(window, 0, 0);
                    SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    resetView();
                    resetLibExpanded();
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
                btnBack->events(ev);
                if (btnBack->click(ev) ) {
                    currentState = app::STARTUP_MENU;
                    SDL_SetWindowMinimumSize( window, 850, 600);
                    SDL_SetWindowMaximumSize( window, 850, 600);
                    SDL_SetWindowSize(window, 850, 600);
                    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    selLibItm = "";
                }

                for (auto b : tlbrBtns) b->events(ev);
                if (tlbrBtns[0]->click(ev)) {
                    currentTool = Tool::SELECT; selLibItm = "";
                    cout << "Select tool\n";
                }
                else if (tlbrBtns[1]->click(ev)) {
                    currentTool = Tool::WIRE; selLibItm = "";
                    cout << "Wire tool\n";
                }
                else if (tlbrBtns[2]->click(ev)) {
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
                    cout<< "Component Library toggled\n";
                }
                else if (tlbrBtns [3]->click(ev)) {
                    showProp = !showProp;
                    cout << "Properties panel toggled\n";
                }
                else if (tlbrBtns[4]->click(ev)){
                    if (!currentProjectPath.empty()) {
                        saveProjectToFile (currentProjectPath);
                        addToRecent(project(projNameFromPath(currentProjectPath ), currentProjectPath, gDate(), canvasWidth, canvasHeight, activeComps));
                        cout << "Project saved.\n";
                    }
                    selLibItm = "";
                }
                else if (tlbrBtns[5]->click(ev)) {
                    string chosen = fileDialog();
                    if (!chosen.empty ()) {
                        if (loadProjectFromFile(chosen)) {
                            currentProjectPath = chosen;
                            resetView();
                            resetLibExpanded();
                            currentState = app::WORKSPACE;
                            cout << "Loaded: " << chosen << endl;
                        }
                    }
                    selLibItm = "";
                }
                else if (tlbrBtns[6]->click (ev)) {
                    undo();
                    cout << "Undo\n";
                }
                else if (tlbrBtns[7]->click(ev)) {
                    redo();
                    cout << "Redo \n";
                }

                if (showLib) {
                    searchBox-> handleEvent(ev);
                    if (searchBox->isActive())
                        searchFilter = searchBox->gTxt();
                    if (!searchBox->isActive() && searchBox->gTxt().empty())
                        searchBox->setTxt("Search...");
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
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    if (showLib && searchBox && searchBox->mouseIn(ev.button.x, ev.button.y)) {
                        searchBox->setActive(true);
                        if (searchBox->gTxt()== "Search...")
                            searchBox->setTxt("");
                    } else {
                        if (searchBox && searchBox->isActive()) {
                            searchBox->setActive(false);
                            if (searchBox->gTxt(). empty())
                                searchBox->setTxt("Search...");
                        }
                    }

                    if (showLib && activeCompsY > 0 && mseX >= 0 && mseX< pnlLW &&
                        mseY >= activeCompsY && mseY < activeCompsY + (int)activeComps.size() * 26) {
                        int index = (mseY- activeCompsY) / 26;
                        if (index >= 0 && index <(int)activeComps.size()) {
                            SDL_Rect r = {5, activeCompsY + index * 26, pnlLW - 10, 20};
                            SDL_Rect xRect = {r.x + r.w - 15, r.y + (r.h - 10)/2, 10, 10};
                            if (mseX >= xRect.x && mseX  <= xRect.x + xRect.w &&
                                mseY >= xRect.y && mseY <= xRect.y + xRect.h) {
                                UndoAction  act;
                                act.type = ACTIVE_REMOVE;
                                act.compName = activeComps[index];
                                act.activeIndex = index;
                                pushUndo(act);
                                activeComps.erase(activeComps.begin() + index);
                            } else if (mseX >= r.x && mseX<= r.x + r.w && mseY >= r.y && mseY <= r.y + r.h) {
                                selLibItm = activeComps [index];
                                currentTool = Tool::COMPONENT;
                            }
                        }
                    }

                    if (showLib && mseX >=0 && mseX < pnlLW && mseY >= tlbrH && mseY < winH - statH) {
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
                    else if (mseX >= zoomRct.x && mseX <= zoomRct.x + zoomRct.w &&
                        mseY >= zoomRct.y && mseY<=zoomRct.y + zoomRct.h) {
                        resetView();
                        selLibItm = "";
                    }
                    else if (mseX >= vpX && mseX < vpX+vpW && mseY >= vpY && mseY < vpY+ vpH) {
                        if (!btnBack-> click(ev)) {
                            if (currentTool == Tool::COMPONENT && !selLibItm.empty()) {
                                int wx = snapToGrid(scrToWldX(mseX));
                                int wy = snapToGrid(scrToWldY(mseY));
                                placedComp pc ={selLibItm, wx, wy};
                                placedComponents.push_back(pc);
                                UndoAction act;
                                act.type = COMP_PLACE;
                                act.comp = pc ;
                                pushUndo(act);
                            } else {
                                isPan = true ;
                                panStartX = ev.button.x ; panStartY = ev.button.y;
                                panOffXst = panX; panOffYst = panY;
                            }
                        }
                        selLibItm ="";
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_RIGHT) {
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
                                break;
                            }
                        }
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                    isPan = false;
                }
                if (isPan && ev.type == SDL_MOUSEMOTION) {
                    panX = panOffXst + (ev.motion.x- panStartX);
                    panY = panOffYst + (ev.motion.y- panStartY);
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

                if (ev.type == SDL_KEYDOWN) {
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
                        cout << "Delete selected item (placeholder)\n";
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
                    else if (ev.key.keysym.sym == SDLK_0 && SDL_GetModState() & KMOD_CTRL) {
                        resetView ();
                        selLibItm = "";
                    }
                    else if (ev.key.keysym.sym == SDLK_s && SDL_GetModState() & KMOD_CTRL) {
                        if (!currentProjectPath.empty()) {
                            saveProjectToFile(currentProjectPath) ;
                            addToRecent(project(projNameFromPath(currentProjectPath), currentProjectPath, gDate(), canvasWidth, canvasHeight, activeComps));
                            cout << "Project saved (Ctrl+S).\n";
                        }
                        selLibItm = "" ;
                    }
                    else if (ev.key.keysym.sym == SDLK_o && SDL_GetModState() & KMOD_CTRL) {
                        string chosen = fileDialog();
                        if (!chosen.empty()) {
                            if (loadProjectFromFile(chosen)) {
                                currentProjectPath = chosen;
                                resetView();
                                resetLibExpanded();
                                currentState = app::WORKSPACE;
                                cout << "Loaded: " << chosen << endl;
                            }
                        }
                        selLibItm = "";
                    }
                    else if (ev.key.keysym.sym == SDLK_z && SDL_GetModState()  & KMOD_CTRL) {
                        undo ();
                        cout << "Undo (Ctrl+Z)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_y && SDL_GetModState() & KMOD_CTRL) {
                        redo();
                        cout << "Redo (Ctrl+Y)\n";
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
                    drawCompPreview(selLibItm, previewRect);
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
                SDL_Rect propBg = {winW - pnlRW, tlbrH, pnlRW , vpH};
                SDL_SetRenderDrawColor(renderer, 225,230,240,255); SDL_RenderFillRect(renderer, &propBg);
                SDL_SetRenderDrawColor(renderer, 170,170,180,255);
                SDL_RenderDrawLine (renderer, winW - pnlRW, tlbrH, winW - pnlRW, tlbrH+vpH);
                int propTitleW, propTitleH;
                TTF_SizeText(font, "Properties", &propTitleW, &propTitleH);
                int propTitleX = winW - pnlRW +(pnlRW - propTitleW)/2;
                drawTxt("Properties", propTitleX, tlbrH+5, {0,0,0,255});
                if (!selLibItm.empty()) {
                    int itemW, itemH;
                    TTF_SizeText(font, selLibItm.c_str(), &itemW, &itemH);
                    int itemX = winW - pnlRW + (pnlRW - itemW)/2;
                    drawTxt(selLibItm, itemX, tlbrH +45, {0,0,0,255});
                }
            }

            drawGrid();
            for (const auto& pc : placedComponents) {
                int sx = wldToScrX(pc.x);
                int sy = wldToScrY(pc.y);
                SDL_Rect compRect = {sx- 40, sy - 20, 80, 40};
                drawCompPreview (pc.name, compRect, false);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 80);
                SDL_RenderDrawRect(renderer, &compRect);
            }
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