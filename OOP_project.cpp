#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <ctime>


using namespace std;

struct project {
    
    string name;
    string path;
    string lastP;

    project(string n, string p, string date)
        : name(n), path(p), lastP(date) {}
};

enum class app {
    STARTUP_MENU,
    NEW_PROJECT_DIALOG,
    WORKSPACE
};

enum class canvasPreset {
    A4,
    A3,
    CUSTOM
};


class Buttons {
    
private:
    SDL_Rect rect;
    SDL_Color normalColor;
    SDL_Color hoverColor;
    bool hover ;

    SDL_Texture* textTexture;
    SDL_Rect textRect;

public:
    Buttons(SDL_Renderer* renderer, TTF_Font* font, int x,int y, int w, int h,
           SDL_Color nColor, SDL_Color hColor, string text){

        rect  = {x, y, w, h};
        normalColor = nColor;
        hoverColor = hColor;
        hover = false ;
        textTexture = nullptr;

        if (font ) {
            SDL_Color textColor = {20, 20, 20, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), textColor);
            if (textSurface) {
                textTexture= SDL_CreateTextureFromSurface(renderer, textSurface);
                textRect.w = textSurface->w;
                textRect.h = textSurface->h;

                textRect.x = x+ (w - textRect.w) / 2;
                textRect.y = y + (h - textRect.h) / 2;
                SDL_FreeSurface(textSurface);
            }
        }
    }

    ~Buttons() {
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

    bool click(const SDL_Event& e) const {
        if (e.type ==SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x;
            int my = e.button.y;
            return (mx >= rect.x && mx <= rect.x + rect.w &&
                    my >= rect.y && my <= rect.y + rect.h);
        }
        return false;
    }

    void draw (SDL_Renderer* renderer) const {
        
        if (hover) {
            SDL_SetRenderDrawColor(renderer, hoverColor.r, hoverColor.g, hoverColor.b, hoverColor.a);
        }
        else {
            SDL_SetRenderDrawColor(renderer,normalColor.r, normalColor.g, normalColor.b, normalColor.a);
        }
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 80,80, 80, 255);
        SDL_RenderDrawRect(renderer, &rect);
 
        if (textTexture) {
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        }
    }
};


class proteus {
    
private:
    
    SDL_Window* window;
    SDL_Renderer*renderer;
    TTF_Font* font;
    bool running;
    app currentState;

    Buttons* btnNewP;
    Buttons* btnOpenP;
    vector <Buttons*> btnRecents;
    vector<project> recentPs;

    Buttons* btnPresetA4;
    Buttons* btnPresetA3;
    Buttons* btnCancelDialog;

    void loadRecentPs() {
        ifstream file("recents.txt");
        if (file.is_open()) {
            string n, p, d;
            while (file >> n >> p >> d) {
                recentPs.push_back(project(n, p, d));
            }
            file.close();
        }

        if (recentPs.empty()) {
            recentPs.push_back(project("Preset", "C:/projects/", "2026/02/10"));
            recentPs.push_back(project("Recent Projects", "C:/projects/", "2026/02/12"));
        }

        int limit = (recentPs.size() < 5) ? recentPs.size() : 5;
        for (int i = 0; i < limit; i++) {
            btnRecents.push_back(new Buttons(
                renderer, font, 450, 150 + (i * 70), 300, 50,
                {230, 230, 230, 255}, {200, 230, 255, 255},
                recentPs[i].name
            ));
        }
    }

public:
    proteus() {
        window = nullptr;
        renderer = nullptr;
        font = nullptr;
        running = false;
        currentState = app::STARTUP_MENU;
    }

    ~proteus( ) {
        delete btnNewP;
        delete btnOpenP;
        delete btnPresetA4 ;
        delete btnPresetA3;
        delete btnCancelDialog;
        for (auto btn :btnRecents) {
            delete btn;
        }

        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow (window);

        TTF_Quit();
        SDL_Quit();
    }

    bool initial() {
        
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr <<"SDL Init failed: " << SDL_GetError() << endl;
            return false;
        }

        if (TTF_Init() == -1){
            cerr << "SDL_ttf Init failed: " << TTF_GetError() << endl;
            return false;
        }

        window = SDL_CreateWindow("Proteus Clone - Startup Menu",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 850, 600, SDL_WINDOW_SHOWN);
        if ( !window) return false;

        renderer =SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) return false;

        font = TTF_OpenFont("arial.ttf", 22);
        if  (!font) {
            cerr << "Warning: Failed to load font.ttf" << endl;
        }

        btnNewP  = new Buttons(renderer, font, 80, 150, 300, 60, {100, 200, 150, 255}, {120, 220, 170, 255}, "Create New Project");
        btnOpenP = new Buttons(renderer, font, 80, 240, 300, 60, {150, 180, 220, 255}, {170, 200, 240, 255}, "Open Existing Project");

        btnPresetA4 = new Buttons(renderer, font, 200, 200, 200, 50, {240, 240, 240, 255}, {210, 210, 210, 255}, "A4 Canvas");
        btnPresetA3 = new Buttons (renderer, font, 450, 200, 200, 50, {240, 240, 240, 255}, {210, 210, 210, 255}, "A3 Canvas");
        btnCancelDialog = new Buttons(renderer, font, 325, 300, 200, 50, {255, 120, 120, 255}, {255, 150, 150, 255}, "Cancel");

        loadRecentPs();

        running = true;
        return true;
    }


    void handleE() {

        SDL_Event ev;
        while (SDL_PollEvent(&ev) !=0) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }

            if (currentState == app:: STARTUP_MENU) {
                btnNewP->events(ev);
                btnOpenP->events(ev);
                for (auto btn : btnRecents) btn-> events(ev);

                if (btnNewP->click(ev)) {
                    currentState= app::NEW_PROJECT_DIALOG;
                }
                else if (btnOpenP->click(ev)){
                    cout << "Opening File Explorer..." << endl;
                }
                else {

                    for (size_t i = 0; i < btnRecents.size(); i++) {
                        if (btnRecents[i]->click(ev)) {
                            cout << "Loading project: " << recentPs[i ].name << endl;
                            currentState = app::WORKSPACE;
                        }
                    }

                }
            }

            else if (currentState == app::NEW_PROJECT_DIALOG) {
                btnPresetA4->events(ev);
                btnPresetA3->events(ev);
                btnCancelDialog->events( ev );

                if (btnPresetA4->click(ev) ) {
                    cout << "Canvas set to A4. Loading Workspace..." << endl;
                    currentState = app::WORKSPACE;
                }
                else if (btnPresetA3->click(ev)) {
                    cout << "Canvas set to A3. Loading Workspace..." << endl;
                    currentState =app::WORKSPACE;
                }
                else if ( btnCancelDialog->click(ev) ) {
                    currentState = app::STARTUP_MENU;
                }
            }
        }
    }

    void render(){
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderClear(renderer) ;

        if (currentState == app::STARTUP_MENU) {
            btnNewP->draw(renderer);
            btnOpenP->draw (renderer);

            for (auto btn: btnRecents) {
                btn->draw(renderer);
            }
        }
        else if (currentState == app::NEW_PROJECT_DIALOG) {
            SDL_Rect dialogBox ={150, 100, 550, 350};
            SDL_SetRenderDrawColor(renderer, 190, 190, 200, 255);
            SDL_RenderFillRect(renderer, &dialogBox);

            SDL_SetRenderDrawColor( renderer, 80, 80, 80, 255);
            SDL_RenderDrawRect(renderer, &dialogBox);

            btnPresetA4->draw(renderer);
            btnPresetA3->draw(renderer);
            btnCancelDialog->draw (renderer);
        }
        else if (currentState == app::WORKSPACE) {
            SDL_SetRenderDrawColor(renderer, 255, 255,255, 255);
            SDL_RenderClear(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    bool active() const{
        return running;
    }
};

int main(int argc, char* argv[]){

    proteus app;

    if (!app.initial()){
        return -1;
    }

    while (app.active()) {
        app.handleE();
        app.render( );
        SDL_Delay(16);
    }

    return 0;
}