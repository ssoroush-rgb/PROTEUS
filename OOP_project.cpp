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

    project(string n, string p , string date, int cw = 800, int ch = 600)
        : name(n), path(p), lastP(date), canvasW(cw), canvasH(ch) {}
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
            textRect.y = box.y + 3;
            SDL_QueryTexture(txtTexture, nullptr, nullptr,&textRect.w, &textRect.h);
            SDL_RenderCopy(rend, txtTexture, nullptr, &textRect);
        }
    }

    string gTxt() const { return text; }

    bool isActive() const {return active; }

    bool mouseIn (int mx, int my) const {
        return (mx >= box.x && mx <= box.x + box.w &&
                my >= box.y && my <= box.y + box.h );
    }

};


class BUTTONS {
private:
    SDL_Rect rect;
    SDL_Color normalColor;
    SDL_Color hoverColor;
    bool hover;

    SDL_Texture* textTexture;
    SDL_Rect textRect;

public:
    BUTTONS(SDL_Renderer* renderer, TTF_Font* font, int x,int y, int w, int h,
            SDL_Color nColor, SDL_Color hColor, string text) {

        rect  ={x, y, w, h};
        normalColor = nColor;
        hoverColor = hColor;
        hover = false ;
        textTexture = nullptr;


        if (font) {
            SDL_Color textColor = {20, 20, 20, 255} ;
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str (), textColor);

            if (textSurface) {
                textTexture= SDL_CreateTextureFromSurface (renderer, textSurface);
                textRect.w = textSurface->w;
                textRect.h = textSurface-> h;

                textRect.x = x+ (w - textRect.w) / 2;
                textRect.y = y + (h - textRect.h) / 2;
                SDL_FreeSurface(textSurface);
            }
        }
    }

    ~BUTTONS() {
        if (textTexture) {
            SDL_DestroyTexture( textTexture );
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
        } else {
            SDL_SetRenderDrawColor(renderer,normalColor.r, normalColor.g, normalColor.b, normalColor.a);
        }
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 80,80, 80, 255);
        SDL_RenderDrawRect(renderer, &rect);

        if (textTexture) {
            SDL_RenderCopy(renderer, textTexture , nullptr, &textRect);
        }
    }
};


class PROTEUS {
private:

    SDL_Window* window;
    SDL_Renderer*renderer;
    TTF_Font* font;
    TTF_Font* titleFont;
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

    SDL_Rect zoomRct;

    bool showGrid ;
    Tool currentTool;

    int tlbrH;
    int pnlLW , pnlRW;
    int vpX, vpY, vpW , vpH;

    vector<BUTTONS*> tlbrBtns;
    vector<string> libItms;
    vector<SDL_Rect> libRcts;
    string selLibItm;

    int wldToScrX(int wx) const { return vpX+ (int)(wx * zmLvl + panX);}
    int wldToScrY(int wy) const { return vpY + (int)(panY + (canvasHeight - wy) * zmLvl);}
    int scrToWldX(int sx) const { return (int)((sx - vpX - panX) / zmLvl); }
    int scrToWldY (int sy) const { return canvasHeight - (int)((sy - vpY - panY) / zmLvl) ;}

    int snapToGrid (int val) const {return ((val + grdSz/2) / grdSz) * grdSz;}

    void resetView() {
        zmLvl = 1.0f;
        panX = 1;
        panY = (winH -tlbrH - statH) - canvasHeight - 1;
    }


    void drawGrid() {
        if (!showGrid) return ;
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
                if (ss >> cw >> ch) { }
                if (!n.empty() && !p.empty() && !d.empty())
                    recentPs.push_back(project(n, p, d,cw, ch));
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
            file << p.name << "|" <<p.path << "|" << p.lastP << "|" <<p.canvasW << "|" << p.canvasH << "\n";
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

public:
    PROTEUS() {
        window = nullptr;
        renderer = nullptr;
        font = nullptr;
        titleFont =nullptr;
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
        showGrid = true;
        currentTool = Tool::SELECT;
        tlbrH = 40;
        pnlLW = 140 ;pnlRW = 140;
        vpX = pnlLW ; vpY = tlbrH;
        vpW = winW - pnlLW - pnlRW;
        vpH = winH - tlbrH - statH;
        selLibItm = "" ;

        libItms = {"Resistor","Capacitor", "LED","Transistor","Ground","VCC"};
        for (size_t i=0; i<libItms.size(); i++) {
            libRcts.push_back({5, tlbrH + 55 + (int)i*40, pnlLW-10, 28});
        }
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
        delete  btnNameCancel;
        delete btnBack;

        for (auto btn :btnRecents)
            delete btn;
        for (auto btn : tlbrBtns)
            delete btn;
        if (titleFont)
            TTF_CloseFont (titleFont);
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
        window =SDL_CreateWindow("Proteus Clone - Startup Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 850, 600, SDL_WINDOW_SHOWN);
        if (!window) return false;
        renderer =SDL_CreateRenderer(window,-1, SDL_RENDERER_ACCELERATED);
        if (!renderer) return false;
        font = TTF_OpenFont ("arial.ttf", 20);
        if (!font) { cerr << "Warning: Failed to load arial.ttf" << endl; }
        titleFont = TTF_OpenFont("arial.ttf", 30);

        btnNewP  = new BUTTONS(renderer, font, 60, 190, 300, 60, {100, 200,150, 255}, {120, 220, 170, 255}, "Create New Project") ;
        btnOpenP = new BUTTONS(renderer, font, 60, 280, 300,60,{144, 238, 144, 255}, {152, 251, 152, 255}, "Open Existing Project");
        btnRemRecents = new BUTTONS (renderer, font, 60, 500, 200, 40, {255, 204, 153, 255}, {255, 178, 102, 255}, "Remove Recents");
        btnPresetA4 = new BUTTONS (renderer, font,213, 180, 180, 45, {200,155, 240, 255}, {180, 130, 225, 255}, "A4 (800x600)");
        btnPresetA3 =new BUTTONS(renderer, font, 456, 180,180, 45,{200, 155, 240, 255}, {180, 130, 225, 255},"A3 (1200x800)");
        btnPresetCustom = new BUTTONS (renderer, font, 325, 245, 200,45,{200, 155, 240, 255}, {180, 130,225, 255}, "Custom Size...");
        btnCancelDialog = new BUTTONS(renderer,font, 325, 390, 200, 45, {255, 255, 255, 255}, {240,240, 240, 255},"Cancel");

        btnBack = new BUTTONS(renderer, font, 5, 8, 60, 24, {255,255,255,255}, {230,230,230,255}, "Back");

        int xpos= vpX + 5;
        tlbrBtns.push_back (new BUTTONS(renderer, font, xpos, 8, 70,24, {255,255,255,255},{230,230,230,255}, "Select")); xpos+=80;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 70,24, {255,255,255,255},{230,230,230,255}, "Wire")); xpos+= 80;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 105,24,{255,255,255,255},{230,230,230,255}, "Component")); xpos+=115 ;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Save")); xpos += 65;
        tlbrBtns.push_back(new BUTTONS (renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Load")); xpos+= 65;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24, {255,255,255,255},{230,230,230,255}, "Undo")); xpos+=65;
        tlbrBtns.push_back(new BUTTONS(renderer, font, xpos, 8, 55,24,  {255,255,255,255},{230,230,230,255}, "Redo"));

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
            if (ev.type == SDL_MOUSEMOTION) {
                SDL_GetMouseState(&mseX , &mseY);
            }

            if (currentState == app:: STARTUP_MENU) {
                btnNewP->events(ev);
                btnOpenP->events(ev);
                btnRemRecents->events (ev);
                for (auto btn: btnRecents)
                    btn->events(ev);
                if (btnNewP-> click(ev)) {
                    currentState = app::NEW_PROJECT_DIALOG;
                }
                else if (btnOpenP->click(ev)){
                    string chosen = fileDialog();
                    if (!chosen.empty()) {
                        canvasWidth  = 800; canvasHeight = 600;
                        string projName = projNameFromPath (chosen);
                        addToRecent(project (projName, chosen, gDate(), canvasWidth, canvasHeight));
                        resetView();
                        currentState = app::WORKSPACE;
                        cout << "Opened project: " << chosen << endl ;
                    }
                }
                else if (btnRemRecents->click(ev))
                    { clearRecents();cout << "Recent projects cleared."<< endl;}
                else {
                    for (size_t i = 0; i < btnRecents.size(); i++) {
                        if (btnRecents [i]->click(ev)) {
                            cout << "Loading project: " << recentPs[i].name <<endl;
                            canvasWidth = recentPs[i].canvasW; canvasHeight = recentPs[i].canvasH;
                            resetView();
                            currentState = app::WORKSPACE;
                        }
                    }
                }
            }
            else if (currentState == app::NEW_PROJECT_DIALOG ) {
                btnPresetA4->events(ev); btnPresetA3->events(ev); btnPresetCustom->events (ev); btnCancelDialog->events(ev);
                if (btnPresetA4->click (ev)) {
                    canvasWidth = 800; canvasHeight = 600; penProjectName =diffName("Untitled"); openNameDialog();
                }
                else if (btnPresetA3-> click(ev)) {
                    canvasWidth = 1200; canvasHeight = 800; penProjectName = diffName("Untitled"); openNameDialog();
                }
                else if (btnPresetCustom->click(ev)) {
                    if ( !txtWidth) {
                        txtWidth = new txtIn(renderer, font, 285, 225, 100, 35, true);
                        txtHeight = new txtIn(renderer,font, 465, 225, 100, 35, true);
                        btnCustomOK = new BUTTONS(renderer, font, 315, 390, 100, 40, {255,255, 255, 255}, {240, 240, 240, 255}, "OK");
                        btnCustomCancel= new BUTTONS(renderer, font, 435, 390,100, 40,{255, 255, 255, 255}, {240, 240, 240, 255}, "Cancel");
                    }
                    txtWidth->setActive(true); txtHeight->setActive(false);
                    currentState =app::CUSTOM_SIZE_DIALOG;
                }
                else if (btnCancelDialog->click(ev)) { currentState = app::STARTUP_MENU; }
            }
            else if (currentState == app::CUSTOM_SIZE_DIALOG) {
                if (txtWidth && txtWidth->isActive ()) txtWidth-> handleEvent(ev);
                if (txtHeight && txtHeight->isActive()) txtHeight->handleEvent(ev);
                if (btnCustomOK) btnCustomOK->events(ev);
                if (btnCustomCancel) btnCustomCancel->events(ev);
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button== SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    if (txtWidth && txtWidth->mouseIn(mx, my)) { txtWidth->setActive( true); if (txtHeight) txtHeight-> setActive(false); }
                    else if (txtHeight && txtHeight->mouseIn(mx, my)) { txtHeight->setActive( true ); if (txtWidth) txtWidth->setActive(false); }
                    else { if (txtWidth) txtWidth->setActive (false); if (txtHeight) txtHeight->setActive(false); }
                }
                if (btnCustomOK && btnCustomOK->click(ev)) {
                    string wStr= txtWidth  ? txtWidth->gTxt() : ""; string hStr = txtHeight ? txtHeight->gTxt(): "";
                    if (!wStr.empty() && !hStr.empty()) {
                        canvasWidth = stoi (wStr); canvasHeight = stoi(hStr);
                    }
                    else {
                        canvasWidth = 800; canvasHeight = 600;
                    }
                    if (txtWidth)
                        txtWidth->setActive (false);
                    if (txtHeight)
                        txtHeight->setActive(false);
                    penProjectName = diffName("Untitled");
                    openNameDialog();
                }
                else if ( btnCustomCancel && btnCustomCancel->click(ev)) {
                    if (txtWidth)
                        txtWidth->setActive (false);
                    if (txtHeight)
                        txtHeight->setActive(false);
                    currentState =  app ::NEW_PROJECT_DIALOG;
                }
            }
            else if (currentState == app::PROJECT_NAME_DIALOG) {
                SDL_StartTextInput();
                if (txtProjectName && txtProjectName->isActive())
                    txtProjectName->handleEvent(ev);
                if (btnNameOK)
                    btnNameOK->events (ev);
                if (btnNameCancel)
                    btnNameCancel->events(ev);
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button ==SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    if (txtProjectName && txtProjectName->mouseIn(mx, my))
                        txtProjectName->setActive (true);
                    else { if (txtProjectName)
                        txtProjectName->setActive(false); }
                }
                if(btnNameOK && btnNameOK->click(ev)) {
                    string name = txtProjectName ? txtProjectName->gTxt() : "";
                    if (name.empty())
                        name = "Untitled";
                    string finalName = diffName(name);
                    string path= "C:/projects/" + finalName + ".proj";
                    addToRecent(project(finalName, path, gDate(), canvasWidth, canvasHeight));
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    resetView ();
                    currentState = app::WORKSPACE;
                    cout << "New project created: " << finalName <<" Canvas: " << canvasWidth << "x" << canvasHeight << endl;
                }
                else if (btnNameCancel && btnNameCancel-> click(ev)) {
                    if (txtProjectName)
                        txtProjectName->setActive(false);
                    currentState = app::STARTUP_MENU;
                }
            }
            else if (currentState == app::WORKSPACE){
                btnBack->events(ev);
                if (btnBack->click(ev) ) {
                    currentState = app::STARTUP_MENU;
                }

                for (auto b : tlbrBtns)
                    b->events(ev);
                if (tlbrBtns[0]->click(ev)) {
                    currentTool = Tool::SELECT;
                    cout << "Select tool \n";
                }
                else if (tlbrBtns[1]->click(ev)) {
                    currentTool= Tool::WIRE;
                    cout << "Wire tool\n";
                }
                else if (tlbrBtns[2]->click(ev)) {
                    currentTool = Tool::COMPONENT;
                    cout<< "Component tool\n";
                }
                else if (tlbrBtns[3]->click(ev)) {
                    cout << "Save (placeholder)\n";
                }
                else if (tlbrBtns[4]->click(ev)) {
                    string chosen = fileDialog();
                    if (!chosen.empty()){
                        canvasWidth  = 800; canvasHeight = 600;
                        string projName = projNameFromPath (chosen);
                        addToRecent(project (projName, chosen, gDate(), canvasWidth, canvasHeight));
                        cout << "Loaded: " << chosen << endl;
                    }
                }
                else if (tlbrBtns[5]->click(ev)) {
                    cout << "Undo (placeholder)\n";
                }
                else if (tlbrBtns[6]-> click(ev)) {
                    cout << "Redo (placeholder)\n";
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    for (size_t i= 0; i<libRcts.size(); i++) {
                        if (mseX >= libRcts[i].x && mseX <= libRcts[i].x+libRcts[i].w &&
                            mseY >= libRcts[i].y && mseY <= libRcts[i].y+libRcts[i].h) {
                            selLibItm = libItms [i];
                            currentTool = Tool::COMPONENT;
                            cout << "Selected component: " << selLibItm << endl;
                        }
                    }
                }

                if(ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_1) {
                        currentTool = Tool::SELECT;
                        cout <<"Select tool (keyboard)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_2) {
                        currentTool = Tool::WIRE;
                        cout << "Wire tool (keyboard)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_3) {
                        currentTool= Tool::COMPONENT;
                        cout << "Component tool (keyboard)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_DELETE){
                        cout << "Delete selected item (placeholder)\n";
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button ==SDL_BUTTON_LEFT) {
                    if (mseX >= zoomRct.x && mseX <= zoomRct.x + zoomRct.w &&
                        mseY >= zoomRct.y && mseY<= zoomRct.y + zoomRct.h) {
                        resetView();
                    }
                    else if (mseX >= vpX && mseX < vpX+vpW && mseY >= vpY && mseY < vpY+ vpH) {
                        if (!btnBack->click(ev)) {
                            isPan = true ;
                            panStartX = ev.button.x ; panStartY = ev.button.y;
                            panOffXst = panX; panOffYst = panY;
                        }
                    }
                }
                if (ev.type == SDL_MOUSEBUTTONUP  && ev.button.button == SDL_BUTTON_LEFT) {
                    isPan = false;
                }
                if (isPan && ev.type == SDL_MOUSEMOTION) {
                    panX = panOffXst + (ev.motion.x - panStartX);
                    panY = panOffYst + (ev.motion.y - panStartY);
                }
                if (ev.type == SDL_MOUSEWHEEL && mseX>=vpX && mseX<vpX+vpW && mseY>=vpY && mseY<vpY+vpH){
                    int mx = mseX, my = mseY;
                    float oldZm =zmLvl;
                    if (ev.wheel.y >0) zmLvl *= 1.1f;
                    else if (ev.wheel.y < 0) zmLvl /= 1.1f ;
                    if (zmLvl < 0.2f) zmLvl = 0.2f;
                    if (zmLvl > 5.0f) zmLvl = 5.0f;
                    panX = (mx - vpX) - (int)(((mx - vpX) - panX) *(zmLvl / oldZm));
                    panY = (my - vpY) - (int)(((my - vpY) - panY) *(zmLvl / oldZm));
                }
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_PLUS || ev.key.keysym.sym  == SDLK_KP_PLUS){
                        float oldZm = zmLvl; zmLvl *= 1.1f; if (zmLvl > 5.0f) zmLvl =5.0f ;
                        int cx = vpW/2 ,cy = vpH/2;
                        panX = cx - (int) ((cx - panX) * (zmLvl / oldZm));
                        panY = cy - (int) ((cy - panY) * (zmLvl / oldZm));
                    }
                    else if (ev.key.keysym.sym == SDLK_MINUS || ev.key.keysym.sym == SDLK_KP_MINUS) {
                        float oldZm = zmLvl; zmLvl/= 1.1f; if (zmLvl < 0.2f) zmLvl = 0.2f;
                        int cx = vpW/2 , cy = vpH/2;
                        panX = cx - (int) ((cx - panX) * (zmLvl / oldZm));
                        panY = cy - (int) ((cy - panY) * (zmLvl / oldZm));
                    }
                    else if (ev.key.keysym.sym == SDLK_0 && SDL_GetModState() & KMOD_CTRL) {
                        resetView();
                    }
                    else if (ev.key.keysym.sym == SDLK_s && SDL_GetModState() & KMOD_CTRL) {
                        cout << "Ctrl+S: Save (placeholder)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_o && SDL_GetModState() & KMOD_CTRL) {
                        string chosen = fileDialog();
                        if (!chosen.empty()) {
                            canvasWidth  = 800; canvasHeight = 600;
                            string projName = projNameFromPath (chosen);
                            addToRecent(project (projName, chosen, gDate(), canvasWidth, canvasHeight));
                            cout << "Loaded: " << chosen << endl;
                        }
                    }
                    else if (ev.key.keysym.sym == SDLK_z && SDL_GetModState() & KMOD_CTRL) {
                        cout << "Undo (placeholder)\n";
                    }
                    else if (ev.key.keysym.sym == SDLK_y && SDL_GetModState() & KMOD_CTRL) {
                        cout << "Redo (placeholder)\n";
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
            btnNewP->draw(renderer) ; btnOpenP->draw (renderer); btnRemRecents->draw(renderer);
            if (btnRecents.empty()) {
                drawTxt("(no recent projects)", 470, 205, {150,150, 150, 255 });
            }
            else { for (auto btn : btnRecents) { btn->draw(renderer); } }
        }
        else if (currentState == app::NEW_PROJECT_DIALOG) {
            SDL_Rect dialogBox ={150, 80, 550, 420};
            SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255); SDL_RenderFillRect(renderer,&dialogBox);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect( renderer, &dialogBox);
            drawTxt("New Project - Select Canvas Size", 285, 100, {20, 20, 20, 255});
            btnPresetA4->draw(renderer); btnPresetA3-> draw(renderer); btnPresetCustom->draw(renderer); btnCancelDialog->draw(renderer);
        }
        else if (currentState == app::CUSTOM_SIZE_DIALOG) {
            SDL_Rect dlg = { 150, 80, 550, 420};
            SDL_SetRenderDrawColor(renderer, 210, 210, 230, 255); SDL_RenderFillRect( renderer, &dlg);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect(renderer, &dlg );
            int titleW, titleH; if (font) TTF_SizeText(font, "Enter canvas dimensions:",&titleW, &titleH); else titleW= 250;
            int titleX = dlg.x + (dlg.w- titleW) / 2; drawTxt ("Enter canvas dimensions:", titleX, 100, {20, 20, 20, 255});
            drawTxt("Width:", 250, 200, {20, 20, 20,255}); drawTxt("Height:", 430, 200, {20, 20, 20, 255});
            if (txtWidth) txtWidth->draw(renderer) ; if (txtHeight) txtHeight->draw(renderer);
            if (btnCustomOK) btnCustomOK-> draw(renderer); if (btnCustomCancel) btnCustomCancel->draw(renderer);
        }
        else if (currentState == app::PROJECT_NAME_DIALOG) {
            SDL_Rect dlg = {200, 150, 450, 300};
            SDL_SetRenderDrawColor (renderer, 210, 210, 230, 255); SDL_RenderFillRect(renderer, &dlg);
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255); SDL_RenderDrawRect(renderer, &dlg);
            int titleW, titleH; if (font) TTF_SizeText(font, "Enter project name:", &titleW, &titleH); else titleW = 200;
            int titleX = dlg.x +(dlg.w - titleW) / 2; drawTxt("Enter project name:", titleX, 180, {20, 20, 20, 255});
            if (txtProjectName) txtProjectName->draw(renderer);
            if (btnNameOK) btnNameOK->draw(renderer); if (btnNameCancel) btnNameCancel->draw(renderer);
        }
        else if (currentState ==app::WORKSPACE){
            SDL_SetRenderDrawColor ( renderer, 255, 255, 255, 255); SDL_RenderClear (renderer);

            SDL_Rect tlbrBg = {0,0,winW, tlbrH};
            SDL_SetRenderDrawColor(renderer, 150,170,200,255);
            SDL_RenderFillRect(renderer, &tlbrBg);
            btnBack->draw(renderer);
            for (auto b : tlbrBtns)
                b->draw(renderer) ;

            SDL_Rect libBg = {0, tlbrH, pnlLW, vpH};
            SDL_SetRenderDrawColor(renderer, 225,230,240,255);
            SDL_RenderFillRect(renderer,&libBg);
            SDL_SetRenderDrawColor(renderer, 170,170,180,255 );
            SDL_RenderDrawLine(renderer, pnlLW, tlbrH, pnlLW, tlbrH+vpH);
            drawTxt("Library", 8, tlbrH+5, {0,0,0,255});
            for (size_t i = 0; i <libItms.size(); i++) {
                SDL_Rect r = libRcts[i];
                SDL_SetRenderDrawColor(renderer, 210,220,240,255);
                SDL_RenderFillRect (renderer, &r);
                if (selLibItm == libItms[i]) {
                    SDL_SetRenderDrawColor(renderer, 160,190,220,255);
                    SDL_RenderFillRect(renderer, &r);
                    SDL_SetRenderDrawColor( renderer, 80,110,160,255);
                    SDL_RenderDrawRect(renderer, &r);
                } else {
                    SDL_SetRenderDrawColor(renderer, 170,180,200,255);
                    SDL_RenderDrawRect(renderer, &r);
                }
                int textW, textH;
                TTF_SizeText(font, libItms[i].c_str(), &textW, &textH);
                int tx = r.x + (r.w - textW) /2;
                int ty = r.y + (r.h - textH) /2;
                drawTxt(libItms[i], tx, ty, {0,0,0,255});
            }

            SDL_Rect propBg = {winW - pnlRW, tlbrH, pnlRW, vpH};
            SDL_SetRenderDrawColor(renderer, 225,230,240,255);
            SDL_RenderFillRect(renderer, &propBg);
            SDL_SetRenderDrawColor(renderer, 170, 170,180,255);
            SDL_RenderDrawLine(renderer, winW - pnlRW, tlbrH, winW - pnlRW, tlbrH + vpH);
            int propTitleW, propTitleH;
            TTF_SizeText(font, "Properties", &propTitleW, &propTitleH );
            int propTitleX = winW - pnlRW +(pnlRW - propTitleW)/2;
            drawTxt("Properties", propTitleX, tlbrH+5, {0,0,0,255});
            if (!selLibItm.empty()) {
                int itemW, itemH;
                TTF_SizeText(font, selLibItm.c_str(), &itemW, &itemH);
                int itemX = winW -pnlRW + (pnlRW - itemW) / 2;
                drawTxt(selLibItm, itemX, tlbrH+45, {0,0,0,255});
            }

            drawGrid();
            drawTxt("Workspace - Canvas: " +to_string (canvasWidth) + "x" + to_string(canvasHeight), vpX+ 10, vpY + 10, {0, 0, 0, 255});
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