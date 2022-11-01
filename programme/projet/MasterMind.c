#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

//_____________some useful definitions_____________//

/* for the "update_screen" function */
#define update_matrix 0
#define update_line 1
#define update_bar 2
#define update_finish 3
#define update_code 4
#define update_comments 5
#define update_main_menu 6
#define update_scores 7
#define update_nouv 8
#define update_saves 9

/* for the "what_is_pressed" function*/
//input
#define in_main_menu 0
#define in_game 1
#define in_nouv 2
#define in_saves 3
//output
#define bar_ball 0
#define line_ball 1
#define code_ball 3
#define finish_button 2

#define main_mode_ 3
#define main_nouv_ 4
#define main_saves_ 5
#define main_scores_ 6

#define nouv_line_ 7

#define saves_line 8
#define saves_line_delete 9
#define saves_left_right 10

/* for the "submit_code" function */
#define __win__ 0
#define __loose__ 1

/* for the "reset" function*/
#define __line__ 1
#define __bar__ 2

/* for the function "animate_transition"  */
/*#define line_to_bar 0
#define bar_to_line 1
#define line_transition 0
#define ball_transition 1*/
//________________________________________________//










// declaration of types and global variables

typedef struct H_M_S
{
    int h,m,s;
} H_M_S;

typedef struct destinations
{
    // game
    SDL_Rect matrix_line[8];
    SDL_Rect bar[6];
    SDL_Rect code[4];
    SDL_Rect back, line, cover, finish, comments, loose, win;
    SDL_Rect bar_area, line_area, time_area;/* used for clipping */
    SDL_Rect clipper; /* used for clipping a part of the background */

   // main_menu
    SDL_Rect main_back;
    SDL_Rect main_mode[2];
    SDL_Rect main_nouv, main_pause, main_saves, main_scores;

    // nouvelle partie
    SDL_Rect nouv_back;
    SDL_Rect nouv_line[3];

    // Sauvegardes
    SDL_Rect saves_back;
    SDL_Rect saves[9];
    SDL_Rect saves_right, saves_left, indicator_area;

    // indicator for "saves" and "new game"
    SDL_Rect indicator;
    SDL_Rect under_line;

    // scores
    SDL_Rect scores_back;
    SDL_Rect score_line[10];

} destinations;

typedef struct data_struct
{
    //if a cell is empty, it has value = last_texture
    //if it is not empty, it has a value = index of the texture it holds
    int matrix[10][6];
    int code[4];
    int bar[6];

    int line; //it indicates the current line
} data_struct;

typedef struct under_mouse
{/// what is pressed ? (with the mouse)
    int type; //line_ball, bar_ball, finish_button . . .
    int i; // index of the thing in case it is a type that hes many elements

}under_mouse;

/*typedef struct transition_info
{
    //transition from A to B/
    int type;

    //"A" balls
    int A_tex[4];
    int A_dest[4];

    //"B" balls
    int B_tex[4];
    int B_dest[4];

} transition_info;*/

static const int width = 1300; /* ex 1300 */
static const int height = 750; /* ex 820 */
destinations *dest;
#define last_texture 24
SDL_Texture *tex[25]; // texture 24 (last_texture) is used to not show anything (NULL)
SDL_Window  *window = NULL;
SDL_Renderer *renderer = NULL;
data_struct data;


H_M_S game_time; // counts the time that actual "gaming time" (while it is not paused)
clock_t global_time; // useful for counting time

int indicator_tex = last_texture;

TTF_Font * font;
SDL_Surface * message = NULL;
SDL_Texture *text = NULL;
SDL_Color textcolor[3] = {{0,0,0},{142,106,35},{250,220,0}}; // 0_black, 1_dark yellow, 2_yellow (for shadow effect)

//// CONITIONAL VARIABLES________
// INTERFACE
bool comment_shown = false;
bool finish_shown = false;
bool loose_shown = false;
bool win_shown = false;
bool cover_shown = true;
bool line_shown = true;
bool finish_was_pressed = false;
bool present_result = true; /* used to avoid tearing of image ... (double buffering) */

//// LOGIC
//AI
bool in_comb[6];
bool excluded[6];

//GAME_INFO
bool game_over = false;
bool easy_mode = false;
bool game_is_paused = false;

////_____________________________















//___________________________________________simple_functions______________________________________________//

SDL_Rect add_rect(SDL_Rect rect_A, SDL_Rect rect_B)
{
    return (SDL_Rect) { rect_A.x + rect_B.x,
                        rect_A.y + rect_B.y,
                        rect_A.w + rect_B.w,
                        rect_A.h + rect_B.h
                      };
}


void clip_and_replace(SDL_Rect location,int where)
{/// this function clips a part of background and draws it in this destination

    dest->clipper = location;
    // transform the "location" from relative to screen to relative to background
    if(where == in_game)
    {
        dest->clipper.x -= dest->back.x;
        dest->clipper.y -= dest->back.y;
        // replace the old location (in the screen) by this
        SDL_RenderCopy(renderer, tex[0], &dest->clipper, &location);
    }
    else
    if(where == in_saves)
    {
        dest->clipper.x -= dest->saves_back.x;
        dest->clipper.y -= dest->saves_back.y;
        // replace the old location (in the screen) by this
        SDL_RenderCopy(renderer, tex[17], &dest->clipper, &location);
    }
}

void reset(int what)
{ /// resets the "what" (__line__ or __bar__) to it's original state
    int i,j,n;
    if(what == __bar__)
    {
        for(i=0;i<6;i++)
            data.bar[i] = i+1;
    }
    else
    if(what == __line__)
    {
        for(i=0;i<4;i++)
        {
            dest->matrix_line[i].y = 739-50;
        }
        //"indice"
        n=0;
        for(i=0;i<2;i++)
        {
            for(j=0;j<2;j++)
            {
                dest->matrix_line[4+n].x = 985 + 38*i;
                dest->matrix_line[4+n].y = 711 - 21*j;
                n++;
            }
        }
        //line >>> tex[10]
        dest->line.y = 733-50;
            //line area
            dest->line_area.y = 733-50;
    }
}

bool mouse_in_range(int x, int y,SDL_Rect range)
{
    if(x > range.x && x < range.w + range.x && y > range.y && y <range.y + range.h)
        return true;
    else
        return false;
}

void set_indicator_info(int where_is_it, int index)
{/// sets the destination of the indicator depending on where is is


    switch(where_is_it)
    {
    case main_mode_:
        dest->indicator = dest->main_mode[index];
        indicator_tex = 18; //short
        break;

    case main_nouv_:
        dest->indicator = dest->main_nouv;
        indicator_tex = 19; //long
        break;

    case main_saves_:
        dest->indicator = dest->main_saves;
        indicator_tex = 19; //long
        break;

    case main_scores_:
        dest->indicator = dest->main_scores;
        indicator_tex = 19; //long
        break;

    case nouv_line_:
        dest->indicator.y = dest->nouv_line[index].y;
        dest->indicator.x = 1092;
        dest->indicator.w = 174;
        dest->indicator.h = 99;
        indicator_tex = 20; //3 arrows
        break;

    case saves_line:
        dest->indicator.y = dest->saves[index].y;
        dest->indicator.x = 1133;
        dest->indicator.w = 119;
        dest->indicator.h = 68;
        indicator_tex = 20; //3 arrows
        break;

    case saves_line_delete:
        dest->indicator.y = dest->saves[index].y;
        dest->indicator.x = 1133;
        dest->indicator.w = 70;
        dest->indicator.h = 60;
        indicator_tex = 12; //loose sign
        break;

    default: // no where
        indicator_tex = last_texture;

    }
}

under_mouse what_is_pressed(int x, int y, int where)
{/// x,y are mouse coordinates, returns -1 if nothing is pressed
    under_mouse thing;
    int i;

    switch(where)
    {
    case in_game:
        // if finish is pressed
        if(mouse_in_range(x,y,dest->finish))
        {
            thing.type = finish_button;
            return thing;
        }

        // if a line ball is pressed
        for(i=0;i<4;i++)
        {
            if(mouse_in_range(x,y,dest->matrix_line[i]))
            {
                thing.type = line_ball;
                thing.i = i;
                return thing;
            }
        }
        // if a bar ball is pressed
        for(i=0;i<6;i++)
        {
            if(mouse_in_range(x,y,dest->bar[i]))
            {
                thing.type = bar_ball;
                thing.i = i;
                return thing;
            }
        }
        // if a line code ball is pressed
        for(i=0;i<4;i++)
        {
            if(mouse_in_range(x,y,dest->code[i]))
            {
                thing.type = code_ball;
                thing.i = i;
                return thing;
            }
        }
        break;

    case in_main_menu:
        if(mouse_in_range(x,y,dest->main_mode[0]))
        {
            thing.type = main_mode_;
            thing.i = 0;
            set_indicator_info(main_mode_, 0);
            return thing;
        }
        else
        if(mouse_in_range(x,y,dest->main_mode[1]))
        {
            thing.type = main_mode_;
            thing.i = 1;
            set_indicator_info(main_mode_, 1);
            return thing;
        }
        else
        if(mouse_in_range(x,y,dest->main_nouv))
        {
            thing.type = main_nouv_;
            set_indicator_info(main_nouv_,0);
            return thing;
        }
        else
        if(mouse_in_range(x,y,dest->main_saves))
        {
            thing.type = main_saves_;
            set_indicator_info(main_saves_,0);
            return thing;
        }
        else
        if(mouse_in_range(x,y,dest->main_scores))
        {
            thing.type = main_scores_;
            set_indicator_info(main_scores_,0);
            return thing;
        }
        break;

    case in_nouv:
        for(i=0;i<3;i++)
        {
            if(mouse_in_range(x,y,dest->nouv_line[i]))
            {
                thing.type = nouv_line_;
                thing.i = i;
                set_indicator_info(nouv_line_,i);
                return thing;
            }
        }
        break;

    case in_saves:
        for(i=0;i<9;i++)
        {
            if(mouse_in_range(x,y,dest->saves[i]))
            {
                thing.type = saves_line;
                thing.i = i;
                set_indicator_info(saves_line,i);
                return thing;
            }
            else // if we ant to delete a save ( we press in the right of the save )
            if(mouse_in_range(x,y,add_rect(dest->saves[i], (SDL_Rect){dest->saves[i].w +10 , 0,0,0})))
            {
                thing.type = saves_line_delete;
                thing.i = i;
                set_indicator_info(saves_line_delete,i);
                return thing;
            }
        }
        if(mouse_in_range(x,y,dest->saves_left))
        {
            thing.type = saves_left_right;
            thing.i = 0; //left
            return thing;
        }
        else
        if(mouse_in_range(x,y,dest->saves_right))
        {
            thing.type = saves_left_right;
            thing.i = 1; //left
            return thing;
        }
        break;
    }

//  at this point, nothing is pressed !
    thing.type = -1; /* !!!! careful thing type must be determined, or else there will be weird bugs !!!!*/
    indicator_tex = last_texture; // don't show indicator
    return thing;
}

int get_available_line_pos()
{/// gets the first available positions in current line, if not found it returns -1
    int i;
    for(i=0;i<4;i++)
    {
        if(data.matrix[data.line][i] == last_texture)
            return i;
    }
    return -1;
}

/*int distance(int x, int y, int x0, int y0)
{
    return (x-x0)*(x-x0) + (y-y0)*(y-y0);
}*/

bool line_is_empty()
{
    int i;
    for(i=0;i<4;i++)
    {
        if(data.matrix[data.line][i] != last_texture)
            return false;
    }
    return true;
}

char* current_time()
{
    time_t current_time;
    char* c_time_string;

    /* Obtain current time. */
    current_time = time(NULL);

    if (current_time == ((time_t)-1))
    {
        fprintf(stderr, "Failure to obtain the current time.\n");
        exit(EXIT_FAILURE);
    }

    /* Convert to local time format. */
    c_time_string = ctime(&current_time);

    if (c_time_string == NULL)
    {
        fprintf(stderr, "Failure to convert the current time.\n");
        exit(EXIT_FAILURE);
    }
    return c_time_string;
}

char int_to_char(int n)
{
    char text[] = "0123456789";
    return (n<10 && n>-1) ? text[n] : 'X';
}

char* hour_min_sec(H_M_S* _time_)
{
    static char text[] = "00:00:00";

    text[0] = int_to_char(_time_->h/10);
    text[1] = int_to_char(_time_->h%10);

    text[3] = int_to_char(_time_->m/10);
    text[4] = int_to_char(_time_->m%10);

    text[6] = int_to_char(_time_->s/10);
    text[7] = int_to_char(_time_->s%10);

    return text;
}

void increment_time(H_M_S *_time_, int by_what)
{
    while(by_what > 0)
    {
        _time_->s++;
        if(_time_->s >= 60)
        {
            _time_->s = 0;
            _time_->m++;
            if(_time_->m >= 60)
            {
                _time_->m = 0;
                _time_->h++;
            }
        }

        by_what--;
    }

}

H_M_S init_time()
{
    return (H_M_S){0,0,0};
}


float time_in_min(H_M_S time_HMS )
{
    return (float)(time_HMS.h*60 + time_HMS.m+ (float)time_HMS.s/60);
}

int time_compare(H_M_S time1, H_M_S time2)
{
    float t1 = time_in_min(time1);
    float t2 = time_in_min(time2);

    if( t1 < t2 ) return 1; /** returns 1 if t1 > t2 */
    else
    if( t1 > t2 ) return -1; /** returns -1 if t1 < t2  */
    else
        return 0; /** returns 0 if t1 == t2  */
}








//_____________________________________________INTERFACE__________________________________________________//

void set_all_destinations()
{ /// Defines where on the "screen" we want to draw the textures
    /**
    hay assanawnar
    **/

    int i,j,n;


//________________________game_______________________________//

    //matrix line
        //"boule"
        for(i=0;i<4;i++)
        {
            dest->matrix_line[i].x = 587 + (int)86.333*i;
            dest->matrix_line[i].y = 739-50;
            dest->matrix_line[i].w = 53;
            dest->matrix_line[i].h = 53;
        }
        //"indice"
        n=0;
        for(i=0;i<2;i++)
        {
            for(j=0;j<2;j++)
            {
                dest->matrix_line[4+n].x = 985 + 38*i;
                dest->matrix_line[4+n].y = 711 - 21*j;
                dest->matrix_line[4+n].w = 33;
                dest->matrix_line[4+n].h = 33;
                n++;
            }
        }

    //bar
    for(i=0;i<6;i++)
    {
        dest->bar[i].x = 457;
        dest->bar[i].y = 650 - 108*i;
        dest->bar[i].w = 53;
        dest->bar[i].h = 53;
    }
        //bar_area
        dest->bar_area.x = 451;
        dest->bar_area.y = 123-50;
        dest->bar_area.w = 66;
        dest->bar_area.h = 626;

    //code
    for(i=0;i<4;i++)
    {
        dest->code[i].x = 587 + (int)86.333*i;
        dest->code[i].y = 04;
        dest->code[i].w = 53;
        dest->code[i].h = 53;
    }
    //other
        //background >>> tex[0] is the background texture
        dest->back.x = 0;
        dest->back.y = -70;
        dest->back.w = 1327;
        dest->back.h = 864;

        //line >>> tex[10]
        dest->line.x = 525;
        dest->line.y = 632;
        dest->line.w = 411;
        dest->line.h = 66;
            //line area
            dest->line_area.x = 524;
            dest->line_area.y = 733-50;
            dest->line_area.w = 558;
            dest->line_area.h = 66;

        //cover >>> tex[9]
        // we don't want to put on the cover yet
        dest->cover.x = 569;
        dest->cover.y = 01;
        dest->cover.w = 344;
        dest->cover.h = 61;

        //finish >>> tex[11]
        dest->finish.x = 41;
        dest->finish.y = 526-50;
        dest->finish.w = 314;
        dest->finish.h = 146;

        //comments >>> tex[14]
        dest->comments.x = 11;
        dest->comments.y = -11;
        dest->comments.w = 363;
        dest->comments.h = 506;

        //loose >>> tex[12]
        dest->loose.x = 300; /* ex 401*/
        dest->loose.y = 44-45;  /* ex 14*/
        dest->loose.w = 124;
        dest->loose.h = 120;

        //win >>> tex[13]
        dest->win.x = 300;  /* ex 401*/
        dest->win.y = 44-45;   /* ex 14*/
        dest->win.w = 136;
        dest->win.h = 107;

        // time area
            dest->time_area.x = 27;
            dest->time_area.y = 51; /* ex 6 ou 7 ou 9 ... */
            dest->time_area.w = 313;
            dest->time_area.h = 81;



//_____________________Menus______________________//

//______ main menu:
        dest->main_back.x = 0;
        dest->main_back.y = -15;
        dest->main_back.w = 1300;
        dest->main_back.h = 818;

        dest->main_mode[0].x = 650 ; /* easy*/ dest->main_mode[1].x = 650 + 248; /* hard */
        dest->main_mode[0].y = dest->main_mode[1].y = 148-15;
        dest->main_mode[0].w = dest->main_mode[1].w = 232;
        dest->main_mode[0].h = dest->main_mode[1].h = 105;

        dest->main_nouv.x = dest->main_saves.x = dest->main_scores.x = 358;
        dest->main_nouv.y  = 289-15; dest->main_saves.y = 289+147-15; dest->main_scores.y = 289+147+147-15;
        dest->main_nouv.w = dest->main_saves.w = dest->main_scores.w = 548;
        dest->main_nouv.h = dest->main_saves.h = dest->main_scores.h = 112;

        dest->main_pause.x = 353;
        dest->main_pause.y = 286-15;
        dest->main_pause.w = 596;
        dest->main_pause.h = 428;

//______ nouvelle partie:
        dest->nouv_back.x = 0;
        dest->nouv_back.y = -15;
        dest->nouv_back.w = 1386;
        dest->nouv_back.h = 863;

        dest->nouv_line[0].x = dest->nouv_line[1].x = dest->nouv_line[2].x = 125;
        dest->nouv_line[0].y = 222 -15; dest->nouv_line[1].y = 222+185 -15; dest->nouv_line[2].y = 222+185+185 -15;
        dest->nouv_line[0].w = dest->nouv_line[1].w = dest->nouv_line[2].w = 966;
        dest->nouv_line[0].h = dest->nouv_line[1].h = dest->nouv_line[2].h = 113;

//______ charger une partie:
        dest->saves_back.x = 0;
        dest->saves_back.y = -27;
        dest->saves_back.w = 1329;
        dest->saves_back.h = 840;

        dest->indicator_area.x = 1133;
        dest->indicator_area.y = 108 -27;
        dest->indicator_area.w = 119;
        dest->indicator_area.h = 606.375;

        for(i=0;i<9;i++)
        {
            dest->saves[i].x = 157;
            dest->saves[i].y = 108 -27 + (int)i*67.375;
            dest->saves[i].w = 945;
            dest->saves[i].h = 68;
        }

        dest->saves_right.x = 1180; dest->saves_left.x = 5;
        dest->saves_right.y = 738 -27; dest->saves_left.y = 734 -27;
        dest->saves_right.w = dest->saves_left.w = 143;
        dest->saves_right.h = dest->saves_left.h = 94;

//______ scores:
        dest->scores_back.x = 0;
        dest->scores_back.y = -25;
        dest->scores_back.w = 1327;
        dest->scores_back.h = 860;

        for(i=0;i<10;i++)
        {
            dest->score_line[i].x = 157;
            dest->score_line[i].y = 116 - 25 + (int)i*67.555;
            dest->score_line[i].w = 1064;
            dest->score_line[i].h = 68;
        }

//______ indicator
        if(easy_mode)
            dest->under_line.x = dest->main_mode[0].x;
        else
            dest->under_line.x = dest->main_mode[1].x;
        dest->under_line.y = dest->main_mode[0].y + dest->main_mode[0].w/2;
        dest->under_line.w = dest->main_mode[0].w;
        dest->under_line.h = 10;

        /* just an meaningless initialization */
        dest->indicator = dest->back;

}

void increment_line()
{ /// makes the "line" move by up by 1
    int i;

    //balls & indices
    for(i=0;i<8;i++)
    {
        dest->matrix_line[i].y += -69;
    }

    //the line
    dest->line.y += -69;
    //line area
    dest->line_area.y += -69;
}

void releaseResources (int lastTextureToFREE)
{ /// frees memory from all allocated resources ( textures, renders & window ), then turns SDL off

    int i;
// Releasing:
    //renderer
    SDL_DestroyRenderer(renderer);
    //window
    SDL_DestroyWindow(window);
    //destinations
    free(dest);
    //textures
    for(i=0;i<=lastTextureToFREE;i++)
    {
        SDL_DestroyTexture(tex[i]);
    }
    IMG_Quit();

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_Quit();
}

void initialize_SDL()
{
// Initialize SDL video
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("failed to initialize SDL (%s) \n", SDL_GetError());
        exit(-1);
    }

// Create a SDL window
    window = SDL_CreateWindow("MasterMind", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        printf("failed to create window (%s) \n", SDL_GetError());
        exit(-1);
    }
// Create a renderer (accelerated and in sync with the display refresh rate)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(renderer == NULL)
    {
        printf("failed to create renderer (%s) \n", SDL_GetError());
        SDL_DestroyWindow(window);
        exit(-1);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

// Initialize support for loading PNG and JPEG images
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

// Init SDL_ttf
    TTF_Init();
    font = TTF_OpenFont("fonts/Harlow Solid Italic.TTF", 60);

}

void loadResources()
{/// loads the needed data (images)

// loading textures
    FILE *textres_file;
    textres_file = fopen("images/textures.txt","r");
    if(textres_file == NULL)
    {
        printf("error to load file");
        releaseResources(0);
    }

    int i = 0;
    //load textures (G.RAM) from (the surfaces (RAM) loaded from file)
    SDL_Surface *image;
    char name[30];
    do
    {
        fscanf(textres_file,"%s\n\n", name);
        //we need to add containing file: images/...
        image = IMG_Load(name);
        tex[i] = SDL_CreateTextureFromSurface(renderer, image);
        if(tex[i] == NULL)
        {
            printf("error loading the (%s) texture, (%s)\n", name, SDL_GetError());
            releaseResources(i-1);
        }
        i++;
        //we must free surface from RAM
        SDL_FreeSurface(image);

    }   while(!feof(textres_file));
    fclose(textres_file);
    //texture last_texture is used to not show anything
    tex[last_texture] = NULL;

// Allocate destinations structure
    dest = (destinations*)malloc(sizeof(destinations));
    set_all_destinations();

}

void update_screen(int what_to_update)
{/// what_to_update ? : update_matrix (for animation), update_code(for showing the code at the end)
    int i,n,line;
    bool all_lines = false;

    if(what_to_update == update_matrix)
    {
        what_to_update = update_line;
        all_lines = true;
    }

/// Draw

// first drawn is background, !!!! DONT DRAW IF YOU DON'T WANT TO CLEAR
    if(all_lines)
    {
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex[0], NULL, &(dest->back));
    }


    switch(what_to_update)
    {
    case update_bar:
        //  remove the old bar
        clip_and_replace(dest->bar_area,in_game);
        // draw new bar
        for(i=0;i<6;i++)
        {
            SDL_RenderCopy(renderer, tex[data.bar[i]], NULL, &(dest->bar[i]));
        }
        break;

    case update_line:
            // current line or all the matrix:
            if(all_lines)
            {
                line = 0;
                reset(__line__);
            }
            else
            {
                line = data.line;
            }

            //matrix (or just one line)
            while(line <= data.line)
            {
                //remove old line
                clip_and_replace(dest->line_area,in_game);

                //draw the line balls
                for(i=0;i<4;i++)
                {
                    SDL_RenderCopy(renderer, tex[data.matrix[line][i]], NULL, &(dest->matrix_line[i]));
                }

                //draw indexes
                i=4;
                    //black indexes >>> tex[7] is the black index
                n = data.matrix[line][4];
                while(n>0)
                    {
                        SDL_RenderCopy(renderer, tex[7], NULL, &(dest->matrix_line[i]));
                        i++; n--;
                    }
                    //white indexes >>> tex[8]
                n = data.matrix[line][5];
                while(n>0)
                    {
                        SDL_RenderCopy(renderer, tex[8], NULL, &(dest->matrix_line[i]));
                        i++; n--;
                    }

            if(all_lines && line <data.line) increment_line();
            line++;
            }

            // the line texture
            if(line_shown)
            SDL_RenderCopy(renderer, tex[10], NULL, &(dest->line));

            break;

    case update_finish:
        //remove old one
        clip_and_replace(dest->finish,in_game);
        if(finish_shown)
        SDL_RenderCopy(renderer, tex[11], NULL, &(dest->finish));
        break;

    case update_code: /* cover, code, loose, win */
        //remove old one
        clip_and_replace(dest->cover,in_game);

        // draw code
        if(!cover_shown)
        for(i=0;i<4;i++)
            SDL_RenderCopy(renderer, tex[data.code[i]], NULL, &(dest->code[i]));

        // cover
        if(cover_shown)
        SDL_RenderCopy(renderer, tex[9], NULL, &(dest->cover));

        // loose
        if(loose_shown)
        SDL_RenderCopy(renderer, tex[12], NULL, &(dest->loose));

        // win
        if(win_shown)
        SDL_RenderCopy(renderer, tex[13], NULL, &(dest->win));
        break;

    case update_comments:
        // remove old one
        clip_and_replace(dest->comments,in_game);
        // comments
        if(comment_shown)
        SDL_RenderCopy(renderer, tex[14], NULL, &(dest->comments));
        break;

    case update_main_menu:
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex[15], NULL, &dest->main_back);
        if(game_is_paused)
            SDL_RenderCopy(renderer, tex[23], NULL, &dest->main_pause); // show paused game options instead of normal options
        SDL_RenderCopy(renderer, tex[22], NULL, &dest->under_line);
        SDL_RenderCopy(renderer, tex[indicator_tex], NULL, &dest->indicator);
        break;

    case update_nouv:
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex[21], NULL, &dest->nouv_back);
        /* draw the text here */
        SDL_RenderCopy(renderer, tex[indicator_tex], NULL, &dest->indicator);
        break;

    case update_saves:
        clip_and_replace(dest->indicator_area, in_saves);
        /* draw the text here */
        SDL_RenderCopy(renderer, tex[indicator_tex], NULL, &dest->indicator);
        break;

    case update_scores:
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex[16], NULL, &dest->scores_back);

    }



/// Show what was drawn
    if(present_result) SDL_RenderPresent(renderer);

}

void redraw_everything(bool draw_result)
{/// if you want to draw images after calling this, you should give it "false" to avoid tearing
    /// ( don't present to screen until you draw all frame )

    present_result = false; /* don't show result until we draw all */
    update_screen(update_matrix);
    update_screen(update_code);
    update_screen(update_bar);
    update_screen(update_comments);

    present_result = draw_result; /* now we can show result, if permitted */
    update_screen(update_finish);
    present_result = true;
}

/*void follow_mouse(int texture_index, int *x, int *y, int ball_origin)
{/// animates the drag of the selected ball using dots ... (ball's origin must be determined __line__ or __bar__)
    SDL_Event event;
    SDL_Rect ball_postition;

    //if he just clicks and releases, we do nothing/
    if(SDL_WaitEvent(&event))
    {
        if(event.type == SDL_MOUSEBUTTONUP)
            return;
    }

    int x0 = *x, y0 = *y; // x0, y0 is the last dot position

    ball_postition.w = 10;
    ball_postition.h = 10;
    ball_postition.x = x0 - ball_postition.w/2;
    ball_postition.y = y0 - ball_postition.h/2;

    SDL_RenderCopy(renderer, tex[texture_index], NULL, &ball_postition); // show a small ball

    if(ball_origin == __line__ || ball_origin == __bar__ )
        update_screen(ball_origin);
    else
        return;


    bool animating = true;
    while(animating)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_MOUSEMOTION)
            {
                *x = event.motion.x;
                *y = event.motion.y;

                //draw the new dot, if it is far enough ...
                if(distance(*x, *y, x0, y0) > 350)
                {
                    ball_postition.x = *x - ball_postition.w/2;
                    ball_postition.y = *y - ball_postition.h/2;
                    SDL_RenderCopy(renderer, tex[texture_index], NULL, &ball_postition);
                    SDL_RenderPresent(renderer);
                    x0 = *x;
                    y0 = *y;
                }

            }
            else
            if(event.type == SDL_MOUSEBUTTONUP)
            {
                animating = false;
                redraw_everything(true);
                break;
            }
        }
    }
}*/

void follow_mouse(int texture_index, int *x, int *y, int ball_origin)
{/// animates the drag of the selected ball using dots ... (ball's origin must be determined __line__ or __bar__)
    SDL_Event event;
    SDL_Rect ball_postition;

    /*if he just clicks and releases, we do nothing*/
    if(SDL_WaitEvent(&event))
    {
        if(event.type == SDL_MOUSEBUTTONUP)
            return;
    }

    int x0 = *x, y0 = *y; // x0, y0 is the last dot position

    ball_postition.w = 60;
    ball_postition.h = 60;
    ball_postition.x = x0 - ball_postition.w/2;
    ball_postition.y = y0 - ball_postition.h/2;

    if(ball_origin != __line__ && ball_origin != __bar__ )
        return;

    present_result = false; // to avoid tearing ...
    update_screen(ball_origin);
    present_result = true;
    SDL_RenderCopy(renderer, tex[texture_index], NULL, &ball_postition); // show a big ball
    SDL_RenderPresent(renderer);


    bool animating = true;
    while(animating)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_MOUSEMOTION)
            {
                *x = event.motion.x;
                *y = event.motion.y;

                ball_postition.x = *x - ball_postition.w/2;
                ball_postition.y = *y - ball_postition.h/2;
                redraw_everything(false);
                SDL_RenderCopy(renderer, tex[texture_index], NULL, &ball_postition);


                SDL_RenderPresent(renderer);
                x0 = *x;
                y0 = *y;

            }
            else
            if(event.type == SDL_MOUSEBUTTONUP)
            {
                animating = false;
                redraw_everything(true);
                break;
            }
        }
    }
}

/*void animate_transition(int from_to, int type)
{
    SDL_Rect animator[4];
    int i,t; // t for time //

    if(from_to == bar_to_line)
    for(i=0;i<4;i++)
    {
        animator[i].x =
        animator[i].w = animator[i].h = 60;
    }

    for(t=0;t<1000;t++)
    {

    }


}*/



void blit_text(char* _text_, SDL_Rect* _dest_)
{/// "blits" ( renderCopy )  the _text_ in the _dest_ location without presenting result
    /// we print the text 3 times (3 layers) to simulate shadow

    SDL_Rect text_dest;
    float text_ratio;
    int i;
            message = TTF_RenderText_Solid(font, _text_, textcolor[0]);
            text = SDL_CreateTextureFromSurface(renderer, message);
            // free it after use
            SDL_FreeSurface(message);

            SDL_QueryTexture(text, NULL,NULL, &text_dest.w, &text_dest.h);
            text_ratio = ((float)text_dest.w)/(float)text_dest.h;

            text_dest.x = _dest_->x;
            text_dest.y = _dest_->y;
            text_dest.h = _dest_->h;
            text_dest.w = (int)(text_dest.h*text_ratio);


            SDL_RenderCopy(renderer, text, NULL, &text_dest);
            // free it after use
            SDL_DestroyTexture(text);

            for(i=1;i<3;i++)
            {
                text_dest.x += 2;
                text_dest.y -= 2;

                message = TTF_RenderText_Solid(font, _text_, textcolor[i]);
                text = SDL_CreateTextureFromSurface(renderer, message);
                // free it after use
                SDL_FreeSurface(message);

                SDL_RenderCopy(renderer, text, NULL, &text_dest);
                // free it after use
                SDL_DestroyTexture(text);
            }

}


int show_saves(int page_index)
{/// returns the index of the LAST SAVE ACTUALY FOUND on the indicated page
    /// returns 0 if no saves exist in this page
    int i,j,n;
    const int FILE_LENGHTH = 19;
    char c[30];

    FILE* saves_file = fopen("sauvegardes.txt", "r");
    if( saves_file == NULL)
        return 0;

    //____ Skip all pages before the "page" (one page holds 9 saves at Maximum) )

    for(j=1;j<page_index;j++) // till we get to the page
    {
        for(i=0;i<9;i++) // skip 9 saves (1 page)
        {
            //try to read first line
            fgets(c, 30, saves_file);
            if(feof(saves_file))
            {
                fclose(saves_file);
                return 0 ; // save does not exist !
            }

            //keep reading (we skiped 1 line !! --> n>1 not n>0)
            for(n=FILE_LENGHTH; n>1; n--)
                fgets(c, 30, saves_file);
        }
    }

    // now we print only the date of the saves

    for(i=0;i<9;i++) // try to read 9 saves (1 page)
        {
            //try to read the date
            fgets(c, 30, saves_file);
            if(feof(saves_file))
            {
                fclose(saves_file);
                return i; // save does not exist ! return the last save before it
            }


            /* print the date on the page */
            c[24] = ' '; // to avoid a dispaly problem ... ( unknown char at the end )
            blit_text(c, &dest->saves[i]);

            // skip all the other info of the save
            for(n=FILE_LENGHTH-1; n>0; n--)
                fgets(c, 30, saves_file);
        }

    fclose(saves_file);
    // show the result
    SDL_RenderPresent(renderer);

    return 9; // all this page is full
}


int show_scores()
{/// returns 0 if no scores were found
    int i,score, scanned_line;
    H_M_S scanned_time;
    char c[30];


    FILE* scores_file = fopen("scores_file.txt", "r");
    if( scores_file == NULL)
        return 0;

    fgets(c, 30, scores_file);   // num of scores

    i=0;
    do
    {
        fscanf(scores_file , "%d\t%d:%d:%d\t%d\n", &score, &scanned_time.h, &scanned_time.m, &scanned_time.s, &scanned_line);

        /* print the score on the page */
        sprintf(c, "score: %d temps: %d:%d:%d essais: %d", score,scanned_time.h, scanned_time.m, scanned_time.s, scanned_line); // convert the score to  text
        blit_text(c, &dest->score_line[i]);

        i++;
    }
    while(!feof(scores_file));

    fclose(scores_file);
    // show the result
    SDL_RenderPresent(renderer);

    return 1; // done correctly
}

void show_score_inGame(int _score_)
{
            SDL_Rect score_dest = dest->time_area;
            score_dest.y += score_dest.h; // below time_area

            char score_text[13];
            sprintf(score_text, "score: %d", _score_); // convert the !! ORRIGINAL_SCORE !! into text
            blit_text(score_text, &score_dest);

            SDL_RenderPresent(renderer);
}











//_____________________________________________GAME_LOGIC_____________________________________________//
void init_data()
{
    int i,j,rand_;
    //random variables
    time_t t;
    srand((unsigned) time(&t));

    //current line & current position
    data.line=0;

    for(i=0;i<10;i++)
    {
        for(j=0;j<4;j++)
            data.matrix[i][j] = last_texture;
        // indexes should be set to their values not textures
        data.matrix[i][4] = data.matrix[i][5] = 0;
    }

    for(i=0;i<6;i++)
        data.bar[i] = i+1;

    //random values for code
        data.code[0] = rand()%6+1;
        for(i=1;i<4;i++)
        {
            rand_ = rand()%6+1;

            if(easy_mode) // sans repetition
            for(j=i-1;j>-1;j--)
            {
                if(rand_ == data.code[j])
                {
                    j=i;
                    rand_=rand()%6+1;
                }
            }
            // the new rand_ var is not equal to previous values
            data.code[i] = rand_;
        }

    // all balls are not excluded yet, and we don't know if they are in the combination
    for(i=0;i<6;i++)
        excluded[i] = in_comb[i] = false;

//_________ game time

    game_time = init_time();

//_________ conditional variables
    win_shown = false;
    loose_shown = false;
    cover_shown = true;
    game_over = false;
}

void calculate_indexes()
{/**
    - first we set all code bars as "not found yet" ( we did not find any match to them in the line )
    we do this in two steps:
        1- Calculate black indexes
        2- Calculate white indexes

* Calculate black indexes :
>>> for each line ball
    {
        if it is well positioned :
            we add a black index :)
            we remember that we found this code ball
    }

* Calculate white indexes :
>>> for each line ball
    {
        for each code ball
        {
            if we find a match with a code ball that is not found yet
            we add a white index:)
            we remember that this code ball was found
            we proceed to the next line ball and start over(break)
        }
    }

**/

    int i,j;
    bool found[4] = {false, false, false, false}; // if we find a ball in code, we can't find it again ...


// Black Indexes
    for(i=0;i<4;i++)
    {
        if(data.matrix[data.line][i] == data.code[i])
        {
            data.matrix[data.line][4]++; // black index
            found[i] = true;
        }
    }
// White Indexes
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            if(data.matrix[data.line][i] == data.code[j] && !found[j])
            {
                found[j] = true;
                data.matrix[data.line][5]++; // white index
                break; // !!! we must stop if we find it to avoid repetition !!!
            }
        }
    }
}

int submit_code()
{ /// this function calculates the result of submission of the code, shows it & goes to next line  automatically if done
    /// it returns __win__ if you won, __loose__ if you lost, -2 if not loose nor win  or -1 if submission is not valid
    /// it also shows the result on screen, and resets bar colors
    int i;

    if(line_is_empty() && data.line > 0 && data.line < 10) /* if line (if it exists & not the first line) is empty just copy the last line to it */
    {
        for(i=0;i<4;i++)
        {
            data.matrix[data.line][i] = data.matrix[data.line - 1][i];
            if(easy_mode)
                data.bar[data.matrix[data.line][i]-1] = last_texture; /* if easy mode, we need to remove every ball already used from bar */
        }
        if(easy_mode) update_screen(update_bar);
        update_screen(update_line);
        return -1; /* if line was empty, we don't want to submit the code yet, we just copy last line */
    }

    else
    // if the line is full & if it is valid !)
    if(get_available_line_pos() == -1 && data.line < 10)
    {
        calculate_indexes();
        line_shown = false;
        update_screen(update_line);
        line_shown = true;

        if(data.matrix[data.line][4] == 4) //if you won
        {
            win_shown = true;
            cover_shown = false;
            update_screen(update_code);
            return __win__;
        }
        else
        if(data.line < 9) // if it isn't the last line && we did not win yet
        {
            if(easy_mode)
            {
                // if it's easy mode, the bar won't be complete
                reset(__bar__);
                update_screen(update_bar);
            }
            data.line++;
            increment_line();
            update_screen(update_line);
            return -2;
        }
        else // you lost !!
        {
            loose_shown = true;
            cover_shown = false;
            update_screen(update_code);
            return __loose__;
        }

    }
    else
    {
        // if line is not full
        return -1 ;
    }


}

void save_game()
{
    int i;
    FILE* saves_file;
    char* c_time = current_time();

    saves_file = fopen("sauvegardes.txt", "a");

    fprintf(saves_file, c_time, '\n');
    fprintf(saves_file,"%d:%d:%d\n",game_time.h,game_time.m,game_time.s);
    fprintf(saves_file, "mode: %d\n", easy_mode);
    fprintf(saves_file, "line: %d\n", data.line);
    fprintf(saves_file, "code:\t%d\t%d\t%d\t%d\n", data.code[0], data.code[1], data.code[2], data.code[3] );
    fprintf(saves_file, "bar:\t%d\t%d\t%d\t%d\t%d\t%d\n", data.bar[0], data.bar[1], data.bar[2], data.bar[3], data.bar[4], data.bar[5]);
    fprintf(saves_file, "matrix:\n");
    for(i=9;i>-1;i--)//___ from bottom to top ____
        fprintf(saves_file, "%d\t%d\t%d\t%d\t%d\t%d\n", data.matrix[i][0], data.matrix[i][1], data.matrix[i][2], data.matrix[i][3], data.matrix[i][4], data.matrix[i][5]);
    fprintf(saves_file,"\n\n");

    //close file
    fclose(saves_file);

/*  example:
_______________________________________
    Mon Dec 26 22:43:38 2016
    0:0:22
    mode: 0
    line: 2
    code:	6	1	5	3
    bar:	1	2	3	4	5	6
    matrix:
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    6	5	2	2	0	0
    6	5	5	5	2	0
    6	6	6	6	1	0


    Mon Dec 26 22:46:44 2016
    0:0:7
    mode: 1
    line: 1
    code:	5	2	6	4
    bar:	1	24	3	4	5	6
    matrix:
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    24	24	24	24	0	0
    2	24	24	24	0	0
    5	6	4	2	1	3


_________________________________________
*/

}

void delete_save(int save_index, int page_index)
{/// the save MUST EXIST, test that before calling this function ,delete the save from saves file
    int i,j,n;
    const int FILE_LENGHTH = 19;
    char c[30] = "";

    FILE* saves_file = fopen("sauvegardes.txt", "r");
    FILE* tmp = fopen("tmp.txt","w");

    if(saves_file == NULL)
        return ;

    //____ Skip all pages before the "page" (one page holds 9 saves at Maximum) )

    for(j=1;j<page_index;j++) // till we get to the page
    {
        for(i=0;i<9;i++) // skip 9 saves (1 page)
        {
            for(n=FILE_LENGHTH; n>0; n--)
            {
                fgets(c, 30, saves_file);
                fputs(c, tmp);
            }
        }
    }

    //___ Skip all saves before the "save" in the page

    for(i=1;i<save_index;i++)
    {
        for(n=FILE_LENGHTH; n>0; n--)
        {
            fgets(c, 30, saves_file);
            fputs(c, tmp);
        }
    }

    //at this point we are exactly in the "save" we need, if it exists X__X

    for(n=FILE_LENGHTH; n>0; n--)
    {
        fgets(c, 30, saves_file);
        /* dont put it in tmp*/
    }

    // now we keep reading till the end of the file
    fgets(c, 30, saves_file);
    while(!feof(saves_file))
    {
        fputs(c, tmp);
        fgets(c, 30, saves_file);
    }

    fclose(saves_file);
    fclose(tmp);

    remove("sauvegardes.txt");
    rename("tmp.txt","sauvegardes.txt");
}

bool load_game(int save_index, int page_index)
{/// true if loaded successfully, else false
 /// page_index >= 1 , save_index in {1,2,3,...,9}

    int i,j,n;
    const int FILE_LENGHTH = 19;
    char c[30] = "";

    FILE* saves_file = fopen("sauvegardes.txt", "r");
    if(saves_file == NULL)
        return false;

    //____ Skip all pages before the "page" (one page holds 9 saves at Maximum) )

    for(j=1;j<page_index;j++) // till we get to the page
    {
        for(i=0;i<9;i++) // skip 9 saves (1 page)
        {
            //try to read first line
            fgets(c, 30, saves_file);
            if(feof(saves_file))
            {
                fclose(saves_file);
                return false; // save does not exist !
            }

            //keep reading (we skiped 1 line !! --> n>1 not n>0)
            for(n=FILE_LENGHTH; n>1; n--)
                fgets(c, 30, saves_file);
        }
    }

    //___ Skip all saves before the "save" in the page

    for(i=1;i<save_index;i++)
    {
        //try to read first line
        fgets(c, 30, saves_file);
        if(feof(saves_file))
        {
            fclose(saves_file);
            return false; // save does not exist !
        }

        //keep reading (we skiped 1 line !! --> n>1 not n>0)
        for(n=FILE_LENGHTH; n>1; n--)
            fgets(c, 30, saves_file);
    }

    //at this point we are exactly in the "save" we need, if it exists X__X

    fgets(c, 30, saves_file);//date
    if(feof(saves_file))
    {
        fclose(saves_file);
        return false; // save does not exist !
    }

    fscanf(saves_file,"%d:%d:%d\n", &game_time.h, &game_time.m, &game_time.s);
    fscanf(saves_file,"mode: %d\n", &easy_mode);
    fscanf(saves_file,"line: %d\n", &data.line);
    fscanf(saves_file,"code:\t%d\t%d\t%d\t%d\n", &data.code[0],&data.code[1],&data.code[2],&data.code[3]);
    fscanf(saves_file,"bar:\t%d\t%d\t%d\t%d\t%d\t%d\n", &data.bar[0],&data.bar[1],&data.bar[2],&data.bar[3],&data.bar[4],&data.bar[5]);
    fgets(c, 30, saves_file); //"matrix:\n"
    for(i=9;i>-1;i--) //__ from bottom to top ___
        fscanf(saves_file,"%d\t%d\t%d\t%d\t%d\t%d\n", &data.matrix[i][0], &data.matrix[i][1], &data.matrix[i][2], &data.matrix[i][3], &data.matrix[i][4], &data.matrix[i][5]);

    fclose(saves_file);
    //done
    return true;
}

int calculate_score()
{
    int score = 0;
    if(game_time.h == 0)
    {
        if(game_time.m == 0)
            score = (-6000*time_in_min(game_time) + 10000);
        else
        if(game_time.m <= 7)
            score = (-500*time_in_min(game_time) + 4500);
        else
            score = (-18.87*time_in_min(game_time) + 1132.08);

    }

    score -= data.line*100+100; // data.line starts with 0 ...
    score += 500*(!easy_mode);

    return score < 0 ? 0 : score;

}

int save_show_score(int score,H_M_S time_of_game, int game_line, bool show_score)
{
    FILE *scores_file, *tmp;

    int scanned_score;
    H_M_S scanned_time;
    int scanned_line;
    int compare;

    int orriginal_score = score; // save the original to show it later
    bool file_is_full = false; // to see if the scores filer has already 10 scores

        scores_file = fopen("scores_file.txt", "r");

        if(scores_file == NULL)
        // if the file does not exist .. or there is an error, we just make one and put the score in it
        {
            scores_file = fopen("scores_file.txt", "w");
            fprintf(scores_file, "number of scores = %d\n",  1); // MAX NUMBER IS 10
            fprintf(scores_file, "%d\t%d:%d:%d\t%d\n", score, time_of_game.h, time_of_game.m, time_of_game.s, game_line);

            fclose(scores_file);
        }
        else // we insert the score in the right position in the scores file ( keep it sorted ~ "trié" )
        {
            int scores_number;
            tmp = fopen("tmp.txt","w");

            fscanf(scores_file, "number of scores = %d\n", &scores_number);
            if(scores_number == 10) // max, can't add more to it
            {
                file_is_full = true;
                fprintf(tmp, "number of scores = %d\n", 10);
            }
            else
                fprintf(tmp, "number of scores = %d\n", scores_number+1); // add 1 to the score_number

            // now we write the scores in order
            do
            {

                fscanf(scores_file ,"%d\t%d:%d:%d\t%d\n", &scanned_score, &scanned_time.h, &scanned_time.m, &scanned_time.s, &scanned_line);
                if(scanned_score > score)
                    fprintf(tmp, "%d\t%d:%d:%d\t%d\n", scanned_score, scanned_time.h, scanned_time.m, scanned_time.s, scanned_line);
                else
                if(scanned_score < score)
                {
                    fprintf(tmp, "%d\t%d:%d:%d\t%d\n", score, time_of_game.h, time_of_game.m, time_of_game.s, game_line);
                    score = scanned_score; // we must write this scanned score to the file later ... so we keep the process as if score = scanned_score
                    time_of_game = scanned_time;
                    game_line = scanned_line;
                }
                else /* scanned_score  == score */
                {
                    compare = time_compare(time_of_game, scanned_time) ;
                    if( compare == 1 )  /* time_of_game > scanned_time */
                    {
                        fprintf(tmp, "%d\t%d:%d:%d\t%d\n", scanned_score, scanned_time.h, scanned_time.m, scanned_time.s, scanned_line);
                    }
                    else
                    if( compare == -1 ) /* time_of_game < scanned_time */
                    {
                        fprintf(tmp, "%d\t%d:%d:%d\t%d\n", score, time_of_game.h, time_of_game.m, time_of_game.s, game_line);
                        score = scanned_score; // we must write this scanned score to the file later ... so we keep the process as if score = scanned_score
                        time_of_game = scanned_time;
                        game_line = scanned_line;
                    }
                    else                /* time_of_game = scanned_time */
                    {
                        if( game_line > scanned_line )
                        {
                            fprintf(tmp, "%d\t%d:%d:%d\t%d\n", scanned_score, scanned_time.h, scanned_time.m, scanned_time.s, scanned_line);
                        }
                        else
                        if( game_line < scanned_line )
                        {
                            fprintf(tmp, "%d\t%d:%d:%d\t%d\n", score, time_of_game.h, time_of_game.m, time_of_game.s, game_line);
                            score = scanned_score; // we must write this scanned score to the file later ... so we keep the process as if score = scanned_score
                            time_of_game = scanned_time;
                            game_line = scanned_line;
                        }
                        else /* identical score ,time & lines ... so we abort the whole procedure of adding the score */
                        {
                            fclose(tmp);
                            fclose(scores_file);

                            remove("tmp_file.txt");
                            if(show_score) show_score_inGame(orriginal_score);
                            return -1 ; // score has not been added
                        }
                    }
                }
                scores_number--;
            }
            while(!feof(scores_file) && scores_number > 0);
            if(!file_is_full)
            {
                fprintf(tmp, "%d\t%d:%d:%d\t%d\n", score, time_of_game.h, time_of_game.m, time_of_game.s, game_line); // we print the score ( or the last scanned score if the original score was inserted ) to the file if not full
            }

            fclose(tmp);
            fclose(scores_file);

            remove("scores_file.txt");
            rename("tmp.txt", "scores_file.txt");
        }


        if(show_score) show_score_inGame(orriginal_score);



        return 0;
}











//________________________________________GAME_LOOPS_______________________________//
void saves_menu()
{
    under_mouse thing;
    SDL_Event event;
    bool running = true;
    int x,y;

    int page_indx = 1;
    int last_save = 0;

    indicator_tex = last_texture;

    // saves menu background
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex[17], NULL, &dest->saves_back);
    last_save = show_saves(1);
    SDL_RenderPresent(renderer);

    while(running)
    {
        while(SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_QUIT://______________________________
                            running = false;
                            break;

                        case SDL_KEYUP://_____________________________
                            if(event.key.keysym.sym == SDLK_ESCAPE)
                                running = false;
                            break;

                        case SDL_MOUSEMOTION://_______________________
                            x = event.motion.x;
                            y = event.motion.y;
                            thing = what_is_pressed(x,y,in_saves);
                            update_screen(update_saves);
                            break;

                        case SDL_MOUSEBUTTONDOWN://___________________
                            if(thing.type == saves_line && thing.i+1 <= last_save)
                            {
                                init_data(); // we must reset all to original values
                                load_game(thing.i+1, page_indx); // load the clicked save
                                game_is_paused = true; // consider the game as paused
                                return; // go back to main menu

                            }
                            else
                            if(thing.type == saves_left_right)
                            {
                                if(thing.i == 0 && page_indx > 1) // show previous page if possible
                                {
                                    page_indx--;

                                    SDL_RenderClear(renderer);
                                    SDL_RenderCopy(renderer, tex[17], NULL, &dest->saves_back);

                                    last_save = show_saves(page_indx);
                                    SDL_RenderPresent(renderer);
                                }
                                else
                                if(thing.i == 1 && last_save == 9) // show next page if possible
                                {
                                    page_indx++;

                                    SDL_RenderClear(renderer);
                                    SDL_RenderCopy(renderer, tex[17], NULL, &dest->saves_back);

                                    last_save = show_saves(page_indx);
                                    SDL_RenderPresent(renderer);
                                    if(last_save == 0)
                                    {
                                        last_save = show_saves(--page_indx); // go back to last page
                                    }
                                }
                            }
                            else
                            if( thing.type == saves_line_delete && thing.i+1 <= last_save )
                            {
                                delete_save(thing.i+1, page_indx);
                                SDL_RenderCopy(renderer, tex[17], NULL, &dest->saves_back); // background
                                last_save = show_saves(page_indx);
                                SDL_RenderPresent(renderer);
                            }

                            break;



                    }
                }
    }

}

void scores_menu()
{
    SDL_Event event;
    bool running = true;

    indicator_tex = last_texture;
    update_screen(update_scores);
    show_scores();

    while(running)
    {
        while(SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_QUIT://______________________________
                            running = false;
                            break;

                        case SDL_KEYUP://_____________________________
                            if(event.key.keysym.sym == SDLK_ESCAPE)
                                running = false;
                                break;

                    }
                }
    }

}

void GameLoop()
{ /// one player main game loop
    bool running = true;

    SDL_Event event;
    int x,y; // mouse position
    int info, info2, pos; // many uses ...
    under_mouse thing; //what is pressed

    clock_t last_time, period;
    char* time_text;
/*
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds", msec/1000, msec%1000);
*/

// initialize game
    if(!game_is_paused) /* if game is paused, don't reset data */
        init_data();
    redraw_everything(true);

    last_time = clock();    // init time


//start
    while(running)
    {
        // process & show time
        if(!game_over)
        {
            period = (clock() - last_time)/CLOCKS_PER_SEC;
            if(period>= 1)
            {
                increment_time(&game_time,period);
                last_time = clock();
            }



            time_text = hour_min_sec(&game_time);
            clip_and_replace(dest->time_area, in_game);
            blit_text(time_text, &dest->time_area);
            SDL_RenderPresent(renderer);
        }
        //__________________


         /*if game is not over yet */
        if(!game_over)
        // Process events
        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT://______________________________
                    running = false;
                    game_is_paused = false;
                    break;

                case SDL_KEYDOWN://___________________________
                    if(event.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        finish_shown = true;
                        update_screen(update_finish);
                    }
                    break;

                case SDL_KEYUP://_____________________________
                    if(event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        running = false;
                        game_is_paused = !game_over; /* if game is over, we need to reload data */
                        break;
                    }
                    else
                    if(event.key.keysym.sym == SDLK_KP_ENTER)
                    {
                        finish_shown = false;
                        info = submit_code();
                        if(info == __win__ || info == __loose__)
                            game_over = true;
                        update_screen(update_finish);
                    }
                    else
                    if(event.key.keysym.sym == SDLK_m) /* pressing m is just like pressing a double click */
                    {
                        thing = what_is_pressed(x,y, in_game);
                        /* bar ball to line */
                        if(thing.type == bar_ball && data.bar[thing.i] != last_texture)
                        {
                            pos = get_available_line_pos();
                            if(pos != -1) // if there is a place to drop the ball !!!
                            {
                                data.matrix[data.line][pos] = data.bar[thing.i];
                                if(easy_mode)
                                    data.bar[thing.i] = last_texture; // delete the ball from bar
                                update_screen(update_bar);
                                update_screen(update_line);
                            }
                        }

                        //_____________________________________________________________________________//

                        else /* line ball to bar */
                        if(thing.type == line_ball && data.matrix[data.line][thing.i] != last_texture)
                        {
                            info = data.matrix[data.line][thing.i];
                            data.matrix[data.line][thing.i] = last_texture; // delete the ball from line
                            data.bar[info-1] = info; // put the ball to bar
                            update_screen(update_bar);
                            update_screen(update_line);
                        }
                    }
                    break;

                case SDL_MOUSEMOTION://_______________________
                    x = event.motion.x;
                    y = event.motion.y;
                    break;

                case SDL_MOUSEBUTTONDOWN://___________________

                    //see what is under mouse
                    thing = what_is_pressed(x,y,in_game);

                    /* always executed */
                    if(thing.type == finish_button)
                    {
                        finish_shown = true;
                        finish_was_pressed = true;
                        update_screen(update_finish);
                        break;
                    }

                    /* now we need to determine the number of clicks */
                    if(SDL_WaitEventTimeout(&event, 10)) // wait, maybe he will release
                    {
                        if(event.type == SDL_MOUSEBUTTONUP)
                            if(!SDL_WaitEventTimeout(&event, 100)) // wait maybe he will press again
                                break; //if he just click and release we get out
                            else
                                if(event.type != SDL_MOUSEBUTTONDOWN) //if he don't press again, it's neither a drag not a double click
                                    break;
                    }

                    /* cases that depend on the number of clicks */

                    //see what is under mouse
                    thing = what_is_pressed(x,y,in_game);

                    if(event.button.clicks == 1)
                    {
                        /* drag a bar ball to line */
                        if(thing.type == bar_ball && data.bar[thing.i] != last_texture)
                        {
                            /* save the ball's infos (position and color) and remove it*/
                            pos = thing.i;
                            info = data.bar[pos];
                            data.bar[pos] = last_texture;
                            /* animate drag*/
                            follow_mouse(info, &x, &y, __bar__);

                            /* where did you drop the ball ? */
                            thing = what_is_pressed(x,y,in_game);

                            if(thing.type == line_ball) /* if you drop it in the line, we put it there .. */
                            {
                                if(!easy_mode) /*we  put ball back to line if not easy mode*/
                                        data.bar[info-1] = info;

                                info2 = data.matrix[data.line][thing.i]; /*save the color of the ball found "there"*/
                                if(easy_mode && info2 != last_texture) /*we  put what was found in "there" back to bar*/
                                        data.bar[info2-1] = info2;

                                data.matrix[data.line][thing.i] = info; /* finally put the ball in "there" */

                                update_screen(update_bar);
                                update_screen(update_line);
                            }
                            else /* if you don't drop it in the line ... we put it back*/
                            {
                                data.bar[pos] = info;
                                update_screen(update_bar);
                            }
                        }

                        //______________________________________________________________________________________//

                        else /* drag a line ball to: 1) swap it with another line ball - 2) swap with a bar ball 3) */
                        if(thing.type == line_ball && data.matrix[data.line][thing.i] != last_texture)
                        {
                            /* save the ball's infos (position and color) and remove it*/
                            pos = thing.i;
                            info = data.matrix[data.line][pos];
                            data.matrix[data.line][pos] = last_texture;
                            /* animate drag*/
                            follow_mouse(info, &x, &y, __line__);

                            /* where did you drop the ball ? */
                            thing = what_is_pressed(x,y,in_game);

                            if(thing.type == line_ball) /* if you drop it in the line, we swap it with what was there  .. */
                            {
                                data.matrix[data.line][pos] = data.matrix[data.line][thing.i];
                                data.matrix[data.line][thing.i] = info;

                                update_screen(update_bar);
                                update_screen(update_line);
                            }
                            else
                            if(thing.type == bar_ball) /* if you drop it in the bar, we swap it with the chosen bar ball */
                            {
                                data.matrix[data.line][pos] = data.bar[thing.i]; // replace with chosen ball
                                if(easy_mode)
                                {
                                    data.bar[thing.i] = last_texture; // remove the chosen ball from bar
                                    data.bar[info-1] = info;
                                    update_screen(update_bar);
                                }
                                update_screen(update_line);
                            }
                            else /* if you don't drop it in the line or bar ... we put it back to the original position */
                            {
                                data.matrix[data.line][pos] = info;
                                update_screen(update_line);
                            }
                        }
                    }
                    else
                    if(event.button.clicks == 2)
                    {
                        /* bar ball to line */
                        if(thing.type == bar_ball && data.bar[thing.i] != last_texture)
                        {
                            pos = get_available_line_pos();
                            if(pos != -1) // if there is a place to drop the ball !!!
                            {
                                data.matrix[data.line][pos] = data.bar[thing.i];
                                if(easy_mode)
                                    data.bar[thing.i] = last_texture; // delete the ball from bar
                                update_screen(update_bar);
                                update_screen(update_line);
                            }
                        }

                        //_____________________________________________________________________________//

                        else /* line ball to bar */
                        if(thing.type == line_ball && data.matrix[data.line][thing.i] != last_texture)
                        {
                            info = data.matrix[data.line][thing.i];
                            data.matrix[data.line][thing.i] = last_texture; // delete the ball from line
                            data.bar[info-1] = info; // put the ball to bar
                            update_screen(update_bar);
                            update_screen(update_line);
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONUP://_______________________
                    if(finish_was_pressed)
                    {
                        finish_shown = false;
                        finish_was_pressed = false;
                        update_screen(update_finish);

                        thing = what_is_pressed(x,y,in_game);
                        if(thing.type == finish_button)
                        {
                            info = submit_code();
                            if(info == __win__ || info == __loose__)
                                game_over = true;
                        }

                    }
                    break;

            }//switch

        }//while(events)
        else /*if game is over*/
        {

            // calculate, show and save score IF THE PLAYER ACTUALLY WON
            if(win_shown)
            {
                info = calculate_score();
                save_show_score(info, game_time, data.line + 1, true); // true->show score
            }

            while( running ) // we add this to avoid saving the score multiple times ...
            {
                while(SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_QUIT://______________________________
                            running = false;
                            game_over = false; // prepare for reloading a new game
                            game_is_paused = false;
                            break;

                        case SDL_KEYUP://_____________________________
                            if(event.key.keysym.sym == SDLK_ESCAPE)
                            {
                                game_over = false;
                                running = false; // prepare for reloading a new game
                                game_is_paused = false;
                                indicator_tex = last_texture;
                            }
                            break;
                    }
                }
            }
        }

    }//while(running)

}

/*void new_game()
{
    under_mouse thing;
    SDL_Event event;
    bool running = true;
    int x,y;

    indicator_tex = last_texture;
    update_screen(update_nouv);
    while(running)
    {
        while(SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_QUIT://______________________________
                            running = false;
                            break;

                        case SDL_KEYUP://_____________________________
                            if(event.key.keysym.sym == SDLK_ESCAPE)
                                running = false;
                            break;

                        case SDL_MOUSEMOTION://_______________________
                            x = event.motion.x;
                            y = event.motion.y;
                            thing = what_is_pressed(x,y, in_nouv);
                            update_screen(update_nouv);
                            break;

                        case SDL_MOUSEBUTTONDOWN://___________________
                            if(thing.type == nouv_line_ && thing.i == 0)
                            {
                                GameLoop();
                                return;
                            }
                            break;



                    }
                }
    }

}*/

void main_menu()
{ /// before the game starts, this procedure decides what are the game options

    under_mouse thing;
    SDL_Event event;
    bool running = true;
    int x=0,y=0;


    indicator_tex = last_texture;
    update_screen(update_main_menu);
    while(running)
    {
        while(SDL_PollEvent(&event))
                {
                    switch (event.type)
                    {
                        case SDL_QUIT://______________________________
                            running = false;
                            break;

                        case SDL_KEYUP://_____________________________
                            if(event.key.keysym.sym == SDLK_ESCAPE)
                            {
                                if(game_is_paused)
                                {
                                    GameLoop(); // resume game
                                    update_screen(update_main_menu); // redraw the main menu screen
                                }
                                else
                                    running = false;
                            }
                            break;


                        case SDL_MOUSEMOTION://_______________________
                            x = event.motion.x;
                            y = event.motion.y;
                            thing = what_is_pressed(x,y, in_main_menu);
                            update_screen(update_main_menu);
                            break;

                        case SDL_MOUSEBUTTONDOWN://___________________
                            thing = what_is_pressed(x,y, in_main_menu);
                            if(thing.type == main_mode_)
                            {
                                if(!game_is_paused) /* if game is paused, you can't change the mode */
                                {
                                    easy_mode = !thing.i;
                                    dest->under_line.x = dest->main_mode[thing.i].x;
                                    update_screen(update_main_menu);
                                }
                            }
                            else
                            if(thing.type == main_nouv_) /* new_game ,or Continue if game paused */
                            {
                                GameLoop();
                                indicator_tex = last_texture; // we need to clear the last indicator...
                                update_screen(update_main_menu);
                            }
                            else
                            if(thing.type == main_saves_) /* saves list ,or save the actual game if game paused */
                            {
                                if(game_is_paused)
                                {
                                    save_game();
                                    GameLoop();
                                }
                                else
                                {
                                    saves_menu();
                                    indicator_tex = last_texture; // we need to clear the last indicator...
                                }
                                update_screen(update_main_menu); // don't forget to redraw the main menu screen
                            }
                            else
                            if(thing.type == main_scores_) /* scores list ,or go back to menu if game paused */
                            {
                                if(game_is_paused)
                                    game_is_paused = false;
                                else
                                    scores_menu();

                                update_screen(update_main_menu);
                            }


                    }
                }
    }



}
















//________________________________________AI_________________________________________//

/*void AI_loop()
{ /// we declare the AI useful data here to optimize Memory

    bool ball_in [6][4] =    {false};     // i ball probably in position j
    bool ball_not_in[6][4] = {false};     // i ball surely not in position j
    int  found[4] = {-1,-1,-1,-1};        // initially {-1,-1,-1,-1}, general {2,3,-1,0}
    int  index[2], clean_index[2];

    int phase = 1;                        // phase_1 = 1, phase_2 = 2




}*/












//_____________________________________________MAIN__________________________________________________//

int main(int argc, char **argv)
{
    initialize_SDL();
    loadResources();
    main_menu();
    releaseResources(last_texture);

    return 0;
}




