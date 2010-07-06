
public class Tuple<G,Y> 
{
 	public double x;
	public double y;
    public string n;
}

public class Graph : Gtk.EventBox
{
	private Cairo.Surface surf = null;

	public Graph()
	{
		/* make the event box paintable and give it an own window to paint on */
		this.app_paintable = true;
		this.visible_window = true;
		this.size_allocate.connect(size_allocate_cb);
	}
	private void size_allocate_cb(Gdk.Rectangle alloc)
	{
		/* Invalidate the previous plot, so it is redrawn */
		this.surf = null;
		if(this.window != null) {
			init();
			draw();
		}
	}

    /* Do the actual drawing */
	override bool expose_event(Gdk.EventExpose event)
	{                          
		var ctx = Gdk.cairo_create(this.window);
		if(surf != null)
		{
			ctx.set_source_surface(this.surf, 0, 0);
//			Gdk.cairo_region(ctx, event.region);
  //  		ctx.clip();
			ctx.paint();
		}
		else
		{
        	ctx.set_source_rgb(1.0f,1.0f,1.0f);
			ctx.paint();
		}
		return false;
	}
	private List<weak List<Tuple<double?,double?>?>> sets = null;
	public List<Tuple<double?,string>> Xaxis = null;
	public List<Tuple<double?,string>> Yaxis = null;
	private		double minx=-1;
	private	   double maxx=0;
	private   	double xscale = 0;
	private   	double miny=0;
	private	double maxy=0;
	private   	double yscale = 0;
	private double xoff = 80.0;
	private double yoff = 40.0;

	public void draw()
	{
		minx = -1; maxx = 0;
		miny = 0; maxy = 0;

		foreach(var points in sets)
		{
			foreach(Tuple<double?,double?> p in points)
			{
				if(minx == -1) minx = p.x;
				maxx = double.max(p.x,maxx);
				minx = double.min(p.x,minx);
				if(miny == -1) miny =  p.y;
				maxy = double.max(p.y,maxy);
				miny = double.min(p.y,miny);
			}
		}
		xscale = (this.allocation.width-xoff)/(double)(maxx-minx);
		yscale = (this.allocation.height-yoff)/(double)(maxy-miny);     
		draw_axis(Xaxis,true);
		draw_axis(Yaxis,false);


		foreach(var i in sets){
			draw_points(i);
		}
	}

	public void set_points(List<Tuple<double?,double?>?> points)
	{
		sets.append(points);
	}
	public void draw_axis(List<Tuple<double?,string>?>? axis, bool v)
	{
		if(axis != null) {
			var ctx = new Cairo.Context(this.surf);
			ctx.set_source_rgba(0.4,0.4,0.4,1);
			ctx.set_line_width(0.4);
			foreach(Tuple<double?,string> p in axis) {
				if(v) {
					double x= p.x;
					ctx.move_to((x-minx)*xscale+xoff, 0);
					ctx.line_to((x-minx)*xscale+xoff,yscale*maxy);
					ctx.move_to((x-minx)*xscale+xoff, yscale*maxy-18+yoff);
					ctx.show_text(p.n);
					ctx.stroke();
				}else{
					double y= p.x;
					ctx.move_to(xoff,				(maxy-y)*yscale);
					ctx.line_to((maxx)*xscale, yscale*(maxy-y));
					ctx.move_to(0,				(maxy-y)*yscale);
					ctx.show_text(p.n);
					ctx.stroke();
				}
			}
		}
	}
	public void draw_points(List<Tuple<double?,double?>?> points)
	{
		var ctx = new Cairo.Context(this.surf);
		var ctx2 = new Cairo.Context(this.surf);
		ctx.set_source_rgba(GLib.Random.double_range(0,1),GLib.Random.double_range(0,1),GLib.Random.double_range(0,1),1);

        ctx2.set_source_rgba(0,0,0,1);
		ctx.set_line_width(1.2);
		ctx2.set_line_width(2.8);
		Tuple<double?,double?> f = points.first().data;
		double x = f.x;
		double y = f.y;
		ctx.move_to((x-minx)*xscale+xoff,(maxy-y)*yscale);
		foreach(Tuple<double?,double?> p in points)
		{
			x = p.x;
			y = p.y;
       		ctx.line_to( (x-minx)*xscale+xoff, (maxy-y)*yscale);
            ctx2.rectangle((x-minx)*xscale+xoff-1, (maxy-y)*yscale-1,2,2);
            ctx2.stroke();
		}
		ctx.stroke();
	}

	/*clear the graph */
	public void init()
	{
		var ctx = Gdk.cairo_create(this.window);
		stdout.printf("Create surface: %i %i\n", this.allocation.width, this.allocation.height);
		this.surf = new Cairo.Surface.similar(ctx.get_target(),
				Cairo.Content.COLOR_ALPHA,
				this.allocation.width, this.allocation.height);

		ctx = new Cairo.Context(this.surf);

		/* Paint background white */
		ctx.set_source_rgba(1,1,1,1);
		ctx.paint();         
		/* set fg color */
		ctx.set_source_rgba(0,0,0,1);

	}
}
