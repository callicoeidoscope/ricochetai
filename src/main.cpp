// preprocessor directives
#include <fstream>
#include <iostream>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include "magic_enum.hpp"

// namespace
using namespace std;

const int WIDTH = 720, HEIGHT = 720;

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

enum button {
    moon, star, planet, gear, bounce, wall, robot, red, green, yellow, blue, black
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

class button_selection {
    public:
        obj* type;
        obj* colour;
};

// creates a map of textures and pieces, and pieces and objects
map<piece, SDL_Texture*> textures;
map<button, SDL_Texture*> button_textures;
map<piece, obj> physicize;
map<button, obj> buttonize;

vector<piece> pieces_to_draw;

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

// defines where the legal squares are (NEEDS TO BE TWEAKED)
void definelegal(int arrx[][16], int arry[][16], int rows)
{
    int legalx, legaly;

    for(int i = 0; i < 16; i++) {
        legalx = 116 + (30 * i);
        for(int j = 0; j < rows; j++) {
            legaly = 116 + (30 * j);
            arrx[j][i] = legalx;
            arry[j][i] = legaly;
        }
    }
}

// rounds object position to the nearest legal square
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
    // initializes SDL
    SDL_Init(SDL_INIT_VIDEO);
    
    // creates the window and the renderer
    SDL_Window* window = SDL_CreateWindow("ricochet robots", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    // creates textures for every piece in the game
    for(int i = 0; i < 42; i++){
        auto texpieceopt = magic_enum::enum_cast<piece>(i);
        if(!texpieceopt.has_value()) {
            cerr << "error code 9: integers not casting correctly to pieces at index " << i << endl;
            continue;
        }
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

    // creates a texture for the board
    SDL_Texture* board = IMG_LoadTexture(renderer, "res/board.png");

    // creates textures for buttons (note to self: makie into a function since this is just a reused ver. of earlier code)
    for(int i = 0; i < 12; i++) {
        auto texbutt = magic_enum::enum_cast<button>(i);
        if(!texbutt.has_value()) {
            cerr << "error code 11: integers not casting correctly to pieces at index " << i << endl;
            continue;
        }
        button texbutton = texbutt.value();
        auto texname = magic_enum::enum_name(texbutton);
        if(!texname.size() > 0) {
            cerr << "error code 12: pieces not casting correctly to strings at index  " << i << endl;
            continue;
        }
        string texfile = "res/buttons/add_" + string(texname) + ".png";
        if(!texfile.size() > 0) {
            cerr << "error code 13: string copying error at index  " << i << endl;
            continue;
        }
        SDL_Texture* tex = IMG_LoadTexture(renderer, texfile.c_str());
        if(!tex) {
            cerr << "error code 14: texture not loading for " << texfile << " (" << SDL_GetError() << ")\n";
            continue;
        }
        button_textures[texbutton] = tex;
    }

    // creates a rectangle class for the gameboard
    SDL_FRect gameboard;
    gameboard.x = 120;
    gameboard.y = 120;
    gameboard.w = 480;
    gameboard.h = 480;

    // creates a rectangle class for buttons
    SDL_FRect buttontype;
    buttontype.x = 55;
    buttontype.y = HEIGHT - HEIGHT / 8 - 5;
    buttontype.w = 128;
    buttontype.h = 64;
    SDL_FRect littlebuttontype;
    littlebuttontype.x = 30;
    littlebuttontype.y = 180;
    littlebuttontype.w = 64;
    littlebuttontype.h = 64;
    
    // zeroes out all the variables used for rendering
    obj* currentobj = nullptr;
    button_selection to_get_the_compiler_to_shut_up;
    button_selection* current_selection = &to_get_the_compiler_to_shut_up;
    bool is_running = true, mouse_is_down = false;
    float mx = 0, my = 0, dragoffsetx = 0, dragoffsety = 0;

    // this is the main rendering loop.
    while(is_running) {
        SDL_Event event;

        SDL_SetRenderDrawColor(renderer, 47, 48, 52, 255);
        SDL_RenderClear(renderer);

        obj add_button;
        add_button.x = WIDTH - 92;
        add_button.y = HEIGHT / 2 - 32;
        add_button.w = littlebuttontype.w;
        add_button.h = littlebuttontype.h;

        // redraw every frame there's an SDL event
        while(SDL_PollEvent(&event))
            switch(event.type) {

                case SDL_EVENT_QUIT:
                    is_running = false; break;
                
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    SDL_GetMouseState(&mx, &my);

                    for(auto& [piece, gamepiece] : physicize) {  // goes through every piece currently onscreen to check if they're currently being dragged.
                    if(mx >= gamepiece.x && mx <= gamepiece.x + gamepiece.w && my >= gamepiece.y && my <= gamepiece.y + gamepiece.h) {
                        // tells program that the current object is what's being dragged
                        mouse_is_down = true;
                        currentobj = &gamepiece;
                        dragoffsetx = mx - gamepiece.x;
                        dragoffsety = my - gamepiece.y;
                        break;
                    }
                    }
                    for(auto& [button, phys_button] : buttonize) {
                        if(mx >= phys_button.x && mx <= phys_button.x + phys_button.w && my >= phys_button.y && my <= phys_button.y + phys_button.h) {
                            mouse_is_down = true;
                            if(button <= robot)
                                current_selection->type = &phys_button;
                            else
                                current_selection->colour = &phys_button;
                            break;
                        }
                    }
                    if(mx >= add_button.x && mx <= add_button.x + add_button.w && my >= add_button.y && my <= add_button.y + add_button.h) {
                        mouse_is_down = true;
                        // sets default for when the user doesn't play nice
                        button tenum = button::moon;
                        button cenum = button::red;
                        for (auto& [which_button, val] : buttonize) {
                            if (val.x == current_selection->type->x && val.y == current_selection->type->y)
                                tenum = which_button;
                            if (val.x == current_selection->colour->x && val.y == current_selection->colour->y)
                                cenum = which_button;
                            }
                        auto type = magic_enum::enum_name(tenum);
                        auto colour = magic_enum::enum_name(cenum);
                        string fullname;
                        fullname.append(colour);
                        fullname.append(type);

                        switch(cenum) {
                            case black:
                                if(tenum != robot)
                                    fullname = "blackhole";
                                break;
                        } switch(tenum) {
                            // WALL ROTATION IMPLEMENTATION HASNT BEEN ADDED BY THE WAY!!!!!!!!!!!!!!!!!
                            case wall:
                                fullname = "bl"; break;
                            // BOUNCE ROTATION IMPLEMENTATION HASNT BEEN ADDED BY THE WAY!!!!!!!!!!!!!!!
                            case bounce:
                                fullname.append("trbl");
                        }

                        auto piece_to_draw = magic_enum::enum_cast<::piece>(string_view(fullname)); 
                        auto pval = piece_to_draw.value();
                        obj& gamepiece = physicize[pval];
                        if(pval <= 5)
                            gamepiece.t = norobot;
                        else if(pval >= 22 && pval <= 30)
                            gamepiece.t = nobounce;
                        else if(pval >= 31 && pval <= 39)
                            gamepiece.t = nw;
                        else
                            gamepiece.t = nogoal;
                        gamepiece.n = pval;
                        pieces_to_draw.push_back(pval);
                    }
                break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    mouse_is_down = false; 
                    if(currentobj) // if there's an object being dragged, snap it to the legal grid
                        roundtolegal(currentobj->x, currentobj->y, legalx, legaly, 16, currentobj->t, currentobj->n);
                    break;
                
                case SDL_EVENT_MOUSE_MOTION:
                    if(mouse_is_down && currentobj) { // makes the current object trail along with the mouse
                        SDL_GetMouseState(&mx, &my);
                        currentobj->x = mx - dragoffsetx;
                        currentobj->y = my - dragoffsety;
                    } break;
            } 

        // OUTPUTS X AND Y FOR DEBUGGING, SHOULD BE COMMENTED OUT
        // cout << currentobj->x << " " << currentobj->y << endl;

        littlebuttontype.x = WIDTH - 92;
        littlebuttontype.y = HEIGHT / 2 - 32;

        SDL_Texture* add_b = IMG_LoadTexture(renderer, "res/add.png");
        SDL_RenderTexture(renderer, add_b, nullptr, &littlebuttontype);

        littlebuttontype.x = 30;
        littlebuttontype.y = 180;

        SDL_RenderTexture(renderer, board, nullptr, &gameboard);
        for(int i = 0; i < 7; i++) {
            auto texbutt = magic_enum::enum_cast<button>(i);
            if(!texbutt.has_value()) {
                cerr << "error code 15: integers not casting correctly to pieces at index " << i << endl;
                continue;
            }
            button texbutton = texbutt.value();
            SDL_RenderTexture(renderer, button_textures[texbutton], nullptr, &buttontype);

            buttonize[texbutton] = obj(buttontype.x, buttontype.y, buttontype.w, buttontype.h);

            buttontype.x += 160;
            if(i == 3) {
                buttontype.y = HEIGHT / 8 - 60;
                buttontype.x = 135;
            }
        }
        buttontype.x = 55;
        buttontype.y = HEIGHT - HEIGHT / 8 - 5;
        for(int i = 7; i < 12; i++) {
            auto texbutt = magic_enum::enum_cast<button>(i);
            if(!texbutt.has_value()) {
                cerr << "error code 15: integers not casting correctly to pieces at index " << i << endl;
                continue;
            }
            button texbutton = texbutt.value();
            SDL_RenderTexture(renderer, button_textures[texbutton], nullptr, &littlebuttontype);

            buttonize[texbutton] = obj(littlebuttontype.x, littlebuttontype.y, littlebuttontype.w, littlebuttontype.h);

            littlebuttontype.y += 74;
        }

        for(piece p : pieces_to_draw) {
            obj& gamepiece = physicize[p];
            SDL_FRect gamepiecerect;
            gamepiecerect.x = WIDTH/2;
            gamepiecerect.y = HEIGHT/2;
            gamepiecerect.w = gamepiece.w;
            gamepiecerect.h = gamepiece.h;
            SDL_RenderTexture(renderer, textures[p], nullptr, &gamepiecerect);
        }

        // SPAWNS ALL PIECES IN FOR DEBUGGING, SHOULD BE COMMENTED OUT
        /* for(int i = 0; i < 42; i++) {
            auto rwrap = magic_enum::enum_cast<piece>(i);
            if(!rwrap.has_value()) {
                cerr << "error code 10: failed to convert integer to piece on repetition " << i+1 << endl; continue;
            }
            auto rval = rwrap.value();
            obj& gamepiece = physicize[rval];
            if(i <= 5)
                gamepiece.t = norobot;
            else if(i >= 22 && i <= 30)
                gamepiece.t = nobounce;
            else if(i >= 31 && i <= 39)
                gamepiece.t = nw;
            else
                gamepiece.t = nogoal;
            gamepiece.n = rval;
            SDL_FRect rect = { (float)gamepiece.x, (float)gamepiece.y, (float)gamepiece.w, (float)gamepiece.h};
            SDL_RenderTexture(renderer, textures[rval], nullptr, &rect);
        } */

    SDL_RenderPresent(renderer);

    }

    // terminates the stuff
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

/* here are the problems by order of How Bad They Are

1. there's no A* implementation yet
2. the add feature doesn't work
3. file output is insanely broken
4. there's no menu
5. dynamic resizing hasn't been implemented

*/