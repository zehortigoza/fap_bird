#include <Elementary.h>
#include <EPhysics.h>

#define WIDTH 800
#define HEIGHT 600

#define BIRD_SIZE 50
#define GROUND_HEIGHT 50

#define WORLD_HEIGHT (HEIGHT-GROUND_HEIGHT)

#define PIPES 4

#define PIPES_DISTANCE 300
#define PIPES_GAPS_HEIGHT 150
#define PIPE_GAPS_HEIGHT_HALF (PIPES_GAPS_HEIGHT / 2)
#define PIPE_WIDTH 75

static int _fap_bird_test_log_dom;

#define ERR(...) EINA_LOG_DOM_ERR(_fap_bird_test_log_dom, __VA_ARGS__)

static EPhysics_World *world;
static EPhysics_Body *bird_body, *sky_body;
static Eina_Inlist *pipes = NULL;
static Eina_Bool keep_moving_world = EINA_TRUE;
static EPhysics_Quaternion *bird_default_rotation;
static Eina_Bool initialized = EINA_FALSE;
static Evas_Object *score_text;
static int score = 0;
static Evas_Coord bird_x;

typedef struct {
   EINA_INLIST;
   EPhysics_Body *top, *bottom;
   Eina_Bool consumed;
} Pipe;

static int
_pipe_position_calc_and_place(Pipe *pipe, int x)
{
   int pipe_start_y = rand() % WORLD_HEIGHT;
   int y, h;
   Evas_Object *pipe_top_image, *pipe_bottom_image;

   if (pipe_start_y < PIPES_GAPS_HEIGHT)
     pipe_start_y = PIPES_GAPS_HEIGHT;
   else if (pipe_start_y > (WORLD_HEIGHT - PIPES_GAPS_HEIGHT))
     pipe_start_y = WORLD_HEIGHT - PIPES_GAPS_HEIGHT;

   pipe_top_image = ephysics_body_evas_object_get(pipe->top);
   pipe_bottom_image = ephysics_body_evas_object_get(pipe->bottom);

   y = pipe_start_y - PIPE_GAPS_HEIGHT_HALF;
   x += (PIPES_DISTANCE + PIPE_WIDTH);
   evas_object_geometry_set(pipe_top_image, x, 0, PIPE_WIDTH, y);
   ephysics_body_geometry_set(pipe->top, x, 0, 1, PIPE_WIDTH, y, 1);

   y = pipe_start_y + PIPE_GAPS_HEIGHT_HALF;
   h = WORLD_HEIGHT - y;
   evas_object_geometry_set(pipe_bottom_image, x, y, PIPE_WIDTH, h);
   ephysics_body_geometry_set(pipe->bottom, x, y, 1, PIPE_WIDTH, h, 1);

   pipe->consumed = EINA_FALSE;

   return x;
}

static void
score_update(void)
{
   char text[512];
   sprintf(text, "%d", score);
   evas_object_textblock_text_markup_set(score_text, text);
}

static Eina_Bool
_move_world(void *data)
{
   Pipe *move = NULL;
   Pipe *pipe;
   Evas_Coord x;

   if (!keep_moving_world)
     return EINA_TRUE;

   EINA_INLIST_FOREACH(pipes, pipe)
     {
        Evas_Coord y, z;
        ephysics_body_geometry_get(pipe->top, &x, &y, &z, NULL, NULL, NULL);
        ephysics_body_move(pipe->top, x-1, y, z);

        ephysics_body_geometry_get(pipe->bottom, &x, &y, &z, NULL, NULL, NULL);
        ephysics_body_move(pipe->bottom, x-1, y, z);

        if (bird_x > x && pipe->consumed == EINA_FALSE)
          {
             pipe->consumed = EINA_TRUE;
             score++;
             score_update();
          }
        else if (move == NULL && x + PIPE_WIDTH < -1)
          {
             move = pipe;
          }
     }

   if (move != NULL)
     {
        pipes = eina_inlist_remove(pipes, EINA_INLIST_GET(move));
        _pipe_position_calc_and_place(move, x);
        pipes = eina_inlist_append(pipes, EINA_INLIST_GET(move));
     }
   return EINA_TRUE;
}

static void
_key_pressed(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if (!keep_moving_world)
     return;
   if (strcmp(ev->keyname, "space"))
     return;

   ephysics_body_linear_velocity_set(bird_body, 0, -170, 0);
}

static void
_pipe_create(Evas *evas, Pipe *pipe)
{
   Evas_Object *pipe_top_image, *pipe_bottom_image;

   pipe_top_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_top_image, 255, 0, 0, 255);
   evas_object_show(pipe_top_image);
   evas_object_resize(pipe_top_image, 10, 10);
   pipe->top = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe->top, pipe_top_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe->top, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe->top, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe->top, EPHYSICS_BODY_MASS_STATIC);

   pipe_bottom_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_bottom_image, 255, 0, 0, 255);
   evas_object_show(pipe_bottom_image);
   evas_object_resize(pipe_bottom_image, 10, 10);
   pipe->bottom = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe->bottom, pipe_bottom_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe->bottom, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe->bottom, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe->bottom, EPHYSICS_BODY_MASS_STATIC);
}

static void
_generate_pipes_position(Evas *evas)
{
   Pipe *pipe;
   Evas_Coord x = WIDTH;

   if (!initialized)
     {
        int i;
        for (i = 0; i < PIPES; i++)
          {
             pipe = malloc(sizeof(Pipe));
             pipes = eina_inlist_append(pipes, EINA_INLIST_GET(pipe));
             _pipe_create(evas, pipe);
          }
        initialized = EINA_TRUE;
     }

   EINA_INLIST_FOREACH(pipes, pipe)
     {
        x = _pipe_position_calc_and_place(pipe, x);
     }
}

static void
_restart_btn_clicked(void *popup, Evas_Object *obj, void *event_info)
{
   evas_object_del(popup);
   
   //Reset
   ephysics_body_rotation_set(bird_body, bird_default_rotation);
   ephysics_body_angular_velocity_set(bird_body, 0, 0, 0);
   ephysics_body_linear_velocity_set(bird_body, 0, 0, 0);
   ephysics_body_move(bird_body, WIDTH / 4, WORLD_HEIGHT / 2, 1);
   _generate_pipes_position(evas_object_evas_get(obj));

   score = 0;
   score_update();
   keep_moving_world = EINA_TRUE;
}

static void
_show_game_over_popup(Evas_Object *win)
{
   Evas_Object *btn, *popup = elm_popup_add(win);
   elm_object_part_text_set(popup, "title,text", "Game Over");

   btn = elm_button_add(popup);
   elm_object_text_set(btn, "Restart");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", _restart_btn_clicked, popup);

   evas_object_show(popup);
}

static void
_bird_update_cb(void *data, EPhysics_Body *body, void *event_info)
{
   Evas_Coord y;

   // To fix some physics bug that was causing the bird across the sky barrier
   ephysics_body_geometry_get(bird_body, NULL, &y, NULL, NULL, NULL, NULL);
   if (y < 5)
     {
        ephysics_body_central_impulse_apply(bird_body, 0, 200, 0);
     }

   ephysics_body_evas_object_update(bird_body);
}

static void
_collision_cb(void *win, EPhysics_Body *body, void *event_info)
{
   EPhysics_Body_Collision *collision = event_info;
   EPhysics_Body *contact;

   contact = ephysics_body_collision_contact_body_get(collision);
   if (contact == sky_body)
     return;

   if (keep_moving_world)
     {
        keep_moving_world = EINA_FALSE;
       _show_game_over_popup(win);
    }
}

static int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win, *bg, *ground_image, *bird_image;
   EPhysics_Body *ground_body;
   Evas *evas;
   Evas_Textblock_Style *style;

   _fap_bird_test_log_dom = eina_log_domain_register("fap_bird", EINA_COLOR_GREEN);
   if (_fap_bird_test_log_dom < 0)
     {
        EINA_LOG_CRIT("Could not register log domain: fap_bird");
        return -1;
     }

   if (!ephysics_init()) return - 1;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
   elm_win_title_set(win, "Fap bird");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_resize(win, WIDTH, HEIGHT);
   evas_object_show(win);

   bg = elm_bg_add(win);
   elm_bg_color_set(bg, 0, 0, 255);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   world = ephysics_world_new();
   ephysics_world_render_geometry_set(world, 0, 0, 0, WIDTH, HEIGHT, 1);

   evas = evas_object_evas_get(win);

   sky_body = ephysics_body_box_add(world);
   ephysics_body_geometry_set(sky_body, 0, 0, 1, WIDTH, 2, 1);
   ephysics_body_restitution_set(sky_body, EPHYSICS_BODY_RESTITUTION_CONCRETE);
   ephysics_body_friction_set(sky_body, EPHYSICS_BODY_FRICTION_CONCRETE);
   ephysics_body_mass_set(sky_body, EPHYSICS_BODY_MASS_STATIC);

   ground_image = evas_object_rectangle_add(evas);
   evas_object_color_set(ground_image, 0, 255, 0, 255);
   evas_object_geometry_set(ground_image, 0, WORLD_HEIGHT, WIDTH, GROUND_HEIGHT);
   evas_object_show(ground_image);
   ground_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(ground_body, ground_image, EINA_TRUE);
   ephysics_body_restitution_set(ground_body, EPHYSICS_BODY_RESTITUTION_CONCRETE);
   ephysics_body_friction_set(ground_body, EPHYSICS_BODY_FRICTION_CONCRETE);
   ephysics_body_mass_set(ground_body, EPHYSICS_BODY_MASS_STATIC);

   bird_image = evas_object_rectangle_add(evas);
   evas_object_color_set(bird_image, 255, 255, 255, 255);
   evas_object_geometry_set(bird_image, WIDTH/4, WORLD_HEIGHT/2, BIRD_SIZE, BIRD_SIZE);
   bird_x = WIDTH/4 + BIRD_SIZE;
   evas_object_show(bird_image);
   bird_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(bird_body, bird_image, EINA_TRUE);
   ephysics_body_restitution_set(bird_body, EPHYSICS_BODY_RESTITUTION_PLASTIC);
   ephysics_body_friction_set(bird_body, EPHYSICS_BODY_FRICTION_PLASTIC);
   ephysics_body_mass_set(bird_body, 1.5);
   ephysics_body_event_callback_add(bird_body, EPHYSICS_CALLBACK_BODY_COLLISION,
                                    _collision_cb, win);
   ephysics_body_event_callback_add(bird_body, EPHYSICS_CALLBACK_BODY_UPDATE,
                                    _bird_update_cb, NULL);
   bird_default_rotation = ephysics_quaternion_new();

   _generate_pipes_position(evas);

   score_text = evas_object_textblock_add(evas);
   evas_object_move(score_text, 0, WORLD_HEIGHT * 0.07);
   evas_object_resize(score_text, WIDTH, 50);
   style = evas_textblock_style_new();
   evas_textblock_style_set(style, "DEFAULT='font=Sans font_size=30 style=shadow color=#FFF text_class=entry align=center shadow_color=#000'");
   evas_object_textblock_style_set(score_text, style);
   evas_object_textblock_text_markup_set(score_text, "0");
   evas_object_show(score_text);

   ecore_timer_add(0.01, _move_world, NULL);
   evas_object_event_callback_add(win, EVAS_CALLBACK_KEY_UP, _key_pressed, bird_body);

   elm_run();

   free(bird_default_rotation);
   evas_textblock_style_free(style);
   ephysics_shutdown();
   eina_log_domain_unregister(_fap_bird_test_log_dom);
   _fap_bird_test_log_dom = -1;

   elm_shutdown();
   return 0;
}
ELM_MAIN()
