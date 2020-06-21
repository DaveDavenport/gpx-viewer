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

namespace Gpx.Viewer.Misc {
	[CCode (cname = "SpeedFormat", cprefix = "")]
	public enum SpeedFormat {
		DISTANCE,
		SPEED,
		ELEVATION,
		ACCEL,
		NA
	}

	/* TODO get correct values */
	const double KM_IN_MILE = 0.621371192;
	const double M_IN_FEET = 0.3048;

	public string convert (double speed, SpeedFormat format)
	{
		string retv = null;
		/* TODO: Make config option? */
		bool do_miles = false;
		switch(format)
		{
		case DISTANCE:
			if(do_miles)
				retv = "% .2f %s".printf (speed/KM_IN_MILE, _("Miles"));
			else
				retv = "% .2f %s".printf (speed, _("km"));
			break;
		case SPEED:
			if(do_miles)
				retv = "% .2f %s".printf (speed/KM_IN_MILE, _("Miles/h"));
			else
				retv = "% .2f %s".printf (speed, _("km/h"));
			break;
		case ELEVATION:
			if(do_miles)
				/* TODO: */
				retv = "% .2f %s".printf (speed/M_IN_FEET, _("ft"));
			else
				retv = "% .2f %s".printf (speed, _("m"));
			break;
		case ACCEL:
			if(do_miles)
				/* TODO: */
				retv = "% .2f %s".printf (speed/M_IN_FEET, _("ft/s²"));
			else
				retv = "% .2f %s".printf (speed, _("m/s²"));
			break;
		case NA:
		default:
			retv = _("n/a");
			break;
		}
		return retv;
	}
}
