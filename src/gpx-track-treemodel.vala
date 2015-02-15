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

using Gtk;
using GLib;
using Xml;
using Config;

namespace Gpx
{
    public class TrackTreeModel :  GLib.Object, Gtk.TreeModel
    {
	/* Available columns */
        public enum Column {
            TIME,
            DISTANCE,
            ELEVATION,
            SPEED,
            NUM_COLUMNS
        }
        private GLib.Type[] ColumnType = {
            typeof(string),
            typeof(double),
            typeof(double),
            typeof(double)
        };

        /* Unique id for iter stamp */
        private int stamp = 42;
        private Gpx.Track track = null;
        public TrackTreeModel (Gpx.Track track)
        {
            this.track = track;
        }

        public GLib.Type get_column_type ( int index_)
        {
            return ColumnType[index_];
        }

        public bool get_iter (out Gtk.TreeIter iter, Gtk.TreePath path)
        {
            iter = Gtk.TreeIter();
            int depth = path.get_depth ();
            assert (depth == 1);
            int n = path.get_indices ()[0];
            assert (n >= 0 && n < (int)this.track.points.length () );
            Point p = this.track.points.nth_data(n);
            iter.stamp = this.stamp;
            iter.user_data = p;
            iter.user_data2 = null;
            iter.user_data3 = null;
            return true;
        }

        public int get_n_columns()
        {
            return Column.NUM_COLUMNS;
        }

        /**
         * Given an iter, returns a path
         */
        public Gtk.TreePath? get_path (Gtk.TreeIter iter){
            assert (iter.user_data != null);
            Point p = iter.user_data as Gpx.Point;

            TreePath path = new TreePath();
            int pos = this.track.points.index (p);
            assert (pos != -1);
            path.append_index (pos);
            return path;
        }
        public void get_value (Gtk.TreeIter iter, int column, out GLib.Value value)
        {
            assert (iter.user_data != null);
            Gpx.Point p = iter.user_data as Gpx.Point;
            value = Value (get_column_type(column));
            switch (column) {
                case Column.TIME:
                    Time t  = Time.local(p.get_time());
                    value.set_string(t.format("%D - %X"));
                    break;
                case Column.DISTANCE:
                    value.set_double(p.distance);
                    break;
                case Column.ELEVATION:
                    value.set_double(p.elevation);
                    break;
                case Column.SPEED:
                    value.set_double(p.speed);
                    break;
                default:
                    break;
            }
        }
        /**
         * Should never be reached since iter_has_child is never true
         */
        public bool iter_children (out Gtk.TreeIter iter, Gtk.TreeIter? parent){
            iter = Gtk.TreeIter();
            return false;
        }

        /**
         * always false since this is a list
         */
        public bool iter_has_child (Gtk.TreeIter iter){
            return false;
        }

        /**
         * Number of children for an iter: treated only the special case,
         * where iter == null, returns the number of elements in the list
         */
        public int iter_n_children (Gtk.TreeIter? iter){
            if (iter == null) {
                uint n_children = this.track.points.length ();
                return (int)n_children;
            } else {
                return 0;
            }
        }
        public Gtk.TreeModelFlags get_flags (){
                Gtk.TreeModelFlags flags = TreeModelFlags.LIST_ONLY | TreeModelFlags.ITERS_PERSIST;
                return flags;
        }
        /**
         * Given an iter, modify it to point to the next file
         */
        public bool iter_next (ref Gtk.TreeIter iter){
            assert (iter.user_data != null);
            Gpx.Point p = iter.user_data as Gpx.Point;
            weak List node = this.track.points.find (p);
            if ((node != null) && (node.next != null))
            {
                iter.stamp = this.stamp;
                iter.user_data = node.next.data;
                return true;
            }
            return false;
        }

        public bool iter_nth_child (out Gtk.TreeIter iter, Gtk.TreeIter? parent, int n)
        {
            iter = Gtk.TreeIter();
            if (parent == null)
                return false;
            if (n<0 || n>= this.track.points.length ())
                return false;
            iter.stamp = this.stamp;
            iter.user_data = this.track.points.nth_data (n);
            return true;
        }

        /**
         * again, not implemented since this is not a tree
         */
        public bool iter_parent (out Gtk.TreeIter iter, Gtk.TreeIter child){
            iter = Gtk.TreeIter();
            return false;
        }

        public void ref_node (Gtk.TreeIter iter){

        }

        public void unref_node (Gtk.TreeIter iter){

        }
    }
}
