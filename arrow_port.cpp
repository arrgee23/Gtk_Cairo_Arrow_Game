#include <cairo.h>
#include <cmath>
#include <gtk/gtk.h>
#include <cstdlib>

const int BALOON_RIGHT_PADDING = 100;	//padding from right border
const int WINDOW_HEIGHT = 600;
const int WINDOW_WIDTH = 800;
const int ARROW_START_X = 0; // arrow starts from this x coordinate from left
int arrows_thrown = 0;
int total_hits = 0;
char* HIT_MESSAGE = "HIT!!";
char* MISS_MESSAGE = "MISS!!";
bool global_ifhit = false;

static gboolean on_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean clicked(GtkWidget *widget, GdkEventButton *event,gpointer user_data);
static gboolean time_handler(GtkWidget *widget);

// returns a random number from 1 - range
int random(int lowlimit,int uplimit)
{
	int num = lowlimit -1;
	while(num < lowlimit)
	{
		//srand (time(NULL)); // slow
		/* generate secret number between 1 and 10: */
		num = rand() % uplimit + 1;
	}
	return num;
}


class Arrow
{
public:
	cairo_surface_t *image;
	static const int height = 15;
	static const int width = 133;
	int wait;
	int tip; //rightmost x coordinate of the image
	int y; //middle y coordinate of arrow
	bool isthrown;
	static const int rightmove_length = 30 ; //speed

	//constructor
	Arrow():tip(width+ARROW_START_X),isthrown(false),wait(0)
	{
		image = cairo_image_surface_create_from_png("bow.png");
	}

	// when clicked set the y coordinate
	void set_thrown()
	{
		isthrown = true;
	}

	void draw(cairo_t *cr)
	{

		if(isthrown)
		{
			cairo_set_source_surface(cr, image, tip-width, y);
			cairo_paint(cr);
			move(cr);
		}
	}
	void move(cairo_t *cr)
	{
		if(tip < WINDOW_WIDTH + width)
		{
			tip += rightmove_length;

		}
		else // arrow crossed boundary
		{
			
            if(!wait)
            {
                // set msg
                if(global_ifhit)
                {
					total_hits++;
                    message.set(HIT_MESSAGE);
                    global_ifhit = false;
                }
                else
                    message.set(MISS_MESSAGE);
            }
			//display msg
			message.display(cr);
			//wait
			wait++;
			if(wait==20)
			{
                wait=0;
				isthrown = false;
				tip = width+ARROW_START_X;
				arrows_thrown++;
			}
		}
	}
};
Arrow arrow;

class Balloon
{
public:
	cairo_surface_t *blast_image;
	cairo_surface_t *image;
	int y;
	int swirl;
	bool swirling;
	int swirl_threshold;
	int wait;
	static const int height = 124;
	static const int width = 73;
	int x ; // balloon's x coordinate 
	int upmove_length; // adjusts how many pixesls the balloon moves in every draw call
	bool hit ;

	// default constructor sets y coordinate to bottom left of the screen
	Balloon(int X,double UPMOVE_LENGTH,int SWIRL_THRESHOLD):y(WINDOW_HEIGHT + height/2),hit(false), wait(0), swirl(0), swirling(false)
	{
		x=X;
		upmove_length = UPMOVE_LENGTH;
        swirl_threshold=SWIRL_THRESHOLD;
		image = cairo_image_surface_create_from_png("pink.png");
		blast_image = cairo_image_surface_create_from_png("blast.png");
    }

	//draws a balloon in its x and y coordinate when a cairo_t is passed
	void draw(cairo_t *cr)
	{
		if(!hit)
		{
			cairo_set_source_surface(cr, image, x+swirler(), y);
			cairo_paint(cr);
			move();
		}
		else // reset the balloon coordinate if hit
		{
			global_ifhit = true;
			cairo_set_source_surface(cr, blast_image, x, y);
			wait++;
			cairo_paint(cr);
			if(wait==20)
			{
                reset();
                wait=0;
            }
		}
	}

	int swirler()
	{
        if(swirling)
            swirl++;
        else
            swirl--;
        if(std::abs(swirl)>=swirl_threshold)
        {
            swirling=(swirling)?false:true;
        }
        return swirl;
	}

	void reset()
	{
		y = WINDOW_HEIGHT + height/2;hit = false;
		x=random(500,700);
		upmove_length = random(1,10);
        swirl_threshold=random(10,30);
	}
	void move()
	{

		if(y < 0 - BALOON_RIGHT_PADDING)
			reset(); // if ballon at top move it back
		else
			y -= upmove_length; // else move it up
	}
	void check_hit(const Arrow& arrow)
	{
		 if((arrow.tip< x+width/2 && arrow.tip > x-width/2 ) && (arrow.y < y+height/2 && arrow.y > y))
			hit = true;
	}
};
Balloon balloon(random(500,700),random(1,10),random(10,30));

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *darea;

	GdkColor color;
	color.red = 8995;
	color.green = 49087;
	color.blue = 57311;

	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	darea = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER (window), darea);

	g_signal_connect(G_OBJECT(darea), "expose-event", G_CALLBACK(on_draw_event), NULL); //expose-event
	g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK );
	g_signal_connect(window, "button-press-event", G_CALLBACK(clicked), NULL); 


	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(window), "Arrow");
	gtk_widget_modify_bg(darea, GTK_STATE_NORMAL, &color);

	g_timeout_add(100, (GSourceFunc) time_handler, (gpointer) window);  //calls time_handler 10 ms apart

	gtk_widget_show_all(window);

	gtk_main();

	  return 0;
}

static gboolean on_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);
	
    arrow.draw(cr);
    
	balloon.draw(cr);
	balloon.check_hit(arrow);
	
    return FALSE;
}


static gboolean time_handler(GtkWidget *widget)
{
    gtk_widget_queue_draw(widget);

    return TRUE;
}

static gboolean clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 1)
	{
        arrow.y = event->y;
        arrow.set_thrown();
    }
    return TRUE;
}
