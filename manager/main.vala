using Gtk;

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
	List<Tuple<double?,double?>?> points = null;
	List<Tuple<double?,double?>?> points_mov = null;
	List<Tuple<double?,string>?> xaxis = null;
	List<Tuple<double?,string>?> yaxis = null;
    int i=1;

	Gtk.init(ref argv);

    for(;argv != null && argv[i] != null;i++) {
        GLib.File f = GLib.File.new_for_commandline_arg(argv[i]);
        var e = new Gpx.File(f);
        files.append(e);
        foreach(var track in e.tracks) {
            tracks.prepend(track);
        }
    }
    tracks.sort((CompareFunc)sort_tracks);

    double total_distance = 0;
	time_t minx = int.MAX;
	time_t maxx = 0;
	double miny = 0;//double.MAX;
	double maxy = 0;
    foreach(var track in tracks) {
        if(track.total_distance > 1)
        {
            if(track.get_track_average() < 50 && track.get_track_average() > 10) {
                time_t a;
                stdout.printf("%s: %9.2f %9.2f %9.2f\n", track.points.first().data.time, track.total_distance, track.get_track_average(), 
                        track.calculate_moving_average(track.points.first().data, track.points.last().data,out a));
                total_distance += track.total_distance;
				Tuple<double?,double?> p = new Tuple<double?,double?>();
				p.x = (double)   track.points.first().data.get_time();
				p.y = (double) total_distance;//track.get_track_average();
				maxy = (p.y > maxy)?p.y:maxy;
				miny = (p.y < miny)?p.y:miny;
				points.prepend((owned)p);
				p = new Tuple<double?,double?>();

				p.x = (double)   track.points.first().data.get_time();
				p.y = (double) track.calculate_moving_average(track.points.first().data, track.points.last().data,out a);
				maxy = (p.y > maxy)?p.y:maxy;
				miny = (p.y < miny)?p.y:miny;

				minx = (track.points.first().data.get_time() < minx)? track.points.first().data.get_time():minx;
				maxx = (track.points.first().data.get_time() > maxx)? track.points.first().data.get_time():maxx;
				


				points_mov.prepend((owned)p);
            }

        }
    }
	stdout.printf("%u %u\n", (uint)minx, (uint)maxx);
    Time e =  Time.gm(minx);  
    time_t a = minx;
	do{
		var t = new Tuple<double?,string?>();
		t.x = (double)a;
		t.n = "%02i-%i".printf(e.month+1, e.year+1900);
		stdout.printf("%s\n",t.n);
		xaxis.append(t);

    	e.month++;
		if(e.month > 11){
			e.year++;
			e.month =0;
		}
		a = e.mktime(); 
	}while(a < maxx);

	for(double y = miny; y < maxy; y+= (maxy-miny)/12.0){
		var t = new Tuple<double?,string?>();
		t.x = y;
		t.n = "%.2fkm".printf(y);
		stdout.printf("%s\n", t.n);
		yaxis.append(t);
	}

	points.reverse();
	points_mov.reverse();
    stdout.printf("Total distance: %f\n", total_distance);

	var win = new Gtk.Window(Gtk.WindowType.TOPLEVEL);
	
	var g = new Graph();
	win.resize(600,300);
	win.add(g);
    win.show_all();

	g.init();
	//g.set_points(points);
    //g.set_points(points_mov);

    /* Create a trent line */
    var t_p = get_trent(points,0);
    g.set_points(t_p);

    //var t_p2 = get_trent(points_mov,0);
    //g.set_points(t_p2);
	g.Xaxis = (owned)xaxis;
	g.Yaxis = (owned)yaxis;
	g.draw();

    win.destroy.connect((source) => {
        Gtk.main_quit();
    });
	Gtk.main();
    return 0;
}

static List<Tuple?> get_trent(List<Tuple?> points, int window)
{
    List<Tuple<double?,double?>?> t_p = null;
    for(weak List<Tuple<double?,double?>?> q = points.first(); q != null; q = q.next)
    {
        int o=0;
        weak List<Tuple<double?,double?>?> iter = q.prev;
        Tuple<double?,double?> p = new Tuple<double?,double?>();
        p.x = (q.data.x);
        p.y = q.data.y;
        o++;
        for(int j = 0;  iter != null && j <window;j++){
            double x = iter.data.y;
            p.y += x;
            o++;
            iter = iter.prev;
        }
        iter = q.next;
        for(int j = 0;  iter != null && j <window;j++){
            double x = iter.data.y;
            p.y += x;
            o++;
            iter = iter.next;
        }
        p.y /= o;
        t_p.append(p);
    }
    return t_p;
}
