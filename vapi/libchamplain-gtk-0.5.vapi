using Champlain;
namespace Gtk
{
    [CCode (cheader_filename = "champlain-gtk/champlain-gtk.h")]
    public class ChamplainEmbed : Gtk.Alignment
    {
		[CCode (type = "GtkWidget*", has_construct_function = false)]
        public ChamplainEmbed();

        public Champlain.View get_view();
        public Champlain.View view {
            get;
            set;
        }
    }
}
