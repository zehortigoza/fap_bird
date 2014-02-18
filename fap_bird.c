#include <Elementary.h>
#include <EPhysics.h>

#define WIDTH 800
#define HEIGHT 600

#define GROUND_HEIGHT 50

static int _fap_bird_test_log_dom;

#define ERR(...) EINA_LOG_DOM_ERR(_fap_bird_test_log_dom, __VA_ARGS__)

static Eina_Bool
_move_bird(void *data)
{
   EPhysics_Body *bird_body = data;
   ephysics_body_central_impulse_apply(bird_body, 30, 0, 0);
   return EINA_TRUE;
}

static Eina_Bool
_key_pressed(void *data, Evas_Object *obj, Evas_Object *src, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   EPhysics_Body *bird_body = data;

   if (type != EVAS_CALLBACK_KEY_UP)
     return EINA_FALSE;
   if (strcmp(ev->keyname, "space"))
      return EINA_FALSE;

   ephysics_body_central_impulse_apply(bird_body, 0, -200, 0);
   return EINA_TRUE;
}

static void
_restart_btn_clicked(void *data, Evas_Object *obj, void *event_info)
{
   ERR("TODO restart world");
}

static void
_collision_cb(void *data, EPhysics_Body *bird_body, void *event_info)
{
   EPhysics_Body_Collision *collision = event_info;
   int y;

   ephysics_body_collision_position_get(collision, NULL, &y, NULL);
   if (y == HEIGHT-GROUND_HEIGHT)
     {
        Evas_Object *btn, *popup = elm_popup_add(data);
        ephysics_world_running_set(ephysics_body_world_get(bird_body), EINA_FALSE);
        elm_object_part_text_set(popup, "title,text", "Game Over");

        btn = elm_button_add(popup);
        elm_object_text_set(btn, "Restart");
        elm_object_part_content_set(popup, "button1", btn);
        evas_object_smart_callback_add(btn, "clicked", _restart_btn_clicked, popup);

        evas_object_show(popup);
     }
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win, *bg, *ly, *ground_image, *bird_image, *pipe_1_image, *pipe_2_image;
   EPhysics_World *world;
   EPhysics_Body *ground_body, *bird_body, *pipe_1_body, *pipe_2_body;
   Evas *evas;
   int i, j;

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
   elm_bg_color_set(bg, 0, 255, 0);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   world = ephysics_world_new();
   ephysics_world_render_geometry_set(world, 0, 0, 0, WIDTH, HEIGHT-GROUND_HEIGHT, 1);

   evas = evas_object_evas_get(win);

   ground_image = evas_object_rectangle_add(evas);
   evas_object_color_set(ground_image, 0, 0, 0, 255);
   evas_object_geometry_set(ground_image, 0, HEIGHT-GROUND_HEIGHT, WIDTH, GROUND_HEIGHT);
   evas_object_show(ground_image);
   ground_body = ephysics_body_bottom_boundary_add(world);

   bird_image = evas_object_rectangle_add(evas);
   evas_object_color_set(bird_image, 0, 0, 255, 255);
   evas_object_geometry_set(bird_image, 50, (HEIGHT-GROUND_HEIGHT)/2, 50, 50);
   evas_object_show(bird_image);
   bird_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(bird_body, bird_image, EINA_TRUE);
   ephysics_body_restitution_set(bird_body, EPHYSICS_BODY_RESTITUTION_RUBBER);
   ephysics_body_friction_set(bird_body, EPHYSICS_BODY_FRICTION_RUBBER);
   ephysics_body_event_callback_add(bird_body, EPHYSICS_CALLBACK_BODY_COLLISION,
                                    _collision_cb, win);

   pipe_1_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_1_image, 255, 0, 0, 255);
   evas_object_geometry_set(pipe_1_image, WIDTH/2, 0, 75, (HEIGHT-GROUND_HEIGHT)/2-100);
   evas_object_show(pipe_1_image);
   pipe_1_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe_1_body, pipe_1_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe_1_body, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe_1_body, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe_1_body, EPHYSICS_BODY_MASS_STATIC);

   pipe_2_image = evas_object_rectangle_add(evas);
   evas_object_color_set(pipe_2_image, 255, 0, 0, 255);
   i = (HEIGHT-GROUND_HEIGHT)/2;
   j = HEIGHT-GROUND_HEIGHT - i;
   evas_object_geometry_set(pipe_2_image, WIDTH/2, i, 75, j);
   evas_object_show(pipe_2_image);
   pipe_2_body = ephysics_body_box_add(world);
   ephysics_body_evas_object_set(pipe_2_body, pipe_2_image, EINA_TRUE);
   ephysics_body_restitution_set(pipe_2_body, EPHYSICS_BODY_RESTITUTION_IRON);
   ephysics_body_friction_set(pipe_2_body, EPHYSICS_BODY_FRICTION_IRON);
   ephysics_body_mass_set(pipe_2_body, EPHYSICS_BODY_MASS_STATIC);

   ecore_timer_add(0.3, _move_bird, bird_body);
   elm_object_event_callback_add(win, _key_pressed, bird_body);

   elm_run();

   ephysics_shutdown();
   eina_log_domain_unregister(_fap_bird_test_log_dom);
   _fap_bird_test_log_dom = -1;

   elm_shutdown();
   return 0;
}
ELM_MAIN()
