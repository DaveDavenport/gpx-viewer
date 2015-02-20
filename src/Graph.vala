using GLib;
using Gtk;
using Cairo;

namespace Graph
{
	public struct Point<G> 
	{
		string label;
		double x;
		double y;
		G value;
	}
	/**
	 * Draw an average line based on the points of another dataset.
	 * It can automatically draw average if and only if points are equally spaced.. 
	 * If not, set it manually.
	 */
	public class DataSetAverage<G> : DataSet<G>
	{
		private DataSet ds;
		private bool average_set = false;
		private double _average = 0;


		public double average {
				get {
					return _average;
				}
				set{
					average_set = true;
					_average = value;
					min_y_point = value;
					max_y_point = value;
					this.changed();
				}
		}
		public override void draw(Cairo.Context ctx, double height, double width,
				double min_x, double max_x,
				double min_y, double max_y)
		{	
			double x_range = max_x-min_x;
			double y_range = max_y-min_y;

			ctx.set_line_width(1.5);
			ctx.set_line_cap(Cairo.LineCap.ROUND);
			ctx.set_source_rgb(r,g,b);

			ctx.move_to(
					width*(0)/(x_range),
					height*(1-(_average-min_y)/(y_range))
					);
			ctx.line_to(
					width*(max_x-min_x)/(x_range),
					height*(1-(_average-min_y)/(y_range))
					);
			ctx.stroke();
		}
		// stupid and fix this.
		// only works for equal intervals.
		private void update_dataset_average(DataSet source)
		{
			if(average_set) return;
			double total = 0;
			uint num_points = 0;
			_average = 0;
			foreach(Point<G> p in source.points)
			{
				total+= p.y;
				num_points++;
			}
			if(num_points > 0 ) {
				_average = total/(double)num_points;
			}
			this.changed();
		}
		public DataSetAverage(DataSet ds)
		{
			this.ds = ds;
			this.ds.changed.connect((source) => {
					update_dataset_average(source);
					});
			update_dataset_average(ds);
		}

	}
	/**
	 * This draws a line in the graph without dots.
	 */
	public class DataSetLine<G> : DataSet<G>
	{
		public bool dots { get; set; default=true;} 

		public override void draw(Cairo.Context ctx, double height, double width,
				double min_x, double max_x,
				double min_y, double max_y)
		{	
			double x_range = max_x-min_x;
			double y_range = max_y-min_y;

			ctx.set_line_width(1.5);
			ctx.set_line_cap(Cairo.LineCap.ROUND);
			ctx.set_source_rgb(r,g,b);

			foreach(Point<G> p in points)
			{
				ctx.line_to(
						width*(p.x-min_x)/(x_range),
						height*(1-(p.y-min_y)/(y_range))
				);
				ctx.stroke();
				ctx.line_to(
						width*(p.x-min_x)/(x_range),
						height*(1-(p.y-min_y)/(y_range))
				);
			}
			ctx.stroke();
			if(dots)
			{
				ctx.set_source_rgb(r,g,b);
				foreach(Point<G> p in points)
				{
					ctx.arc(
							width*(p.x-min_x)/(x_range),
							height*(1-(p.y-min_y)/(y_range)),
							double.max(3,height/300),
							0,2*Math.PI
						   );
					ctx.fill();
				}


			}
		}

	}
	/**
	 * This dataset plots bars 
	 */
	public class DataSetBar<G> : DataSet<G>
	{
		public double bar_width = 20;
		public DataSetBar()
		{
			this.changed.connect((source)=>{
				update_bar_width();
				stdout.printf("Bar width: %f\n", bar_width);
				unowned List<Point<G>?> l = points.last();
				if(l != null) {
					max_x_point = l.data.x+bar_width/2-1;
					min_x_point = points.first().data.x-bar_width/2+1;
				}
			});
		}
		void update_bar_width()
		{
			bar_width = double.MAX;
			if(points.first() == null) 
			{
				bar_width = 20;
				return;
			}
			Point<G> prev = points.first().data;
			foreach(Point<G> p in points)
			{
				if(prev != p)
				{
					double wd = ((p.x-prev.x)/2-1)*2;
					bar_width = double.min(wd, bar_width);
				}
				prev = p;
			}
			
		}

		public override void draw(Cairo.Context ctx, double height, double width,
				double min_x, double max_x,
				double min_y, double max_y)
		{	
			double x_range = max_x-min_x;
			double y_range = max_y-min_y;
			bar_width = double.min(bar_width, x_range-1);
			stdout.printf("Bar width: %f\n", bar_width);
			Point<G> prev = points.first().data;
			foreach(Point<G> p in points)
			{
				//if(prev != p) 
				{
					ctx.move_to(width*(p.x-bar_width/2-min_x)/x_range,
							height*(1-(0-min_y)/(y_range)));
					ctx.line_to(width*(p.x-bar_width/2-min_x)/x_range,
							height*(1-(p.y-min_y)/(y_range)));

					ctx.line_to(width*(p.x+bar_width/2-min_x)/x_range,
							height*(1-(p.y-min_y)/(y_range)));

					ctx.line_to(width*(p.x+bar_width/2-min_x)/x_range,
							height*(1-(0-min_y)/(y_range)));

					ctx.close_path();
					ctx.set_source_rgba(r,g,b,0.4);
					ctx.fill_preserve();
					ctx.set_source_rgb(r,g,b);
					ctx.stroke();
				}
				prev = p;
			}
		}
		public override Point? match_point (double x, double y)
		{
			unowned List<Point?> point = points.first();
			while(point != null)
			{
				Point<G> a = point.data;
				
				if(Math.fabs(a.x - x) < (bar_width/2-1))
				{
					stdout.printf("value: %f %f\n", a.x, a.y);
					return a;
				}
				point = point.next;
			}	

			return null;
		}
	}
	/**
	 * This dataset plots area beneath 
	 */
	public class DataSetArea<G> : DataSet<G>
	{

		public override void draw(Cairo.Context ctx, double height, double width,
				double min_x, double max_x,
				double min_y, double max_y)
		{	
			double x_range = max_x-min_x;
			double y_range = max_y-min_y;

			ctx.set_line_width(1.5);
			ctx.set_line_cap(Cairo.LineCap.ROUND);
			ctx.set_source_rgb(r,g,b);

			ctx.move_to(width*(min_x_point-min_x)/x_range, height*(1+min_y/y_range));
			foreach(Point<G> p in points)
			{
				ctx.line_to(
						width*(p.x-min_x)/(x_range),
						height*(1-(p.y-min_y)/(y_range))
				);
			}
			ctx.line_to(width*(this.max_x_point-min_x)/x_range, height*(1+min_y/y_range));
			ctx.stroke_preserve();
			ctx.line_to(width*(this.min_x_point-min_x)/x_range, height*(1+min_y/y_range));
			ctx.set_source_rgba(r,g,b,0.4);
			ctx.fill();

			ctx.set_source_rgb(r,g,b);
			foreach(Point<G> p in points)
			{
				ctx.arc(
						width*(p.x-min_x)/(x_range),
						height*(1-(p.y-min_y)/(y_range)),
						double.max(3,height/300),
						0,2*Math.PI
				);
				ctx.fill();
			}
		}
	}
	/**
	 * This dataset plots lines 
	 */
	public class DataSet<G> 
	{
		// todo make protected again.
		public List<Point?> points = new List<Point?>();
		public double min_x_point = double.MAX;
		public double min_y_point = double.MAX;
		public double max_x_point = 0;
		public double max_y_point = 0;

		protected double r = 0.0;
		protected double g = 0.0;
		protected double b = 0.0;

		public signal void changed();


		public delegate string FormatTooltipCallback<T> (T p);

		public FormatTooltipCallback? format_callback =null;

		public void recalculate()
		{
			stdout.printf("recalculate\n");
			min_x_point = double.MAX;
			min_y_point = double.MAX;
			max_x_point = 0;
			max_y_point = 0;
			foreach(var p in points)
			{
				if(p.x < min_x_point) min_x_point = p.x;
				if(p.y < min_y_point) min_y_point = p.y;

				if(p.x > max_x_point) max_x_point = p.x;
				if(p.y > max_y_point) max_y_point = p.y;
			}
		}
		public void set_color(double r, double g, double b)
		{
			this.r = r;
			this.g = g;
			this.b = b;
			this.changed();
		}
		public void add_point(double x, double y)
		{
			if( x < min_x_point)	
				min_x_point = x;
			if( y < min_y_point)	
				min_y_point = y;
			if( x > max_x_point)	
				max_x_point = x;
			if( y > max_y_point)	
				max_y_point = y;

			Point<G> p = Point<G>();
			p.x = x;
			p.y = y;
			points.append(p);
			this.changed();
		}
		public void add_point_value ( double x, double y, G value)
		{
			if( x < min_x_point)	
				min_x_point = x;
			if( y < min_y_point)	
				min_y_point = y;
			if( x > max_x_point)	
				max_x_point = x;
			if( y > max_y_point)	
				max_y_point = y;

			Point<G> p = Point<G>();
			p.x = x;
			p.y = y;
			p.value = value;
			points.append((owned)p);
			this.changed();
		}

		public virtual void draw(Cairo.Context ctx,
				double height, double width,
				
				double min_x, double max_x,
				double min_y, double max_y
			)
		{	
			GLib.error("Base class, do not use directly.");
		}


		public virtual Point? match_point (double x, double y)
		{
			unowned List<Point?> point = points.first();
			while(point != null && point.next != null)
			{
				Point<G> a = point.data;
				Point<G> b = point.next.data;
				
				if(Math.fabs(a.x - x) < (b.x-a.x)/2.0)
				{
					stdout.printf("value: %f %f\n", a.x, a.y);
					return a;
				}
				else if(Math.fabs(x-b.x) < (b.x-a.x)/2.0)
				{
					stdout.printf("value1: %f %f\n", b.x, b.y);
					return b;
				}
				point = point.next;
			}	

			return null;
		}
	}
	/**************************************************************************
     * GRAPH
	 *************************************************************************/
	public class Graph
	{

		private Pango.FontDescription  fd   = null;
		private Cairo.Surface 		   surf = null;
		private Cairo.Surface 		   grid = null;
		private Cairo.Surface 		   highlight = null;


		private double width = 0;
		private double height = 0;

		public double min_x_point = double.MAX;
		public double min_y_point = double.MAX;
		public double max_x_point = 0;
		public double max_y_point = 0;

		// List with data sets.
		private List<DataSet> dss = new List<DataSet>();

		// Labels
		private string? _x_axis_label = null;
		private string? _y_axis_label = null; 
		private string? _title_label  = null;
		public string? x_axis_label {
			get{ 
				return _x_axis_label;
			}
			set{
				_x_axis_label = value;
				this.clear();
			}
		}

		public string? y_axis_label {
			get{ 
				return _y_axis_label;
			}
			set{
				_y_axis_label = value;
				this.clear();
			}
		}
		public string? title_label {
			get{ 
				return _title_label;
			}
			set{
				_title_label = value;
				this.clear();
			}
		}
		private bool use_auto_yticks = false;
		private bool use_auto_xticks = false;
		// font size
		private double _title_label_font_size = 24*Pango.SCALE;
		private double _axis_label_font_size = 22*Pango.SCALE;
		private double _label_font_size = 12*Pango.SCALE;

		public double title_label_font_size {
			get {
				return _title_label_font_size/Pango.SCALE;
			}
			set {
				_title_label_font_size = value*Pango.SCALE;
				this.clear();
			}
		}

		public double axis_label_font_size {
			get {
				return _axis_label_font_size/Pango.SCALE;
			}
			set {
				_axis_label_font_size = value*Pango.SCALE;
				this.clear();
			}
		}
		public double label_font_size {
			get {
				return _label_font_size/Pango.SCALE;
			}
			set {
				_label_font_size = value*Pango.SCALE;
				this.clear();
			}
		}

		/**
		 * Xticks.
		 */
		private List<Point?> xticks = new List<Point?>();
		public void add_xticks(double x, string value)
		{
			Point<int> p = Point<int>();
			p.x = x;
			p.label = value;
			xticks.append((owned)p);
		}

		/**
		 * Yticks.
		 */
		private List<Point?> yticks = new List<Point?>();
		public void add_yticks(double y, string value)
		{
			Point<int> p = Point<int>();
			p.y = y;
			p.label = value;
			yticks.append((owned)p);
		}

		public void add_data_set(DataSet ds)
		{
			dss.append(ds);
			ds.changed.connect((source)=>{
					this.clear();
					this.changed();
					});
			this.clear();
		}

/*
		public DataSet create_data_set(Type g)
		{
			DataSet ds = new DataSetLine<g>();	
			add_data_set(ds);
			return ds;
		}

		public DataSet create_data_set_area()
		{
			DataSet ds = new DataSetArea();	
			add_data_set(ds);
			return ds;
		}
		public DataSet create_data_set_bar()
		{
			DataSet ds = new DataSetBar();	
			add_data_set(ds);
			return ds;
		}
*/


		private double left_offset		= 5.0;
		private double right_offset 	= 30.0;
		private double top_offset		= 5.0;
		private double bottom_offset	= 5.0;

		public Graph()
		{

			/* Create and setup font description */
			this.fd = new Pango.FontDescription();
			fd.set_family("sans mono");
		}

		/**
		 * Call this if you want to force a full redraw of the graph.
		 */
		public void clear()
		{
			/* Invalidate the previous plot, so it is redrawn */
			this.surf = null;
			this.grid = null;
			this.highlight = null;
		}

		private double calculate_step_size(double range)
		{
			double step = 1;
			double order = Math.pow(10, Math.floor(Math.log10(range)));
			if(order < 1) return 0.0;
			// Want +- 10 steps.
			order /= 10;

			step = order;
			stdout.printf("calc step size: order: %f %f\n", order,range);
			while(!(step*9 <= range && step *10 >= range))
			{
				step += order/10;
			}

			return step;
		}

		/**
		 * Create an 'auto' set of yticks.
		 */
		private void auto_yticks()
		{
			double y_range = max_y_point-min_y_point;
			yticks = null;
			
			double i = 0;
			double step = calculate_step_size(y_range); 
			double max = Math.ceil(max_y_point/step)*step+0.15*step;
			for ( i =double.max(min_y_point,0) ; i <= max; i+=step)
			{
				this.add_yticks(i, "%.2f".printf(i));
			}
			for ( i =-1/step; i > min_y_point; i-=1/step)
			{
				this.add_yticks(i, "%.2f".printf(i));
			}
			use_auto_yticks = true;
		}

		private void auto_xticks()
		{
			double x_range = max_x_point-min_x_point;
			xticks = null;
			
			double i = 0;
			double step = calculate_step_size(x_range); 
			double max = Math.ceil(max_x_point/step)*step+0.15*step;
			stdout.printf("max: %f\n", max);
			for ( i =double.max(min_x_point,0) ; i <= max; i+=step)
			{
				stdout.printf("step: %f\n",i);
				this.add_xticks(i, "%.2f".printf(i));
			}
			for ( i =-step; i > min_x_point; i-=step)
			{
				stdout.printf("step-: %f\n",i);
				this.add_xticks(i, "%.2f".printf(i));
			}
			use_auto_xticks = true;
		}
		/**
		 * Based on the minimum and maximum points off the different datasets
		 * calculate the bounding box (aka range) of the x and y values.
		 */
		private void calculate_bounding_box()
		{
			min_x_point = double.MAX;
			min_y_point = double.MAX;
			max_x_point = double.MIN;
			max_y_point = double.MIN;
			bool mod = false;
			foreach (DataSet ds in dss)
			{
				min_x_point = (ds.min_x_point < min_x_point)? ds.min_x_point:min_x_point;
				max_x_point = (ds.max_x_point > max_x_point)? ds.max_x_point:max_x_point;
				min_y_point = (ds.min_y_point < min_y_point)? ds.min_y_point:min_y_point;
				max_y_point = (ds.max_y_point > max_y_point)? ds.max_y_point:max_y_point;
				if(ds.points.length() > 0)
					mod = true;
			}
			if(!mod) return;
			if(yticks.length()== 0 || use_auto_yticks)
			{
				stdout.printf("Do auto yticks\n");
				this.auto_yticks();
			}
			if(xticks.length()== 0 || use_auto_xticks)
			{
				stdout.printf("Do auto xticks\n");
				this.auto_xticks();
			}
			foreach  ( Point<int> p in xticks)
			{
				min_x_point = (p.x < min_x_point)?p.x:min_x_point;
				max_x_point = (p.x > max_x_point)?p.x:max_x_point;
			}
			foreach  ( Point<int> p in yticks)
			{
				min_y_point = (p.y < min_y_point)?p.y:min_y_point;
				max_y_point = (p.y > max_y_point)?p.y:max_y_point;
			}
		}

		private void calculate_margins(Cairo.Context ctx, Pango.Layout layout)
		{
			// Set defaults.
			left_offset    = 30;
			right_offset   = 30;
			top_offset     = 15;
			bottom_offset  = 15;

			fd.set_absolute_size(_label_font_size);
			layout.set_font_description(fd);

			ctx.set_source_rgb(0.6,0.6,0.6);
			ctx.set_line_width(1.0);

			// Calculate how much space we need.

			double text_width_offset = 0;
			double text_height_offset = 0;
			double text_width = 0;

			foreach(weak Point<int> p in yticks)
			{
				if(p.label != null)
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);
					text_width_offset = double.max(text_width_offset, wt*1.2);
				}
			}
			foreach(weak Point<int> p in xticks)
			{
				if(p.label != null)
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);
					text_height_offset = double.max(text_height_offset, ht*1.2+5);
					text_width= double.max(text_width, wt*1.2);
				}
			}
			left_offset= double.max(left_offset, text_width_offset);
			bottom_offset= double.max(bottom_offset, text_height_offset);


			// Calculate label offset.
			if(_x_axis_label != null && _x_axis_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_axis_label_font_size);
				layout.set_font_description(fd);
				layout.set_text(_x_axis_label, -1);
				layout.get_pixel_size(out wt, out ht);

				bottom_offset+= 10+ht;
			}
			if(_y_axis_label != null && _y_axis_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_axis_label_font_size);
				layout.set_font_description(fd);
				layout.set_text(_y_axis_label, -1);
				layout.get_pixel_size(out wt, out ht);

				left_offset+= 10+ht;
			}
			if(_title_label != null && _title_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_title_label_font_size);
				layout.set_font_description(fd);
				layout.set_text(_title_label, -1);
				layout.get_pixel_size(out wt, out ht);

				top_offset += 10+ht;
			}
		}

		private void update_grid(Cairo.Context ctx_ori, Gtk.Allocation alloc)
		{
			// Create new clean surface to paint on.
			this.grid = new Cairo.Surface.similar(ctx_ori.get_target(),
					Cairo.Content.COLOR_ALPHA,
					alloc.width, alloc.height);

			stdout.printf("type: %d\n", this.grid.get_type());
			// TODO: Move this to a better place.
			calculate_bounding_box();

			
			var ctx = new Cairo.Context(this.grid);


			var layout = Pango.cairo_create_layout(ctx);

			// Get the margnings.
			calculate_margins(ctx, layout);


			// Height off the actual graph.
			double height = (double)(alloc.height-bottom_offset-top_offset);	
			double width = (double)(alloc.width-left_offset-right_offset);	


			/**
			 * Xticks.
			 */
			fd.set_absolute_size(_label_font_size);
			layout.set_font_description(fd);
			uint req_entries = xticks.length()+1;

			double text_width = 0;
			foreach(weak Point<int> p in xticks)
			{
				if(p.label != null)
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);
					text_width = double.max(text_width, wt*1.2);
				}
			}

			uint entries = (uint) Math.floor(width/text_width);
			uint step = 1;
			while(entries < req_entries){
				req_entries >>=1;
				step*=2;
			}
			entries = 0;	
			foreach(weak Point<int> p in xticks)
			{
				if(entries%step == (step-1))
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);

					// Draw label.
					ctx.move_to(left_offset+width*((p.x-min_x_point)/(max_x_point-min_x_point))-wt/2, top_offset+height+ht/2);
					Pango.cairo_layout_path(ctx, layout);
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.fill();

					// Draw line.
					ctx.move_to(
							left_offset+width*((p.x-min_x_point)/(max_x_point-min_x_point)),
							top_offset);
					ctx.line_to(
							left_offset+width*((p.x-min_x_point)/(max_x_point-min_x_point)),
							height+top_offset);
					ctx.set_source_rgba(0.5, 0.5, 0.5, 1.0);
					ctx.stroke();
				}
				entries++;
			}
		
			/**
			 * YTicks
			 */	

			double text_height = 0;
			text_width = 0;
			foreach(weak Point<int> p in yticks)
			{
				if(p.label != null)
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);
					text_height = double.max(text_height, ht*1.2);
					text_width = double.max(text_width, wt*1.2);
				}
			}
			req_entries = yticks.length()+1;
			entries = (uint) Math.floor(height/text_height);
			step = 1;
			while(entries < req_entries){
				req_entries >>=1;
				step*=2;
			}
			entries = 0;	
			foreach(weak Point<int> p in yticks)
			{
				if(entries%step == (step-1))
				{
					int wt,ht;
					layout.set_text(p.label, -1);
					layout.get_pixel_size(out wt, out ht);

					// Draw label.
					ctx.move_to(left_offset-wt-5, top_offset+height*(1-(p.y-min_y_point)/(max_y_point-min_y_point))-ht/2); 
					Pango.cairo_layout_path(ctx, layout);
					ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
					ctx.fill();

					ctx.set_source_rgba(0.5, 0.5, 0.5, 1.0);
					ctx.move_to(left_offset, top_offset+height*(1-(p.y-min_y_point)/(max_y_point-min_y_point))); 
					ctx.line_to(left_offset+width, top_offset+height*(1-(p.y-min_y_point)/(max_y_point-min_y_point))); 
					ctx.stroke();
				}
				entries++;
			}

			/**
		     * Final grid
			 */

			ctx.translate(left_offset,top_offset);

			ctx.set_source_rgb(0.0,0.0,0.0);
			ctx.set_line_width(2.0);
			ctx.move_to(0, 0);
			ctx.line_to(0, height); 
			ctx.stroke();

			double min_point = 0;
			if( min_y_point > 0) min_point = min_y_point;

			ctx.move_to(0, 		height*(1-(min_point-min_y_point)/(max_y_point-min_y_point)));
			ctx.line_to(width,  height*(1-(min_point-min_y_point)/(max_y_point-min_y_point)));
			ctx.stroke();

			if(_title_label != null && _title_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_title_label_font_size);

				layout.set_width(Pango.SCALE*alloc.width);
				layout.set_ellipsize(Pango.EllipsizeMode.MIDDLE);
				layout.set_font_description(fd);
				layout.set_text(_title_label, -1);
				layout.get_pixel_size(out wt, out ht);

				// Draw the label.
				ctx.move_to(alloc.width/2-wt/2-left_offset, -top_offset+5);
				Pango.cairo_layout_path(ctx, layout);
				ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
				ctx.fill();

			}


			if(_x_axis_label != null && _x_axis_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_axis_label_font_size);
				layout.set_font_description(fd);
				layout.set_text(_x_axis_label, -1);
				layout.get_pixel_size(out wt, out ht);

				// Draw the label.
				ctx.move_to(width/2-wt/2, height+bottom_offset-ht-5);
				Pango.cairo_layout_path(ctx, layout);
				ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
				ctx.fill();
			}

			if(_y_axis_label != null && _y_axis_label.length > 0)
			{
				int wt,ht;
				fd.set_absolute_size(_axis_label_font_size);
				layout.set_font_description(fd);
				layout.set_text(_y_axis_label, -1);
				layout.get_pixel_size(out wt, out ht);

				// Draw the label.
				ctx.move_to(-left_offset+5, height/2+wt/2);
				ctx.rotate(-0.5*Math.PI);
				Pango.cairo_layout_path(ctx, layout);
				ctx.set_source_rgba(0.0, 0.0, 0.0, 1.0);
				ctx.fill();
			}
		}

		private void update_surface(Cairo.Context ctx_ori, Gtk.Allocation alloc)
		{
			this.surf = new Cairo.Surface.similar(ctx_ori.get_target(),
					Cairo.Content.COLOR_ALPHA,
					alloc.width, alloc.height);

			stdout.printf("type surf: %d\n", this.grid.get_type());

			var ctx = new Cairo.Context(this.surf);


			ctx.translate(left_offset,top_offset);
			double height = (double)(alloc.height-bottom_offset-top_offset);
			double width = (double)(alloc.width-left_offset-right_offset);

			ctx.rectangle(0,0,width,height);
			ctx.clip();
			foreach (DataSet ds in dss)
			{
				ds.draw(ctx,height ,width ,
					min_x_point, max_x_point,
					min_y_point, max_y_point);
			}
		}

		/**
		 * Repaint the graph to a certain context
		 * In a widget call this from the draw handler.
		 */
		public void repaint(Cairo.Context ctx, Gtk.Allocation alloc)
		{
			height = (double)(alloc.height-bottom_offset-top_offset);
			width = (double)(alloc.width-left_offset-right_offset);


			/**
			 * Paint the background white
			 */
			ctx.set_source_rgb(1.0,1.0,1.0);
			ctx.paint();

			/**
			 * Paint the grid layer.
			 */
			if(this.grid == null) update_grid(ctx, alloc);

			ctx.set_source_surface(this.grid, 0, 0);
			ctx.paint();

			/**
			 * Paint the surf layer.
			 */
			if(this.surf == null) update_surface(ctx, alloc);
			ctx.set_source_surface(this.surf, 0, 0);
			ctx.paint();

			if(this.highlight!= null)
			{
				ctx.translate(left_offset,top_offset);
				ctx.set_source_surface(this.highlight, 0, 0);
				ctx.paint();
			}
			

		}

		public signal void changed();

		private void draw_highlight(DataSet ds, Cairo.Context ctx, Point p)
		{
			double x_range = max_x_point-min_x_point;
			double y_range = max_y_point-min_y_point;
			var layout = Pango.cairo_create_layout(ctx);
			fd.set_absolute_size(_label_font_size);
			layout.set_font_description(fd);
			if(ds.format_callback != null)
			{
				layout.set_text(ds.format_callback(p.value),-1);
			}else{
				layout.set_text("x: %8.02f\ny: %8.02f".printf(p.x, p.y),-1);
			}

			int th,tw;
			layout.get_pixel_size(out tw, out th);

			double x_pos = double.max(double.min(width-tw-7, width*(p.x-min_x_point)/x_range-tw/2),1);
			double y_pos = height*(1-(p.y-min_y_point)/y_range);
			y_pos = double.max(1, y_pos-th-7);
			y_pos = double.min(height-th-7, y_pos);

			ctx.rectangle(x_pos, y_pos, tw+6, th+6);
			ctx.set_source_rgba(1, 0.9, 0.5, 0.8);
			ctx.fill_preserve();
			ctx.set_source_rgba(0,0,0,1);
			ctx.stroke();
			ctx.move_to(x_pos+3, y_pos+3);
			Pango.cairo_layout_path(ctx, layout);
			ctx.fill();

		}
		public void highlight_clear()
		{
			if(this.highlight != null)
			{
				this.highlight = null;
				this.changed();
			}
		}
		public void highlight_point(double x, double y)
		{
			double x_range = max_x_point-min_x_point;
			double y_range = max_y_point-min_y_point;
			if(x < left_offset || x> (width+left_offset)) return;
			if(y < top_offset || y > (height+top_offset)) return; 
			foreach(var ds in dss)
			{
				stdout.printf("x: %f y: %f\n", 
							min_x_point+x_range*((x-left_offset)/width),
							min_y_point+y_range*((height - y+top_offset)/height));
				Point? p = ds.match_point(
							min_x_point+x_range*((x-left_offset)/width),
							min_y_point+y_range*((height-y+top_offset)/height));
				if(p != null)
				{
					if(this.highlight == null)
					{
						this.highlight = new Cairo.Surface.similar(this.surf,
								Cairo.Content.COLOR_ALPHA,
								(int)width, (int)height);
					}
					var ctx = new Cairo.Context(this.highlight);
					ctx.set_operator(Cairo.Operator.CLEAR);
					ctx.paint();

					ctx.set_operator(Cairo.Operator.OVER);

					ctx.rectangle( width*((p.x-min_x_point)/x_range)-2.5,
							height*(1-(p.y-min_y_point)/y_range)-2.5, 5, 5);
					ctx.stroke();
					draw_highlight(ds,ctx,p);

					this.changed();
					return;
				}
			}
			if(this.highlight != null)
			{
				var ctx = new Cairo.Context(this.highlight);
				ctx.set_operator(Cairo.Operator.CLEAR);
				ctx.paint();
			}
		}
	}
	public class Widget: Gtk.EventBox
	{
		public Graph graph = new Graph();
		private uint _real_resize_timeout = 0;
		private bool real_resize()
		{
			this.graph.clear();
			_real_resize_timeout = 0;
			stdout.printf("Resizing\n");
			this.queue_draw();
			return false;
		}
		private void size_allocate_cb(Gtk.Allocation alloc)
		{
			if(_real_resize_timeout > 0) {
				GLib.Source.remove(_real_resize_timeout);
			}
			_real_resize_timeout = GLib.Timeout.add(300, real_resize);
		}


		private uint32 _real_movement_timeout = 0;

		public Widget()
		{
			/* make the event box paintable and give it an own window to paint on */
			this.app_paintable = true;
			this.visible_window = true;
			/* signals */
			graph.changed.connect((source)=>{
				this.queue_draw();
					});

			this.add_events(Gdk.EventMask.POINTER_MOTION_MASK);
			this.motion_notify_event.connect((source, event)=>{
				if(_real_movement_timeout > 0) {
					graph.highlight_clear();
					GLib.Source.remove(_real_movement_timeout);
				}
				// timeout to 300 ms.
				_real_movement_timeout = GLib.Timeout.add(300, ()=>{
					graph.highlight_point(event.x, event.y);
					_real_movement_timeout = 0;
					return false;
					});
				return false;
			});

			this.size_allocate.connect(size_allocate_cb);
			this.draw.connect(a_expose_event);
		}



		private bool a_expose_event(Cairo.Context ctx)
		{
			/* Get allocation */
			Gtk.Allocation alloc;
			this.get_allocation(out alloc);
			this.graph.repaint(ctx, alloc);
			return false;
		}
	}
	public class PNG
	{
		public Graph graph = new Graph();
		public PNG()
		{
		}
		public void output(string filename, double width, double height)
		{
			Cairo.Surface sf = new Cairo.ImageSurface(Format.ARGB32,(int)width, (int)height);
			graph.clear();
			Gtk.Allocation alloc = Gtk.Allocation();
			alloc.width = (int)width;
			alloc.height = (int)height;
			var ctx = new Cairo.Context(sf);
			graph.repaint(ctx,alloc);	
            sf.write_to_png(filename);
		}

	}
	public class Svg
	{
		public Graph graph = new Graph();
		public Svg()
		{
		}
		public void output(string filename, double width, double height)
		{
			Cairo.Surface sf = new Cairo.SvgSurface(filename, width, height);
			graph.clear();
			Gtk.Allocation alloc = Gtk.Allocation();
			alloc.width = (int)width;
			alloc.height = (int)height;
			var ctx = new Cairo.Context(sf);
			graph.repaint(ctx,alloc);	
		}

	}
	public class Eps
	{
		public Graph graph = new Graph();
		public Eps()
		{
		}
		public void output(string filename, double width, double height)
		{
			Cairo.PsSurface sf = new Cairo.PsSurface(filename, width, height);
			sf.set_eps(true);
			Gtk.Allocation alloc = Gtk.Allocation();
			alloc.width = (int)width;
			alloc.height = (int)height;
			var ctx = new Cairo.Context(sf);
			graph.clear();
			graph.repaint(ctx,alloc);	
		}

	}
}
