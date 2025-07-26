// preprocessor directives
#include <fstream>
#include <iostream>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include "magic_enum.hpp"

// namespace
using namespace std;

// creates a piece-type enum
enum piece {
    redrobot, greenrobot, bluerobot, yellowrobot, blackrobot, norobot,
    redstar, greenstar, bluestar, yellowstar,
    redgear, greengear, bluegear, yellowgear,
    redplanet, greenplanet, blueplanet, yellowplanet,
    redmoon, greenmoon, bluemoon, yellowmoon,
    redbouncetlbr, greenbouncetlbr, bluebouncetlbr, yellowbouncetlbr,
    redbouncetrbl, greenbouncetrbl, bluebouncetrbl, yellowbouncetrbl, nobounce,
    l, r, t, b, tl, tr, bl, br, nw,
    blackhole, nogoal
};

// each block has a wall, goal, robot, and a bounce handled through the enum
class block {
    public:
        piece wall;    
        piece goal;
        piece robot;
        piece bounce;
        block()
            : wall(nw), goal(nogoal), robot(norobot), bounce(nobounce) { }
        block(piece w)
            : wall(w), goal(nogoal), robot(norobot), bounce(nobounce) { }
        block(piece w, piece g)
            : wall(w), goal(g), robot(norobot), bounce(nobounce) { }
        block(piece w, piece g, piece r)
            : wall(w), goal(g), robot(r), bounce(nobounce) { }
        block(piece w, piece g, piece r, piece b)
            : wall(w), goal(g), robot(r), bounce(b) { }
};

// creates a board of blocks & legal x and y positions for those blocks
block board[16][16];
int legalx[16][16];
int legaly[16][16];

class obj{
    public:
        int x, y, w, h;
        piece n, t;
        obj()
            : x(120), y(120), w(38), h(38), n(nw), t(nw) { }
        obj(int x, int y)
            : x(x), y(y), w(38), h(38), n(nw), t(nw)  { }
        obj(int x, int y, int w, int h)
            : x(x), y(y), w(w), h(h), n(nw), t(nw)  { }
        obj(int x, int y, int w, int h, piece n, piece t)
            : x(x), y(y), w(w), h(h), n(n), t(t)  { }
};

// creates a map of textures and pieces, and pieces and objects
map<piece, SDL_Texture*> textures;
map<piece, obj> physicize;

// translates a string to a block
block stob(string st)
{
    // creates strings to enter each piece status and initializes variables
    string w, g, r, b;
    int runner = 0, j = 0;
    
    // loops through the inputted string and parses the dashes into ministrings
    for(int i = 0; i < st.size(); i++)
    {
        if(st[i] == '-') {
            if(runner == 0)
                w = st.substr(0, i);
            else if(runner == 1)
                g = st.substr(j+1, i - j - 1);
            else if(runner == 2)
                r = st.substr(j+1, i - j - 1);
            else
                b = st.substr(j+1, i - j - 1);
            runner++;
            j = i;
        }
    }

    // uses different constructors for each level of specificity
    if(runner == 0) {
        auto result = magic_enum::enum_cast<piece>(st);
        if(!result.has_value()) {
            cerr << "error code 4: couldn't parse piece from " << st << endl;
            return block();
        }
        block instance(result.value());
        return instance;
    } else if (runner == 1) {
        auto result = magic_enum::enum_cast<piece>(w);
        if(!result.has_value()) {
            cerr << "error code 5: couldn't parse piece from " << st << endl;
            return block();
        }
        block instance(result.value());
        return instance;
    } else if (runner == 2) {
        auto result = magic_enum::enum_cast<piece>(w);
        auto resultwo = magic_enum::enum_cast<piece>(g);
        if(!result.has_value() || !resultwo.has_value()) {
            cerr << "error code 6: couldn't parse piece from " << st << endl;
            return block();
        }
        block instance(result.value(), resultwo.value());
        return instance;
    } else if (runner == 3) {
        auto result = magic_enum::enum_cast<piece>(w);
        auto resultwo = magic_enum::enum_cast<piece>(g);
        auto resulthree = magic_enum::enum_cast<piece>(r);
        if(!result.has_value() || !resultwo.has_value() || !resulthree.has_value()) {
            cerr << "error code 7: couldn't parse piece from " << st << endl;
            return block();
        }
        block instance(result.value(), resultwo.value(), resulthree.value());
        return instance;
    } else if (runner == 4) {
        auto result = magic_enum::enum_cast<piece>(w);
        auto resultwo = magic_enum::enum_cast<piece>(g);
        auto resulthree = magic_enum::enum_cast<piece>(r);
        auto resultfour = magic_enum::enum_cast<piece>(b);
        if(!result.has_value() || !resultwo.has_value() || !resulthree.has_value() || !resultfour.has_value()) {
            cerr << "error code 8: couldn't parse piece from " << st << endl;
            return block();
        }
        block instance(result.value(), resultwo.value(), resulthree.value(), resultfour.value());
        return instance;
    }

    return block();
}

void definelegal(int arrx[][16], int arry[][16], int rows)
{
    int legalx = 115, legaly = 115;

    for(int i = 0; i < 16; i++) {
        legalx = 115 + (30 * i);
        for(int j = 0; j < rows; j++) {
            legaly = 115 + (30 * j);
            arrx[j][i] = legalx;
            arry[j][i] = legaly;
        }
    }
}

void roundtolegal(int& x, int& y, int arrx[][16], int arry[][16], int rows, piece type, piece name)
{
    int closestdist = 99999;
    int sx = x, sy = y;

    for(int i = 0; i < 16; i++) {
        for (int j = 0; j < rows; j++) {
            int posix = arrx[j][i];
            int posiy = arry[j][i];
            int dist = (x - posix) * (x - posix) + (y - posiy) * (y - posiy);

            if(dist < closestdist) {
                closestdist = dist;
                sx = posix;
                sy = posiy;
                if(type == nw)
                    board[j][i].wall = name;
                else if(type == norobot)
                    board[j][i].robot = name;
                else if(type == nogoal)
                    board[j][i].goal = name;
                else
                    board[j][i].bounce = name;
            }
        }
    }

    x = sx;
    y = sy;
}

// draws the board to the screen
void drawtoscreen()
{
    SDL_Init(SDL_INIT_VIDEO);
    float w, h;
    
    SDL_Window* window = SDL_CreateWindow("ricochet robots", 720, 720, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    for(int i = 0; i < 42; i++){
        // casts integer to optional
        auto texpieceopt = magic_enum::enum_cast<piece>(i);
        if(!texpieceopt.has_value()) {
            cerr << "error code 9: integers not casting correctly to pieces at index " << i << endl;
            continue;
        }

        // casts optional to piece
        piece texpiece = texpieceopt.value();


        auto texname = magic_enum::enum_name(texpiece);
        if(!texname.size() > 0) {
            cerr << "error code 1: pieces not casting correctly to strings at index  " << i << endl;
            continue;
        }

        string texfile = "res/pieces/" + string(texname) + ".png";

        if(!texfile.size() > 0) {
            cerr << "error code 2: string copying error at index  " << i << endl;
            continue;
        }

        SDL_Texture* tex = IMG_LoadTexture(renderer, texfile.c_str());

        if(!tex) {
            cerr << "error code 3: texture not loading for " << texfile << " (" << SDL_GetError() << ")\n";
            continue;
        }

        textures[texpiece] = tex;
    }

    SDL_Texture* board = IMG_LoadTexture(renderer, "res/board.png");

    SDL_GetTextureSize(textures[bluestar], &w, &h);

    w /= 16;
    h /= 16;

    SDL_FRect dsrc;
    dsrc.x = (720 - w) / 2.0f;
    dsrc.y = (720 - h) / 2.0f;
    dsrc.w = (float)w;
    dsrc.h = (float)h;

    SDL_FRect gameboard;
    gameboard.x = 120;
    gameboard.y = 120;
    gameboard.w = 480;
    gameboard.h = 480;

    SDL_SetRenderDrawColor(renderer, 224, 226, 219, 255);
    SDL_RenderFillRect(renderer, &gameboard);

    SDL_RenderTexture(renderer, board, nullptr, &gameboard);
    
    obj* currentobj = nullptr;
    bool isr = true, isd = false;
    float mx = 0, my = 0, dragoffsetx = 0, dragoffsety = 0;

    while(isr) {
        SDL_Event event;
        while(SDL_PollEvent(&event))
            switch(event.type) {
                case SDL_EVENT_QUIT:
                    isr = false; break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    SDL_GetMouseState(&mx, &my);
                    for(auto& [piece, robot] : physicize) {
                    if(mx >= robot.x && mx <= robot.x + robot.w && my >= robot.y && my <= robot.y + robot.h) {
                        isd = true;
                        currentobj = &robot;
                        dragoffsetx = mx - robot.x;
                        dragoffsety = my - robot.y;
                        break;
                    }
                } break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    isd = false; 
                    if(currentobj)
                        roundtolegal(currentobj->x, currentobj->y, legalx, legaly, 16, currentobj->t, currentobj->n);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if(isd && currentobj) {
                        SDL_GetMouseState(&mx, &my);
                        currentobj->x = mx - dragoffsetx;
                        currentobj->y = my - dragoffsety;
                    } break;
            } 
    
    SDL_SetRenderDrawColor(renderer, 47, 48, 52, 255);
    SDL_RenderClear(renderer);
    SDL_RenderFillRect(renderer, &gameboard);
    SDL_RenderTexture(renderer, board, nullptr, &gameboard);
    for(int i = 0; i < 42; i++) {
        auto rwrap = magic_enum::enum_cast<piece>(i);
        if(!rwrap.has_value()) {
            cerr << "error code 10: failed to convert integer to piece on repetition " << i+1 << endl; continue;
        }
        auto rval = rwrap.value();
        obj& robot = physicize[rval];
        if(i <= 5)
            robot.t = norobot;
        else if(i >= 22 && i <= 30)
            robot.t = nobounce;
        else if(i >= 31 && i <= 39)
            robot.t = nw;
        else
            robot.t = nogoal;
        robot.n = rval;
        SDL_FRect rect = { (float)robot.x, (float)robot.y, (float)robot.w, (float)robot.h};
        SDL_RenderTexture(renderer, textures[rval], nullptr, &rect);
    }

    SDL_RenderPresent(renderer);

    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/* KEEP WORKING ON THIS.

class state {
    int rx, ry, gx, gy, bx, by, yx, yy, moves;
    vector<string> history;
};

*/

int main(int argc, char** argv)
{
    // THIS IS FOR READING IN CODE AND SHOULD BE IMPLEMENTED ON THE OTHER END OF THE MODIFIED A*.
    /* ifstream fin;
    string cell;
    fin.open("ricochet_input.txt");

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            fin >> cell;
            board[j][i] = stob(cell);
    }   } */

    definelegal(legalx, legaly, 16);
    drawtoscreen();

    ofstream fon;
    fon.open("ricochet_output.txt");
    for(int i = 0; i < 16; i++) {
        for(int j = 0; j < 16; j++) {
            fon << magic_enum::enum_name(board[j][i].wall) << "-" << magic_enum::enum_name(board[j][i].robot) << "-" << magic_enum::enum_name(board[j][i].goal) << "-" << magic_enum::enum_name(board[j][i].bounce) << " ";
        }
        fon << "\n";
    }

    return 0;
}

// to compile: g++ -Iinclude src/main.cpp -Llib -o main.exe -lmingw32 -lSDL3 -lSDL3_image

// note: there are MANY problems with this, mostly that it doesn't clear out every block with no piece every frame
// which leaves pieces where they oughtn't be in the output. also the output is too verbose, and the x and y are flipped.
// additionally, A* hasn't been implemented, so all of its related things have been commented out.
// the board is slightly too big for anything to look nice and mesh, so i need to draw a new sprite.
// i also need to add the "add gamepiece" button, spawning all 42 at once is simply a debugging feature.