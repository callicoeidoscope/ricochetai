// preprocessor directives
#include <fstream>
#include <iostream>
#include <map>
#include <variant>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>
#include "magic_enum.hpp"
#include "definitions.h"

// namespace
using namespace std;

// creates neccesary maps (this isn't in defenitions.h because i'm not comfortable enough with my understanding of #pragma)
map<piece, SDL_Texture*> piece_textures;
map<button, SDL_Texture*> button_textures;
map<piece, obj> physicize;
map<button, obj> buttonize;

vector<piece> pieces_to_draw;
vector<obj> duplicatables_to_draw;

editor_state editor;

// checks if a piece has value
bool error_check(optional<piece> to_be_checked) {
    if(to_be_checked.has_value())
        return false;
    else
        return true;
}

// turns a list of sections into a valid block that can be parsed by the pathfinding algo
block blockize(int number_of_sections, string wall_sec, string goal_sec, string robot_sec, string bounce_sec) {
    optional<piece> bounce_sec_optional = nobounce, robot_sec_optional = norobot, goal_sec_optional = nogoal, wall_sec_optional = nw;
    
    switch (number_of_sections) {
        case 4:
            bounce_sec_optional = magic_enum::enum_cast<piece>(bounce_sec);
            if(error_check(bounce_sec_optional))
                return block();
        case 3:
            robot_sec_optional = magic_enum::enum_cast<piece>(robot_sec);
            if(error_check(robot_sec_optional))
                return block();
        case 2:
            goal_sec_optional = magic_enum::enum_cast<piece>(goal_sec);
            if(error_check(goal_sec_optional))
                return block();
        case 0:
        case 1:
            wall_sec_optional = magic_enum::enum_cast<piece>(wall_sec);
            if(error_check(wall_sec_optional))
                return block();
            break;
        default:
            cerr << "poorly coded call of cast.\n";
    } return block(wall_sec_optional.value(), goal_sec_optional.value(), robot_sec_optional.value(), bounce_sec_optional.value());
}

// translates a string to a block by splitting sections, then calling blockize
block stob(string input_string)
{
    // initializes variables
    string stob_wall, stob_goal, stob_robot, stob_bounce;
    int section_counter = 0, prev_i = 0;

    // splits the input at each hyphen, converting them into sections.
    for (int i = 0; i < input_string.size(); i++) {
        if (input_string[i] == '-') {
            switch (section_counter) {
                case 0:
                    stob_wall = input_string.substr(0, i); break;
                case 1:
                    stob_goal = input_string.substr(prev_i + 1, i - prev_i - 1); break;
                case 2:
                    stob_robot = input_string.substr(prev_i + 1, i - prev_i - 1); break;
                default:
                    stob_bounce = input_string.substr(prev_i + 1, i - prev_i - 1); break;
            }

            section_counter++;
            prev_i = i;
        }
    }

    // turns the sections into a block object
    return blockize(section_counter, stob_wall, stob_goal, stob_robot, stob_bounce);
}

// defines an array of legal squares that pieces snap to
void define_legal(int arrx[][16], int arry[][16], int rows)
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
void snap_to_legal(int& x, int& y, int arrx[][16], int arry[][16], int rows, button type, piece name)
{
    int closest_dist = ARBITRARILY_HIGH_NUMBER;
    int final_x = x, final_y = y;
    int columns = 16;

    for(int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            int proposed_x = arrx[j][i];
            int proposed_y = arry[j][i];
            int distance_to_square = (x - proposed_x) * (x - proposed_x) + (y - proposed_y) * (y - proposed_y);

            if(distance_to_square < closest_dist) {
                closest_dist = distance_to_square;
                final_x = proposed_x;
                final_y = proposed_y;
                switch (type) {
                    case nw:
                        board[j][i].wall = name; break;
                    case norobot:
                        board[j][i].robot = name; break;
                    case nogoal:
                        board[j][i].goal = name; break;
                    default:
                        board[j][i].bounce = name; break;
                }
            }
        }
    }

    x = final_x;
    y = final_y;
}

void create_texture(bool is_a_button, int enum_number, SDL_Renderer* renderer) 
{
    if (is_a_button) {
        auto texture_button_optional = magic_enum::enum_cast<button>(enum_number);
        if(!texture_button_optional.has_value()) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        button textured_button = texture_button_optional.value();
        auto textured_button_name = magic_enum::enum_name(textured_button);
        if(!textured_button_name.size() > 0) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        string texture_filename;
        if(enum_number < 12)
            texture_filename = "res/buttons/add_" + string(textured_button_name) + ".png";
        else
            texture_filename = "res/buttons/" + string(textured_button_name) + ".png";
        if(!texture_filename.size() > 0) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        SDL_Texture* actual_texture = IMG_LoadTexture(renderer, texture_filename.c_str());
        if(!actual_texture) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        button_textures[textured_button] = actual_texture;
    } else {
        auto texture_piece_optional = magic_enum::enum_cast<piece>(enum_number);
        if(!texture_piece_optional.has_value()) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        piece textured_piece = texture_piece_optional.value();
        auto textured_button_name = magic_enum::enum_name(textured_piece);
        if(!textured_button_name.size() > 0) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        string texture_filename = "res/pieces/" + string(textured_button_name) + ".png";
        if(!texture_filename.size() > 0) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        SDL_Texture* actual_texture = IMG_LoadTexture(renderer, texture_filename.c_str());
        if(!actual_texture) {
            cerr << "something went wrong whilst creating textures.\n";
            return;
        }
        piece_textures[textured_piece] = actual_texture;
    }
}

SDL_FRect make_sdl_rectangle(float x, float y, float w, float h) {
    SDL_FRect temporary_rect;
    temporary_rect.x = x;
    temporary_rect.y = y;
    temporary_rect.w = w;
    temporary_rect.h = h;
    return temporary_rect;
}

bool mouse_is_touching (obj& object_to_compare) {
    return (editor.mouse_x >= object_to_compare.x && editor.mouse_x <= object_to_compare.x + object_to_compare.w && editor.mouse_y >= object_to_compare.y && editor.mouse_y <= object_to_compare.y + object_to_compare.h);
}

void add_object(obj& type, obj& colour) {
    // why is the default red moon? will wood, of course!
    button type_enum = red, colour_enum = moon;

    for (auto& [which_button, val] : buttonize) {
        if(val.x == type.x && val.y == type.y)
            type_enum = which_button;
        if(val.x == colour.x && val.y == colour.y)
            colour_enum = which_button;
    }

    auto type_string_view = magic_enum::enum_name(type_enum);
    auto colour_string_view = magic_enum::enum_name(colour_enum);
    string colour_string = string(colour_string_view), type_string = string(type_string_view);
    string fullname = (colour_string + type_string);

    // exceptions to the naming convention

    switch (colour_enum) {
        case black:
            if (type_enum != robot)
                fullname = "blackhole";
            break;
    } switch (type_enum) {
        case wall:
            fullname = magic_enum::enum_name(editor.rotation); break;
        case bounce:
            switch (editor.rotation) {
                case t:
                    fullname.append("trbl"); break;
                default:
                    fullname.append("tlbr");
            } break;
    }

    auto piece_to_draw = magic_enum::enum_cast<::piece>(string_view(fullname)); 
    if (!piece_to_draw.has_value()) {
        cerr << "could not cast from fullname: " << fullname << endl;
        piece_to_draw = magic_enum::enum_cast<::piece>("nw");
    }

    auto pval = piece_to_draw.value();
    obj& gamepiece = physicize[pval];
    bool is_duplicatable = false;
    if(pval <= 5)
        gamepiece.type = robot;
    else if(pval >= 22 && pval <= 30) {
        gamepiece.type = bounce;
        is_duplicatable = true;
    } else if(pval >= 31 && pval <= 39){
        gamepiece.type = wall;
        is_duplicatable = true;
    } else
        gamepiece.type = moon; // just pretend this says goal :3
    gamepiece.name = pval;
    if(is_duplicatable) {
        obj duplicatable_gamepiece = gamepiece;
        duplicatable_gamepiece.x = WALLSPACE - 4;
        duplicatable_gamepiece.y = WALLSPACE - 4;
        duplicatables_to_draw.push_back(duplicatable_gamepiece);
        return;
    }
    pieces_to_draw.push_back(pval);
    gamepiece.x = WALLSPACE - 4;
    gamepiece.y = WALLSPACE - 4;
}

void handle_mouse_down (SDL_FRect& little_button_generator) {
    SDL_GetMouseState(&editor.mouse_x, &editor.mouse_y);
    editor.mouse_is_down = true;

    obj add_button;
    add_button.x = WIDTH - WALLSPACE + (MACROPIXEL / 2);
    add_button.y = WALLSPACE + (2 * WALLSPACE) - (MACROPIXEL / 2);
    add_button.w = little_button_generator.w;
    add_button.h = little_button_generator.h;

    obj rotate_button = add_button;
    rotate_button.y += (2 * WALLSPACE) - (MACROPIXEL / 2);

    obj send_button = add_button;
    send_button.y -= (2 * WALLSPACE) - (MACROPIXEL / 2);

    for (auto& [piece, gamepiece] : physicize) {
        if (mouse_is_touching(gamepiece)) {
            editor.current_object = &gamepiece;
            editor.drag_x =  editor.mouse_x - gamepiece.x;
            editor.drag_y =  editor.mouse_y - gamepiece.y;
            break;
        }
    } for (obj& gamepiece : duplicatables_to_draw)  {
        if (mouse_is_touching(gamepiece)) {
            editor.current_object = &gamepiece;
            editor.drag_x =  editor.mouse_x - gamepiece.x;
            editor.drag_y =  editor.mouse_y - gamepiece.y;
            break;
        }
    }
    for (auto& [button, phys_button] : buttonize) {
        if (mouse_is_touching(phys_button)) {
            if (button <= robot)
                editor.current_selection->type = &phys_button;
            else if (button < send)
                editor.current_selection->colour = &phys_button;
            break;
        }
    } if (mouse_is_touching(add_button))
        add_object(*editor.current_selection->type, *editor.current_selection->colour);
    if (mouse_is_touching(rotate_button)) {
        switch (editor.rotation) {
            case l:
                editor.rotation = t; break;
            case t:
                editor.rotation = l; break;
        }
    } /* if (mouse_is_touching(send_button))
        // astar(); */
}

void handle_events(SDL_Event& event, SDL_FRect& little_button_generator) {
    switch(event.type) {
        case SDL_EVENT_QUIT:
            editor.is_running = false; break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            handle_mouse_down(little_button_generator); break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            editor.mouse_is_down = false;
            if(editor.current_object)
                snap_to_legal(editor.current_object->x, editor.current_object->y, legalx, legaly, BOARD_DIMENSIONS, editor.current_object->type, editor.current_object->name);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if(editor.mouse_is_down && editor.current_object) { // makes the current object trail along with the mouse
                SDL_GetMouseState(&editor.mouse_x, &editor.mouse_y);
                editor.current_object->x = editor.mouse_x - editor.drag_x;
                editor.current_object->y = editor.mouse_y - editor.drag_y;
            } break;
            
    }
}

void generate_ui_element(char axis, SDL_FRect& generator, int enum_index, int condition, SDL_Renderer* renderer, double execute_before, double execute_whilst_y, double execute_whilst_x, double execute_after) {
    auto texbutt = magic_enum::enum_cast<button>(enum_index);
    if(!texbutt.has_value()) {
        cerr << "GENERATE_UI_ELEMENT: integers not casting correctly to pieces at index " << enum_index << endl;
        return;
    }
    button texbutton = texbutt.value();
    SDL_RenderTexture(renderer, button_textures[texbutton], nullptr, &generator);

    buttonize[texbutton] = obj(generator.x, generator.y, generator.w, generator.h);

    if(enum_index < condition) {
        if(axis == 'x')
            generator.x += execute_before;
        else
            generator.y += execute_before;
    } else if(enum_index == condition) {
        generator.y = execute_whilst_y;
        generator.x = execute_whilst_x;;
    } else {
        if(axis == 'x')
            generator.x += execute_after;
        else
            generator.y += execute_after;
    }
}

// draws the board to the screen
void launch_board_editor()
{
    // initializes graphics things
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("ricochet robots", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    // creates the textures for all the pieces, buttons, and the board.
    for(int i = 0; i < 42; i++)
        create_texture(false, i, renderer);
    for(int i = 0; i < 15; i++)
        create_texture(true, i, renderer);

    SDL_Texture* board = IMG_LoadTexture(renderer, "res/board.png");

    SDL_FRect gameboard = make_sdl_rectangle ( WALLSPACE, WALLSPACE, WIDTH * 2.0 / 3.0, HEIGHT * 2.0 / 3.0 );
    SDL_FRect button_generator = make_sdl_rectangle ( WALLSPACE / 2, HEIGHT - WALLSPACE + (MACROPIXEL / 2), MACROPIXEL * 2, MACROPIXEL );
    SDL_FRect little_button_generator = make_sdl_rectangle ( MACROPIXEL / 2, WALLSPACE, MACROPIXEL, MACROPIXEL);

    editor.is_running = true, editor.mouse_is_down = false, editor.rotation = l;
    button_selection bugfix;
    editor.current_selection = &bugfix;

    // this is the main rendering loop.
    while(editor.is_running) {
        SDL_Event event;

        while(SDL_PollEvent(&event))
            handle_events(event, little_button_generator);

        little_button_generator.x = MACROPIXEL / 2;
        little_button_generator.y = WALLSPACE;

        SDL_SetRenderDrawColor(renderer, 47, 48, 52, 255);
        SDL_RenderClear(renderer);

        SDL_RenderTexture(renderer, board, nullptr, &gameboard);

        button_generator.x = WALLSPACE / 2;
        button_generator.y = HEIGHT - WALLSPACE + (MACROPIXEL / 2);

        for(int i = 0; i < 7; i++)
            generate_ui_element('x', button_generator, i, 3, renderer, (WALLSPACE + (MACROPIXEL * 2.0 / 3)), MACROPIXEL / 2, WALLSPACE, MACROPIXEL + WALLSPACE);
        for(int i = 7; i < 15; i++)
            generate_ui_element('y', little_button_generator, i, 11, renderer, (MACROPIXEL + MACROPIXEL / 2 + MACROPIXEL / 4), WALLSPACE, WIDTH - WALLSPACE + (MACROPIXEL / 2), (2 * WALLSPACE) - (MACROPIXEL / 2));

        // i cant be bothered to refactor this into not duplicating code rn, diy future me
        for(piece p : pieces_to_draw) {
            obj& gamepiece = physicize[p];
            SDL_FRect gamepiecerect;
            gamepiecerect.x = gamepiece.x;
            gamepiecerect.y = gamepiece.y;
            gamepiecerect.w = gamepiece.w;
            gamepiecerect.h = gamepiece.h;
            SDL_RenderTexture(renderer, piece_textures[p], nullptr, &gamepiecerect);
        } for(obj& gamepiece : duplicatables_to_draw) {
            SDL_FRect gamepiecerect;
            gamepiecerect.x = gamepiece.x;
            gamepiecerect.y = gamepiece.y;
            gamepiecerect.w = gamepiece.w;
            gamepiecerect.h = gamepiece.h;
            SDL_RenderTexture(renderer, piece_textures[gamepiece.name], nullptr, &gamepiecerect);
        } 

    SDL_RenderPresent(renderer);

    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int main(int argc, char** argv)
{
    define_legal(legalx, legaly, 16);
    launch_board_editor();

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

- no a*
- render order is entirely wrong
- current object never deselects, making for weird dragging
- file output wrong format (and do i even need file outout?)
- the wall textures mess up dragging, look bad, and also maybe tax performance
- no ui highlighting
- no delete button
- black holes don't get dragged idk why
- no menu system

*/