
public int sort_tracks(Gpx.Track a, Gpx.Track b)
{
    Gpx.Point ap = a.points.first().data;
    Gpx.Point bp = b.points.first().data;
    if(ap == null && bp == null) return 0;
    if(ap == null) return 1;
    if(bp == null) return -1;

    return (int)(ap.get_time()-bp.get_time());
}
public static int main(string[] argv)
{
    List<Gpx.File> files = null;
    List<Gpx.Track> tracks = null;
    int i=1;
    for(;argv != null && argv[i] != null;i++) {
        GLib.File f = GLib.File.new_for_commandline_arg(argv[i]);
        var e = new Gpx.File(f);
        files.append(e);
        foreach(var track in e.tracks) {
            tracks.append(track);
        }
    }
    tracks.sort((CompareFunc)sort_tracks);

    double total_distance = 0;
    foreach(var track in tracks) {
        if(track.total_distance > 1)
        {
            if(track.get_track_average() < 50 && track.get_track_average() > 5) {
                time_t a;
                stdout.printf("%s: %9.2f %9.2f %9.2f\n", track.points.first().data.time, track.total_distance, track.get_track_average(), 
                        track.calculate_moving_average(track.points.first().data, track.points.last().data,out a));
                total_distance += track.total_distance;
            }

        }
    }
    stdout.printf("Total distance: %f\n", total_distance);

    return 0;
}
