namespace Gpx
{
	namespace Viewer
	{
		namespace Misc
		{
            [CCode (cprefix="",cheader_filename="gpx-viewer.h")]
			public enum SpeedFormat {
				DISTANCE,
					SPEED,
					ELEVATION,
					ACCEL
			}
			public string convert(double speed, SpeedFormat format);
		}
	}
}
