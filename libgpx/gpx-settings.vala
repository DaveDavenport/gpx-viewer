/* Gpx Viewer
 * Copyright (C) 2009-2015 Qball Cow <qball@sarine.nl>
 * Project homepage: http://blog.sarine.nl/

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

namespace Gpx
{
    namespace Viewer
    {
        public class Settings : GLib.Object
        {
            /* The log domain */
            const string LOG_DOMAIN = "Gpx.Viewer.Settings";

            /* The keyfile holding the values */
            private GLib.KeyFile keyfile = new GLib.KeyFile();

            public static Type enum_type;
            private enum __enum {
                FOO,
            }
            static construct {
                enum_type = typeof(__enum).parent();
            }
            /* Create preferences object */
            public Settings()
            {
                GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Creating preferences");

                /* Create location of file  */
                string path = GLib.Path.build_filename(
                        GLib.Environment.get_user_config_dir(),
                        "gpx-viewer",
                        "config.ini");
                GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Loading from: %s", path);
                /* Load old config file */
                try {
                    keyfile.load_from_file(path, KeyFileFlags.NONE);
                }catch (Error e) {
                    GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_WARNING,
                        "Failed to load config file: %s",
                        e.message);
                }
            }
            ~Settings()
            {
                GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Destroying preferences");
                /* Save the preferences */
                size_t size;
                /* according to doc, never trows an error, so do not catch */
                string ini_data = keyfile.to_data(out size);
                /* Create location of file  */
                string path = GLib.Path.build_filename(
                        GLib.Environment.get_user_config_dir(),
                        "gpx-viewer",
                        "config.ini");
                GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Saving to: %s", path);
                try {
                    GLib.FileUtils.set_contents(path, ini_data, (ssize_t)size);
                } catch (Error e) {
                    GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_WARNING,
                        "Failed to store config file: %s",
                        e.message);
                }
            }


            /* Work around */
            static void object_value_changed(GLib.Object object, ParamSpec sp,Settings pref)
            {
                GLib.Type t = object.get_type();
                Value value = Value(sp.value_type) ;
                object.get_property(sp.name, ref value);
                if(sp.value_type == typeof(int)) {
                    pref.keyfile.set_integer(t.name(), sp.name, value.get_int());
                }else if (sp.value_type.is_a(Settings.enum_type)) {
                    pref.keyfile.set_integer(t.name(), sp.name, (int)value.get_enum());
                }else if (sp.value_type.is_a(typeof(bool))) {
                    pref.keyfile.set_boolean(t.name(), sp.name, value.get_boolean());
                }else if (sp.value_type.is_a(typeof(string))) {
                    pref.keyfile.set_string(t.name(), sp.name, value.get_string());
                }
            }

            /**
             * Atomatically save and restore (on calling this function)
             * A GLib.Object property.
             */
            public void add_object_property(GLib.Object wobject,string property)
            {
                ParamSpec sp = wobject.get_class().find_property(property);
                if(sp == null) {
                    GLib.log(LOG_DOMAIN,
                        GLib.LogLevelFlags.LEVEL_WARNING,
                        "object has not property: %s",
                        property);
                    return;
                }
                /* (Try) to set previous stored value */
                if(sp.value_type == typeof(int) || sp.value_type.is_a(enum_type)) {
                    int val;
                    try {
                        val = (int) keyfile.get_integer (wobject.get_type().name(),sp.name);
                        wobject.set(sp.name, val);
                    }catch (Error e) {
                    }
                }else if (sp.value_type.is_a(typeof(bool))) {
                    bool val;
                    try {
                        val = keyfile.get_boolean(wobject.get_type().name(),sp.name);
                        wobject.set(sp.name, val);
                    }catch (Error e) {
                    }
                }else if (sp.value_type.is_a(typeof(string))) {
                    string val;
                    try {
                        val = keyfile.get_string(wobject.get_type().name(),sp.name);
                        wobject.set(sp.name, val);
                    }catch (Error e) {
                    }
                }
                /* Connect changed signal */
                GLib.Signal.connect_object(wobject, "notify::"+property, (GLib.Callback) object_value_changed, this,ConnectFlags.AFTER);
            }

            /**
             * manual config system
             */
            public int get_integer(string class, string name, int def)
            {
                int val;
                try{
                    val = keyfile.get_integer(class, name);
                }catch (Error e) {
                    val = def;
                }
                return val;
            }
            public string get_string(string class, string name, string? def)
            {
                string val;
                try{
                    val = keyfile.get_string(class, name);
                }catch (Error e) {
                    val = def;
                }
                return val;
            }
            public void set_integer(string class, string name, int val)
            {
                keyfile.set_integer(class, name, val);
            }
            public void set_string(string class, string name, string val)
            {
                keyfile.set_string(class, name, val);
            }
            public double get_double(string class, string name, double def)
            {
                double val;
                try{
                    val = keyfile.get_double(class, name);
                }catch (Error e) {
                    val = def;
                }
                return val;
            }
            public bool get_boolean (string class, string name, bool def)
            {
                bool val;
                try{
                    val = keyfile.get_boolean(class, name);
                }catch (Error e) {
                    val = def;
                }
                return val;
            }
        }
    }
}
