const int WIDTH = 720, HEIGHT = 720, ARBITRARILY_HIGH_NUMBER = 999999;
const int MACROPIXEL = WIDTH / 12;
const int WALLSPACE = MACROPIXEL * 2;

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
    moon, star, planet, gear, bounce, wall, robot, red, green, yellow, blue, black, send, add, rotate
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