#include <clutter/clutter.h>
#include <clutter/clutter-container.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "clutter-dominatrix.h"

#define SIZE_X 80
#define MARG 4

static void
keypress_cb (ClutterStage    *stage,
	     ClutterEvent    *event,
	     gpointer         data)
{
  gchar keybuf[9];
  int   len = 0;

  switch (event->type)
    {
    case CLUTTER_KEY_PRESS:
      len = g_unichar_to_utf8 (clutter_keysym_to_unicode (event->key.keyval),
			       keybuf);
      if (keybuf[0] == 'Q' ||
	  keybuf[0] == 'q')
	exit (0);
      
      break;
    default:;
    }
}

static ClutterDominatrix *
make_img_item (ClutterActor * stage, const gchar * name)
{
  ClutterActor      * img, * rect, * group, * shaddow;
  ClutterDominatrix * dmx;
  ClutterColor        bckg_clr = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor        shdw_clr = { 0x44, 0x44, 0x44, 0x44 };
  GdkPixbuf         * pixbuf;
  gdouble             scale;
  gint                w, h, sw, sh, x, y;
  ClutterFixed        zang;
  
  pixbuf = gdk_pixbuf_new_from_file_at_size (name, 400, 400, NULL);

  if (!pixbuf)
    return NULL;

  scale = (double) SIZE_X / (double) gdk_pixbuf_get_width (pixbuf);
  w = SIZE_X;
  h = (gint)(scale * (double) gdk_pixbuf_get_height (pixbuf));

  sw = clutter_actor_get_width  (stage) - w;
  sh = clutter_actor_get_height (stage) - h;

  x = rand () % sw;
  y = rand () % sh;
  
  group = clutter_group_new ();
  clutter_actor_set_position (group, x, y);
  clutter_actor_set_size (group, w + MARG, h + MARG);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), group);
  clutter_actor_show (group);
  
  rect = clutter_rectangle_new ();
  clutter_actor_set_position (rect, 0, 0);
  clutter_actor_set_size (rect, w + MARG, h + MARG);
  
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (rect),
                               &bckg_clr);
  clutter_actor_show (rect);

  shaddow = clutter_rectangle_new ();
  clutter_actor_set_position (shaddow, 2, 2);
  clutter_actor_set_size (shaddow, w + MARG, h + MARG);
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (shaddow), &shdw_clr);
  clutter_actor_show (shaddow);
  
  img = clutter_texture_new_from_pixbuf (pixbuf);
  clutter_actor_set_position (img, 2, 2);
  clutter_actor_set_size (img, w, h);
  clutter_actor_show (img);
  
  clutter_container_add (CLUTTER_CONTAINER (group),
			 shaddow, rect, img, NULL);

  zang = CLUTTER_INT_TO_FIXED (rand()%360);
  clutter_actor_rotate_zx (group, zang, (w + MARG)/2, (h + MARG)/2);
  
  dmx = clutter_dominatrix_new (group);

  return dmx;
}

static gboolean
is_supported_img (const gchar * name)
{
  GdkPixbufFormat * fmt = gdk_pixbuf_get_file_info (name, NULL, NULL);

  if (fmt)
    return (gdk_pixbuf_format_is_disabled (fmt) != TRUE);
  
  return FALSE;
}

static void
process_directory (const gchar * name,
		   ClutterActor * stage, ClutterActor * notice)
{
  GDir              * dir;
  const gchar       * fname;
  struct stat         sbuf;
  
  dir = g_dir_open (name, 0, NULL);

  if (!dir)
    return;

  g_chdir (name);
  
  while ((fname = g_dir_read_name (dir)))
    {
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);

      if (is_supported_img (fname))
	{
	  make_img_item (stage, fname);
	  clutter_actor_raise_top (notice);
	}
      
      if (g_stat (fname, &sbuf) > -1 && S_ISDIR (sbuf.st_mode))
	process_directory (fname, stage, notice);
    }

  g_chdir ("..");
  
  g_dir_close (dir);
}

static ClutterActor *
make_busy_notice (ClutterActor * stage)
{
  ClutterActor     * label;
  ClutterActor     * rect;
  ClutterActor     * group;
  ClutterColor       text_clr = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor       bckg_clr = { 0x5c, 0x54, 0x57, 0x9f };
  
  label = clutter_label_new_with_text ("Sans 54",
				       "Please wait, loading images ...");
  
  clutter_label_set_color (CLUTTER_LABEL (label), &text_clr);
  clutter_actor_set_position (label, 10, 10);
  clutter_actor_show (label);

  group = clutter_group_new ();
  clutter_actor_show (group);
  
  rect = clutter_rectangle_new ();
  clutter_actor_set_position (rect, 0, 0);
  clutter_actor_set_size (rect,
			  clutter_actor_get_width (label) + 20,
			  clutter_actor_get_height (label) + 20);
  
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (rect),
                               &bckg_clr);
  clutter_actor_show (rect);

  clutter_container_add (CLUTTER_CONTAINER (group), rect, label, NULL);
  
  return group;
}

struct timeout_cb_data
{
  ClutterActor * stage;
  ClutterActor * notice;
  const gchar  * name;
};

static gboolean
timeout_cb (gpointer data)
{
  struct timeout_cb_data * d = data;
  
  process_directory (d->name, d->stage, d->notice);

  clutter_group_remove (CLUTTER_GROUP (d->stage), d->notice);

  clutter_actor_show_all (d->stage);
  return FALSE;
}

int
main (int argc, char *argv[])
{
  ClutterActor      * stage, * notice;
  ClutterColor        stage_clr = { 0xed, 0xe8, 0xe1, 0xff };
  struct timeout_cb_data tcbd;
  
  if (argc != 2)
    {
      g_print ("\n    usage: %s image_directory\n\n", argv[0]);
      exit (1);
    }
  
  srand (time(NULL) + getpid());
  
  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_clr);

  g_object_set (stage, "fullscreen", TRUE, NULL);

  notice = make_busy_notice (stage);
  clutter_actor_set_position (notice,
			      (clutter_actor_get_width (stage) -
			       clutter_actor_get_width (notice))/2,
			      (clutter_actor_get_height (stage) -
			       clutter_actor_get_height (notice))/2);
  
  clutter_group_add (CLUTTER_GROUP(stage), notice);
  clutter_actor_show_all (stage);

  g_signal_connect (stage, "event", G_CALLBACK (keypress_cb), NULL);
  
  tcbd.stage  = stage;
  tcbd.notice = notice;
  tcbd.name   = argv[1];

  g_timeout_add (100, timeout_cb, &tcbd);
  
  clutter_main();

  return EXIT_SUCCESS;
}