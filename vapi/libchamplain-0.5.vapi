using GLib;
using Clutter;
namespace Champlain
{
    public enum ScrollMode {
        PUSH,
        KINETIC
    }
    public class BaseMarker : Clutter.Actor
    {
        public void set_position(double lat_dec, double lon_dec);
    }
    public class Marker : BaseMarker
    {
		[CCode (has_construct_function = false)]
        public Marker();
        public Marker.with_text(string *text, string *font, Clutter.Color? text_color, Clutter.Color? marker_color);
        public void set_text(string text);
        public void set_use_markup(bool use_markup);
        public void set_color(ref Clutter.Color color);
        public void set_text_color(ref Clutter.Color color);
    }
    public class MapSource : Object
    {
    }
    public class Layer : Gtk.Bin, Clutter.Container
    {
		[CCode (has_construct_function = false)]
        public Layer();
        public void show();
        public void hide();
    }
    [Compact]
    [Immutable]
    [CCode (free_function="champlain_map_source_desc_free",
            copy_function="champlain_map_source_desc_copy")]
    public class MapSourceDesc
    {
        public string id;
        public string name;
        public string license;
        public string license_uri;
        public int   max_zoom_level;
        public int   min_zoom_level;

        public string uri_format;
        public void    *data;
    }
    class MapSourceFactory : GLib.Object
    {
        [CCode (cname="champlain_map_source_factory_dup_default",has_construct_function = false)]
        public MapSourceFactory.dup_default();
        public SList<weak MapSourceDesc?> dup_list();
        public unowned MapSource create(string id);
        public unowned MapSource create_cached_source(string id);

    }
    [CCode (cheader_filename="champlain/champlain.h")]
        public class View : Gtk.Bin
    {
        public ScrollMode scroll_mode {get; set;}
        public bool show_scale {get; set;}
        public int zoom_level {get; set;}
        public int max_zoom_level {get;set;}
        public int min_zoom_level {get;set;}
        public View ();
        public void set_map_source(MapSource source);
        public void add_layer(Layer layer);

    }
}
