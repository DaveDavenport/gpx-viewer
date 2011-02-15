/* champlain-0.8.vapi generated by vapigen, do not modify. */

[CCode (cprefix = "Champlain", lower_case_cprefix = "champlain_")]
namespace Champlain {
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class BaseMarker : Clutter.Group, Clutter.Scriptable, Clutter.Container {
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public BaseMarker ();
		public void animate_in ();
		public void animate_in_with_delay (uint delay);
		public void animate_out ();
		public void animate_out_with_delay (uint delay);
		public bool get_highlighted ();
		public double get_latitude ();
		public double get_longitude ();
		public void set_highlighted (bool value);
		public void set_position (double latitude, double longitude);
		public bool highlighted { get; set; }
		[NoAccessorMethod]
		public double latitude { get; set; }
		[NoAccessorMethod]
		public double longitude { get; set; }
	}
	[Compact]
	[CCode (copy_function = "champlain_bounding_box_copy", type_id = "CHAMPLAIN_TYPE_BOUNDING_BOX", cheader_filename = "champlain/champlain.h")]
	public class BoundingBox {
		public double bottom;
		public double left;
		public double right;
		public double top;
		[CCode (has_construct_function = false)]
		public BoundingBox ();
		public unowned Champlain.BoundingBox copy ();
		public void get_center (out double lat, out double lon);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class ErrorTileRenderer : Champlain.Renderer {
		[CCode (has_construct_function = false)]
		public ErrorTileRenderer (uint tile_size);
		public uint get_tile_size ();
		public void set_tile_size (uint size);
		public uint tile_size { get; set; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class FileCache : Champlain.TileCache {
		[CCode (has_construct_function = false)]
		protected FileCache ();
		[CCode (has_construct_function = false)]
		public FileCache.full (uint size_limit, string? cache_dir, Champlain.Renderer renderer);
		public unowned string get_cache_dir ();
		public uint get_size_limit ();
		public void purge ();
		public void purge_on_idle ();
		public void set_size_limit (uint size_limit);
		public string cache_dir { get; construct; }
		public uint size_limit { get; set construct; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class FileTileSource : Champlain.TileSource {
		[CCode (has_construct_function = false)]
		protected FileTileSource ();
		[CCode (has_construct_function = false)]
		public FileTileSource.full (string id, string name, string license, string license_uri, uint min_zoom, uint max_zoom, uint tile_size, Champlain.MapProjection projection, Champlain.Renderer renderer);
		public void load_map_data (string map_path);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class ImageRenderer : Champlain.Renderer {
		[CCode (has_construct_function = false)]
		public ImageRenderer ();
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class Layer : Clutter.Group, Clutter.Scriptable, Clutter.Container {
		[CCode (has_construct_function = false)]
		public Layer ();
		public void add_marker (Champlain.BaseMarker marker);
		public void animate_in_all_markers ();
		public void animate_out_all_markers ();
		public void hide ();
		public void hide_all_markers ();
		public void remove_marker (Champlain.BaseMarker marker);
		public void show ();
		public void show_all_markers ();
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class MapSource : GLib.InitiallyUnowned {
		[CCode (has_construct_function = false)]
		protected MapSource ();
		public virtual void fill_tile (Champlain.Tile tile);
		public uint get_column_count (uint zoom_level);
		public virtual unowned string get_id ();
		public double get_latitude (uint zoom_level, uint y);
		public virtual unowned string get_license ();
		public virtual unowned string get_license_uri ();
		public double get_longitude (uint zoom_level, uint x);
		public virtual uint get_max_zoom_level ();
		public double get_meters_per_pixel (uint zoom_level, double latitude, double longitude);
		public virtual uint get_min_zoom_level ();
		public virtual unowned string get_name ();
		public unowned Champlain.MapSource get_next_source ();
		public virtual Champlain.MapProjection get_projection ();
		public unowned Champlain.Renderer get_renderer ();
		public uint get_row_count (uint zoom_level);
		public virtual uint get_tile_size ();
		public uint get_x (uint zoom_level, double longitude);
		public uint get_y (uint zoom_level, double latitude);
		public void set_next_source (Champlain.MapSource next_source);
		public void set_renderer (Champlain.Renderer renderer);
		public Champlain.MapSource next_source { get; set; }
		public Champlain.Renderer renderer { get; set; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class MapSourceChain : Champlain.MapSource {
		[CCode (has_construct_function = false)]
		public MapSourceChain ();
		public void pop ();
		public void push (Champlain.MapSource map_source);
	}
	[Compact]
	[CCode (copy_function = "champlain_map_source_desc_copy", type_id = "CHAMPLAIN_TYPE_MAP_SOURCE_DESC", cheader_filename = "champlain/champlain.h")]
	public class MapSourceDesc {
		public weak Champlain.MapSourceConstructor constructor;
		public void* data;
		public weak string id;
		public weak string license;
		public weak string license_uri;
		public int max_zoom_level;
		public int min_zoom_level;
		public weak string name;
		public Champlain.MapProjection projection;
		public weak string uri_format;
		[CCode (has_construct_function = false)]
		public MapSourceDesc ();
		public unowned Champlain.MapSourceDesc copy ();
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class MapSourceFactory : GLib.Object {
		[CCode (has_construct_function = false)]
		protected MapSourceFactory ();
		public unowned Champlain.MapSource create (string id);
		public unowned Champlain.MapSource create_cached_source (string id);
		public unowned Champlain.MapSource create_error_source (uint tile_size);
		public static unowned Champlain.MapSourceFactory dup_default ();
		public GLib.SList<weak Champlain.MapSourceDesc> dup_list ();
		public bool register (Champlain.MapSourceDesc desc, Champlain.MapSourceConstructor constructor, void* data);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class Marker : Champlain.BaseMarker, Clutter.Scriptable, Clutter.Container {
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public Marker ();
		[NoWrapper]
		public virtual void draw_marker ();
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public Marker.from_file (string filename) throws GLib.Error;
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public Marker.full (string text, Clutter.Actor actor);
		public Pango.Alignment get_alignment ();
		public Clutter.Color get_color ();
		public bool get_draw_background ();
		public Pango.EllipsizeMode get_ellipsize ();
		public unowned string get_font_name ();
		public static Clutter.Color get_highlight_color ();
		public static Clutter.Color get_highlight_text_color ();
		public unowned Clutter.Actor get_image ();
		public bool get_single_line_mode ();
		public unowned string get_text ();
		public Clutter.Color get_text_color ();
		public bool get_use_markup ();
		public bool get_wrap ();
		public Pango.WrapMode get_wrap_mode ();
		public void queue_redraw ();
		public void set_alignment (Pango.Alignment alignment);
		public void set_attributes (Pango.AttrList list);
		public void set_color (Clutter.Color color);
		public void set_draw_background (bool background);
		public void set_ellipsize (Pango.EllipsizeMode mode);
		public void set_font_name (string font_name);
		public static void set_highlight_color (Clutter.Color color);
		public static void set_highlight_text_color (Clutter.Color color);
		public void set_image (Clutter.Actor image);
		public void set_single_line_mode (bool mode);
		public void set_text (string text);
		public void set_text_color (Clutter.Color color);
		public void set_use_markup (bool use_markup);
		public void set_wrap (bool wrap);
		public void set_wrap_mode (Pango.WrapMode wrap_mode);
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public Marker.with_image (Clutter.Actor actor);
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public Marker.with_text (string text, string? font, Clutter.Color? text_color, Clutter.Color? marker_color);
		public Pango.Alignment alignment { get; set; }
		public Clutter.Color color { get; set; }
		public bool draw_background { get; set; }
		public Pango.EllipsizeMode ellipsize { get; set; }
		public string font_name { get; set; }
		public Clutter.Actor image { get; set; }
		public bool single_line_mode { get; set; }
		public string text { get; set; }
		public Clutter.Color text_color { get; set; }
		public bool use_markup { get; set; }
		public bool wrap { get; set; }
		public Pango.WrapMode wrap_mode { get; set; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class MemoryCache : Champlain.TileCache {
		[CCode (has_construct_function = false)]
		protected MemoryCache ();
		public void clean ();
		[CCode (has_construct_function = false)]
		public MemoryCache.full (uint size_limit, Champlain.Renderer renderer);
		public uint get_size_limit ();
		public void set_size_limit (uint size_limit);
		public uint size_limit { get; set construct; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class NetworkBboxTileSource : Champlain.TileSource {
		[CCode (has_construct_function = false)]
		protected NetworkBboxTileSource ();
		[CCode (has_construct_function = false)]
		public NetworkBboxTileSource.full (string id, string name, string license, string license_uri, uint min_zoom, uint max_zoom, uint tile_size, Champlain.MapProjection projection, Champlain.Renderer renderer);
		public unowned string get_api_uri ();
		public void load_map_data (double bound_left, double bound_bottom, double bound_right, double bound_top);
		public void set_api_uri (string api_uri);
		public string api_uri { get; set; }
		[NoAccessorMethod]
		public string proxy_uri { owned get; set; }
		[NoAccessorMethod]
		public Champlain.State state { get; set; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class NetworkTileSource : Champlain.TileSource {
		[CCode (has_construct_function = false)]
		protected NetworkTileSource ();
		[CCode (has_construct_function = false)]
		public NetworkTileSource.full (string id, string name, string license, string license_uri, uint min_zoom, uint max_zoom, uint tile_size, Champlain.MapProjection projection, string uri_format, Champlain.Renderer renderer);
		public bool get_offline ();
		public unowned string get_proxy_uri ();
		public unowned string get_uri_format ();
		public void set_offline (bool offline);
		public void set_proxy_uri (string proxy_uri);
		public void set_uri_format (string uri_format);
		public bool offline { get; set; }
		public string proxy_uri { get; set; }
		public string uri_format { get; set construct; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class NullTileSource : Champlain.TileSource {
		[CCode (has_construct_function = false)]
		protected NullTileSource ();
		[CCode (has_construct_function = false)]
		public NullTileSource.full (Champlain.Renderer renderer);
	}
	[Compact]
	[CCode (copy_function = "champlain_point_copy", type_id = "CHAMPLAIN_TYPE_POINT", cheader_filename = "champlain/champlain.h")]
	public class Point {
		public double lat;
		public double lon;
		[CCode (has_construct_function = false)]
		public Point (double lat, double lon);
		public unowned Champlain.Point copy ();
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class Polygon : Clutter.Group, Clutter.Scriptable, Clutter.Container {
		[CCode (has_construct_function = false)]
		public Polygon ();
		public unowned Champlain.Point append_point (double lat, double lon);
		public void clear_points ();
		public void draw_polygon (Champlain.MapSource map_source, uint zoom_level, float width, float height, float shift_x, float shift_y);
		public bool get_fill ();
		public Clutter.Color get_fill_color ();
		public bool get_mark_points ();
		public unowned GLib.List<weak Champlain.Point> get_points ();
		public bool get_stroke ();
		public Clutter.Color get_stroke_color ();
		public double get_stroke_width ();
		public void hide ();
		public unowned Champlain.Point insert_point (double lat, double lon, int pos);
		public void remove_point (Champlain.Point point);
		public void set_fill (bool value);
		public void set_fill_color (Clutter.Color color);
		public void set_mark_points (bool value);
		public void set_stroke (bool value);
		public void set_stroke_color (Clutter.Color color);
		public void set_stroke_width (double value);
		public void show ();
		[NoAccessorMethod]
		public bool closed_path { get; set; }
		public bool fill { get; set; }
		public Clutter.Color fill_color { get; set; }
		public bool mark_points { get; set; }
		public bool stroke { get; set; }
		public Clutter.Color stroke_color { get; set; }
		public double stroke_width { get; set; }
		[NoAccessorMethod]
		public bool visible { get; set; }
	}
	[Compact]
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class RenderCallbackData {
		public weak string data;
		public bool error;
		public uint size;
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class Renderer : GLib.InitiallyUnowned {
		[CCode (has_construct_function = false)]
		protected Renderer ();
		public virtual void render (Champlain.Tile tile);
		public virtual void set_data (string data, uint size);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class SelectionLayer : Champlain.Layer, Clutter.Scriptable, Clutter.Container {
		[CCode (type = "ChamplainLayer*", has_construct_function = false)]
		public SelectionLayer ();
		public uint count_selected_markers ();
		public unowned Champlain.BaseMarker get_selected ();
		public unowned GLib.List<weak Champlain.BaseMarker> get_selected_markers ();
		public Champlain.SelectionMode get_selection_mode ();
		public bool marker_is_selected (Champlain.BaseMarker marker);
		public void select (Champlain.BaseMarker marker);
		public void select_all ();
		public void set_selection_mode (Champlain.SelectionMode mode);
		public void unselect (Champlain.BaseMarker marker);
		public void unselect_all ();
		public Champlain.SelectionMode selection_mode { get; set; }
		public virtual signal void changed ();
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class Tile : Clutter.Group, Clutter.Scriptable, Clutter.Container {
		[CCode (has_construct_function = false)]
		public Tile ();
		public void display_content ();
		[CCode (has_construct_function = false)]
		public Tile.full (int x, int y, uint size, int zoom_level);
		public unowned Clutter.Actor get_content ();
		public unowned string get_etag ();
		public bool get_fade_in ();
		public GLib.TimeVal get_modified_time ();
		public uint get_size ();
		public Champlain.State get_state ();
		public int get_x ();
		public int get_y ();
		public int get_zoom_level ();
		public void set_content (Clutter.Actor actor);
		public void set_etag (string etag);
		public void set_fade_in (bool fade_in);
		public void set_modified_time (GLib.TimeVal time);
		public void set_size (uint size);
		public void set_state (Champlain.State state);
		public void set_x (int x);
		public void set_y (int y);
		public void set_zoom_level (int zoom_level);
		public Clutter.Actor content { get; set; }
		public string etag { get; set; }
		public bool fade_in { get; set; }
		public uint size { get; set; }
		public Champlain.State state { get; set; }
		public int x { get; set; }
		public int y { get; set; }
		public int zoom_level { get; set; }
		public virtual signal void render_complete (void* p0);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class TileCache : Champlain.MapSource {
		[CCode (has_construct_function = false)]
		protected TileCache ();
		public virtual void on_tile_filled (Champlain.Tile tile);
		public virtual void refresh_tile_time (Champlain.Tile tile);
		public virtual void store_tile (Champlain.Tile tile, string contents, size_t size);
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class TileSource : Champlain.MapSource {
		[CCode (has_construct_function = false)]
		protected TileSource ();
		public unowned Champlain.TileCache get_cache ();
		public void set_cache (Champlain.TileCache cache);
		public void set_id (string id);
		public void set_license (string license);
		public void set_license_uri (string license_uri);
		public void set_max_zoom_level (uint zoom_level);
		public void set_min_zoom_level (uint zoom_level);
		public void set_name (string name);
		public void set_projection (Champlain.MapProjection projection);
		public void set_tile_size (uint tile_size);
		public Champlain.TileCache cache { get; set; }
		[NoAccessorMethod]
		public string id { owned get; set construct; }
		[NoAccessorMethod]
		public string license { owned get; set construct; }
		[NoAccessorMethod]
		public string license_uri { owned get; set construct; }
		[NoAccessorMethod]
		public uint max_zoom_level { get; set construct; }
		[NoAccessorMethod]
		public uint min_zoom_level { get; set construct; }
		[NoAccessorMethod]
		public string name { owned get; set construct; }
		[NoAccessorMethod]
		public Champlain.MapProjection projection { get; set construct; }
		[NoAccessorMethod]
		public uint tile_size { get; set construct; }
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public class View : Clutter.Group, Clutter.Scriptable, Clutter.Container {
		[CCode (type = "ClutterActor*", has_construct_function = false)]
		public View ();
		public void add_layer (Champlain.Layer layer);
		public void add_polygon (Champlain.Polygon polygon);
		public void center_on (double latitude, double longitude);
		public void ensure_markers_visible ([CCode (array_length = false)] Champlain.BaseMarker[] markers, bool animate);
		public void ensure_visible (double lat1, double lon1, double lat2, double lon2, bool animate);
		public bool get_coords_at (uint x, uint y, out double lat, out double lon);
		public bool get_coords_from_event (Clutter.Event event, out double lat, out double lon);
		public double get_decel_rate ();
		public bool get_keep_center_on_resize ();
		public unowned string get_license_text ();
		public unowned Champlain.MapSource get_map_source ();
		public uint get_max_scale_width ();
		public int get_max_zoom_level ();
		public int get_min_zoom_level ();
		public Champlain.Unit get_scale_unit ();
		public Champlain.ScrollMode get_scroll_mode ();
		public bool get_show_license ();
		public bool get_show_scale ();
		public int get_zoom_level ();
		public bool get_zoom_on_double_click ();
		public void go_to (double latitude, double longitude);
		public void reload_tiles ();
		public void remove_layer (Champlain.Layer layer);
		public void remove_polygon (Champlain.Polygon polygon);
		public void set_decel_rate (double rate);
		public void set_keep_center_on_resize (bool value);
		public void set_license_text (string text);
		public void set_map_source (Champlain.MapSource map_source);
		public void set_max_scale_width (uint value);
		public void set_max_zoom_level (int zoom_level);
		public void set_min_zoom_level (int zoom_level);
		public void set_scale_unit (Champlain.Unit unit);
		public void set_scroll_mode (Champlain.ScrollMode mode);
		public void set_show_license (bool value);
		public void set_show_scale (bool value);
		public void set_size (uint width, uint height);
		public void set_zoom_level (int zoom_level);
		public void set_zoom_on_double_click (bool value);
		public void stop_go_to ();
		public void zoom_in ();
		public void zoom_out ();
		public double decel_rate { get; set; }
		public bool keep_center_on_resize { get; set; }
		[NoAccessorMethod]
		public double latitude { get; set; }
		public string license_text { get; set; }
		[NoAccessorMethod]
		public double longitude { get; set; }
		public Champlain.MapSource map_source { get; set; }
		public uint max_scale_width { get; set; }
		public int max_zoom_level { get; set; }
		public int min_zoom_level { get; set; }
		public Champlain.Unit scale_unit { get; set; }
		public Champlain.ScrollMode scroll_mode { get; set; }
		public bool show_license { get; set; }
		public bool show_scale { get; set; }
		[NoAccessorMethod]
		public Champlain.State state { get; }
		public int zoom_level { get; set; }
		public bool zoom_on_double_click { get; set; }
		public virtual signal void animation_completed ();
	}
	[CCode (cprefix = "CHAMPLAIN_MAP_PROJECTION_", has_type_id = false, cheader_filename = "champlain/champlain.h")]
	public enum MapProjection {
		MERCATOR
	}
	[CCode (cprefix = "CHAMPLAIN_SCROLL_MODE_", has_type_id = false, cheader_filename = "champlain/champlain.h")]
	public enum ScrollMode {
		PUSH,
		KINETIC
	}
	[CCode (cprefix = "CHAMPLAIN_SELECTION_", has_type_id = false, cheader_filename = "champlain/champlain.h")]
	public enum SelectionMode {
		NONE,
		SINGLE,
		MULTIPLE
	}
	[CCode (cprefix = "CHAMPLAIN_STATE_", has_type_id = false, cheader_filename = "champlain/champlain.h")]
	public enum State {
		NONE,
		LOADING,
		LOADED,
		DONE
	}
	[CCode (cprefix = "CHAMPLAIN_UNIT_", has_type_id = false, cheader_filename = "champlain/champlain.h")]
	public enum Unit {
		KM,
		MILES
	}
	[CCode (cheader_filename = "champlain/champlain.h")]
	public delegate unowned Champlain.MapSource MapSourceConstructor (Champlain.MapSourceDesc desc);
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const int MAJOR_VERSION;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_MEMPHIS_LOCAL;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_MEMPHIS_NETWORK;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_MFF_RELIEF;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OAM;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OSM_CYCLE_MAP;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OSM_MAPNIK;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OSM_MAPQUEST;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OSM_OSMARENDER;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string MAP_SOURCE_OSM_TRANSPORT_MAP;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const int MICRO_VERSION;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const int MINOR_VERSION;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const int VERSION_HEX;
	[CCode (cheader_filename = "champlain/champlain.h")]
	public const string VERSION_S;
}
