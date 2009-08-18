/* Gpx Viewer 
 * Copyright (C) 2009-2009 Qball Cow <qball@sarine.nl>
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
using Gpx;
using GLib;
using Config;

static const string GT_LOG_DOMAIN="GPX_PLAYBACK";
static const string gt_unique_graph = Config.VERSION;
namespace Gpx
{
    public class Playback : GLib.Object
    {
        private Gpx.Track track =null;
        private uint timer = 0;
        private time_t progress = 0;
        private weak List <weak Gpx.Point> current;
        private Gpx.Point first = null;

        public signal void tick(Gpx.Point? point);

        public Playback(Gpx.Track track)
        {
            this.track = track;
            if(this.track.points != null)
            {
                this.first = this.track.points.first().data;
            }
        }
        public bool timer_callback()
        {
            this.progress+=30;
            GLib.debug("Tick\n");
            while(this.current != null && this.current.data.get_time() < this.progress) this.current = this.current.next;
            if(this.current == null) {
                this.timer = 0;
                tick(null);
                GLib.debug("stopping\n");
                return false;
            }
            /* make larger steps */
            tick(this.current.data);
            this.current = this.current.next;
            return true;
        }
        public void start()
        {
            this.stop();
            if(this.first != null)
            {
                this.progress  = this.first.get_time();
                GLib.debug("start playback\n");
                this.current = this.track.points.first();
                this.timer = GLib.Timeout.add(250, timer_callback); 
            }
        }
        public void pause()
        {
            if(this.current == null) return;
            if(this.timer > 0) {
                GLib.Source.remove(this.timer);
                timer = 0;
            }else{
                this.timer = GLib.Timeout.add(250, timer_callback); 
            }
        }
        public void stop()
        {
            if(this.timer > 0) {
                GLib.debug("stop playback\n");
                GLib.Source.remove(this.timer);
               timer = 0;
            }
            this.tick(null);
        }
    }
}
