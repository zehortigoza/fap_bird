#include <Elementary.h>
#include <EPhysics.h>

#define WIDTH 800
#define HEIGHT 600

#define GROUND_HEIGHT 50

#define WORLD_HEIGHT (HEIGHT-GROUND_HEIGHT)

static int _fap_bird_test_log_dom;

#define ERR(...) EINA_LOG_DOM_ERR(_fap_bird_test_log_dom, __VA_ARGS__)

static EPhysics_World *world;
static EPhysics_Body *bird_body, *pipe_1_body, *pipe_2_body, *sky_body;
static Eina_Bool keep_moving_world = EINA_TRUE;
static EPhysics_Quaternion *bird_default_rotation;

static Eina_Bool
_move_world(void *data)
{
   Evas_Coord x, y, z;

   if (!keep_moving_world)
     return EINA_TRUE;

   ephysics_body_geometry_get(pipe_1_body, &x, &y, &z, NULL, NULL, NULL);
   ephysics_body_move(pipe_1_body, x-1, y, z);

   ephysics_body_geometry_get(pipe_2_body, &x, &y, &z, NULL, NULL, NULL);
   ephysics_body_move(pipe_2_body, x-1, y, z);
   return EINA_TRUE;
}

static Eina_Bool
_key_pressed(void *data, Evas_Object *obj, Evas_Object *src, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   EPhysics_Body *bird_body = data;

   if (!keep_moving_world)
     return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_UP)
     return EINA_FALSE;
   if (strcmp(ev->keyname, "space"))
     return EINA_FALSE;

   ephysics_body_linear_velocity_set(bird_body, 0, -170, 0);
   return EINA_TRUE;
}

static void
_generate_pipes_position()
{
   Evas_Object *pipe_top_image, *pipe_bottom_image;
   int i, j;

   pipe_top_image = ephysics_body_evas_object_get(pipe_1_body);
   pipe_bottom_image = ephysics_body_evas_object_get(pipe_2_body);

   ephysics_body_geometry_set(pipe_1_body, WIDTH, 0, 1, 75, WORLD_HEIGHT/2-100, 1);
   evas_object_geometry_set(pipe_top_image, WIDTH, 0, 75, WORLD_HEIGHT/2-100);

   i = WORLD_HEIGHT / 2;
   j = WORLD_HEIGHT - i;
   evas_object_geometry_set(pipe_bottom_image, WIDTH, i, 75, j);
   ephysics_body_geometry_set(pipe_2_body, WIDTH, i, 1, 75, j, 1);
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
   _generate_pipes_position();

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
_bird_update_cb(void *data, EPhysics_Body *bird_body, void *event_info)
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
_collision_cb(void *win, EPhysics_Body *bird_body, void *event_info)
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

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win, *bg, *ly, *ground_image, *bird_image, *pipe_1_image, *pipe_2_image;
   EPhysics_Body *ground_body;
   Evas *evas;

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
   evas_object_geometry_set(bird_image, WIDTH/4, WORLD_HEIGHT/2, 50, 50);
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

   pipe_1_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_1_image, 255, 0, 0, 255);
   evas_object_show(pipe_1_image);
   pipe_1_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe_1_body, pipe_1_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe_1_body, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe_1_body, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe_1_body, EPHYSICS_BODY_MASS_STATIC);

   pipe_2_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_2_image, 255, 0, 0, 255);
   evas_object_show(pipe_2_image);
   pipe_2_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe_2_body, pipe_2_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe_2_body, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe_2_body, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe_2_body, EPHYSICS_BODY_MASS_STATIC);

   _generate_pipes_position();

   ecore_timer_add(0.01, _move_world, NULL);
   elm_object_event_callback_add(win, _key_pressed, bird_body);

   elm_run();

   free(bird_default_rotation);
   ephysics_shutdown();
   eina_log_domain_unregister(_fap_bird_test_log_dom);
   _fap_bird_test_log_dom = -1;

   elm_shutdown();
   return 0;
}
ELM_MAIN()
