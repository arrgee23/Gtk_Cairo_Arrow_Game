#include <cairo.h>
#include <cmath>
#include <gtk/gtk.h>
#include <cstdlib>

const int BALOON_RIGHT_PADDING = 100;	//padding from right border
const int WINDOW_HEIGHT = 768;
const int WINDOW_WIDTH = 1366;
const int ARROW_START_X = 0; // arrow starts from this x coordinate from left
int arrows_thrown = 0;
int total_hits = 0;
char* HIT_MESSAGE[] = {
						"HIT!!",
						"BIRD!!!"
					};

char* MISS_MESSAGE[] = {
						"MISS!!",
						"CLOSE"
						};
bool global_ifhit = false;
bool global_birdhit = false;

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
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

class Scoreboard
{
	public:
	char buffer [50];
	void set()
	{
	  	sprintf (buffer, "Arrows Thrown: %d    Hits: %d",arrows_thrown,total_hits);
	}
	void display(cairo_t *cr)
	{
		set();
		cairo_set_source_rgb(cr,0, 0, 0);

		cairo_select_font_face(cr, "Purisa",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);

		cairo_set_font_size(cr, 20);

		cairo_move_to(cr, WINDOW_WIDTH - 400, 40);
		cairo_show_text(cr, buffer);
	}
};
Scoreboard scoreboard;

class Message
{
	public:
	char* msg;
	Message(){};
	Message(char* message)
	{
		msg = message;
	}
	void set(char* message)
	{
		msg = message;
	}
	void display(cairo_t *cr)
	{
		cairo_set_source_rgb(cr,255, 0, 0);

		cairo_select_font_face(cr, "Purisa",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);

		cairo_set_font_size(cr, 50);

		cairo_move_to(cr, WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2);
		cairo_show_text(cr, msg);
	}
	void input_display(char* buffer,cairo_t *cr)
	{
		cairo_set_source_rgb(cr,255, 0, 0);

		cairo_select_font_face(cr, "Purisa",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);

		cairo_set_font_size(cr, 50);

		cairo_move_to(cr, WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2);
		cairo_show_text(cr, buffer);
	}

};
Message message;

class Sun
{
	
public:
	cairo_surface_t *image;
	int y;
	int x;
	Sun(int a, int b):x(a), y(b)
	{
        image = cairo_image_surface_create_from_png("sun.png");
    }
	void draw(cairo_t *cr)
	{
		cairo_set_source_surface(cr, image, x, y);
		cairo_paint(cr);
	}

};
Sun sun(300,150);

class Bush
{
public:
	cairo_surface_t *image;
	int y;
	int x;
	static const int image_height=156;
	static const int image_width=388;
	Bush(int a, int b):x(a), y(b)
	{
        image = cairo_image_surface_create_from_png("bush.png");
    }
	void draw(cairo_t *cr)
	{
		cairo_set_source_surface(cr, image, x-20, y-image_height+20);
		cairo_paint(cr);
	}
};

Bush bush1(0, WINDOW_HEIGHT);
Bush bush2(300, WINDOW_HEIGHT);
Bush bush3(600, WINDOW_HEIGHT);
Bush bush4(900, WINDOW_HEIGHT);
Bush bush5(1200, WINDOW_HEIGHT);

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
	Arrow():tip(0),isthrown(false),wait(0)
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
		if(isthrown)
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
					
                    message.set(HIT_MESSAGE[0]);
						global_ifhit = false;
                }
                else
                    message.set(MISS_MESSAGE[0]);
            }
			//display msg
			message.display(cr);
			//wait
			wait++;
			if(wait==20)
			{
                wait=0;
				isthrown = false;
				tip = -width;
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
	Balloon(int X,double UPMOVE_LENGTH,int SWIRL_THRESHOLD,const char* filename):y(WINDOW_HEIGHT + height/2),hit(false), wait(0), swirl(0), swirling(false)
	{
		x=X;
		upmove_length = UPMOVE_LENGTH;
        swirl_threshold=SWIRL_THRESHOLD;
		image = cairo_image_surface_create_from_png(filename);
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
			if(!wait)
            {
				total_hits++;
				global_ifhit = true;
				
			}
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
		x=random(WINDOW_WIDTH - 400,WINDOW_WIDTH - 100);
		upmove_length = random(1,5);
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
		 if((arrow.tip< x+width/2 && arrow.tip > x-width/2 ) && (arrow.y < y+height && arrow.y > y))
			hit = true;
	}
};
Balloon balloon(random(WINDOW_WIDTH - 400,WINDOW_WIDTH - 100),random(1,3),random(10,30),"pink.png");
Balloon balloon2(random(WINDOW_WIDTH - 400,WINDOW_WIDTH - 100),random(1,2),random(10,30),"pink.png");
Balloon balloon3(random(WINDOW_WIDTH - 400,WINDOW_WIDTH - 100),random(1,3),random(10,30),"pink.png");
Balloon balloon4(random(WINDOW_WIDTH - 400,WINDOW_WIDTH - 100),random(1,3),random(10,30),"pink.png");

class Cloud
{
public:
	cairo_surface_t *image;
	static const int height = 100;
	static const int width = 52;
	int y;//starting y
	int swirl;
	bool swirling;
	int swirl_threshold;
	bool isout;
	int x;//strating x
	double tip; //moving x coordinate of cloud
	double rightmove_length ; //speed

	//constructor
	Cloud(int a, int b, double speed):y(a), isout(true), tip(0), x(b), rightmove_length(speed)
	{
		swirl_threshold=0;
		image = cairo_image_surface_create_from_png("cloud.png");
		tip=x;
	}

	// when clicked set the y coordinate

	void draw(cairo_t *cr)
	{
		if(isout)
		{
			cairo_set_source_surface(cr, image, tip , y);
			cairo_paint(cr);
			move();
		}
	}
	void move()
	{
		if(tip < WINDOW_WIDTH + width)
		{
			tip += rightmove_length;
		}
		else
		{
			isout = true;
			tip = (x>0)?-100:x;
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
};

Cloud cloud1(100, 100, 0.5);
Cloud cloud2(40, -100, 0.3);
Cloud cloud3(120, 600, 0.42);
Cloud cloud4(60, -200, 0.6);
Cloud cloud8(150, 30, 0.32);
Cloud cloud7(170, 58, 0.57);

class Bird
{
public:
	cairo_surface_t *image1, *image2,*deadbird;
	static const int height = 100;
	int wait;
	static const int width = 52;
	int y;//starting y
	int swirl;
	bool swirling;
	int swirl_threshold;
	bool isout;
	bool image_switcher;
	int x;//strating x
	double tip; //moving x coordinate of bird
	double rightmove_length ; //speed
	bool hit;
	bool isdead;

	//constructor
	Bird(int a, int b, double speed):y(a), isout(true), tip(0), x(b),isdead(false) ,rightmove_length(speed),hit(false), image_switcher(true), wait(10)
	{
		swirl_threshold=30;
		image1 = cairo_image_surface_create_from_png("bird1.png");
		image2 = cairo_image_surface_create_from_png("bird2.png");
		deadbird = cairo_image_surface_create_from_png("deadbird.png");
		tip=x;
	}

	// when clicked set the y coordinate

	void draw(cairo_t *cr)
	{
		if(!hit)
		{
			if(isout)
			{
				if(image_switcher)
				{
					cairo_set_source_surface(cr, image1, tip , y+swirler());
					if(wait<=5)
						wait++;
					else
					{
						image_switcher=false;
						wait=0;
					}
				}
				else
				{
					cairo_set_source_surface(cr, image2, tip , y+swirler());
					if(wait<=5)
						wait++;
					else
					{
						image_switcher=true;
						wait=0;
					}
				}
				cairo_paint(cr);
				move();
			}
		}
		else 
		{
			cairo_set_source_surface(cr, deadbird, tip , y+swirler());
			cairo_paint(cr);
			drop();
		}
    }
    void drop()
    {
		if(y <= WINDOW_HEIGHT + height)
		{
			y += 5*rightmove_length;
		}
		else
		{
			isout=true;
			hit = false;
			y=100;
			tip = -100;
		}
	}
	void move()
	{
		if(tip < WINDOW_WIDTH + width)
		{
			tip += rightmove_length;
		}
		else
		{
			isout = true;
			tip = (x>0)?-100:x;
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
	void check_hit(const Arrow& arrow)
	{
		if((arrow.tip< tip+width/2 && arrow.tip > tip-width/2 ) && (arrow.y < y+height && arrow.y > y))
		{
			hit = true;
			global_birdhit = true;
		}
	}
};

Bird bird(100,0,1);

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

	g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL); //expose-event
	g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(clicked), NULL);
	gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK );
	g_signal_connect(window, "button-press-event", G_CALLBACK(clicked), NULL); // not working :(


	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
	gtk_window_set_title(GTK_WINDOW(window), "Arrow");
	gtk_widget_modify_bg(darea, GTK_STATE_NORMAL, &color);

	g_timeout_add(10, (GSourceFunc) time_handler, (gpointer) window);  //calls time_handler 10 ms apart

	gtk_widget_show_all(window);

	gtk_main();

	  return 0;
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	sun.draw(cr);
	
	cloud1.draw(cr);
	cloud2.draw(cr);
    cloud3.draw(cr);
    cloud4.draw(cr);
    cloud7.draw(cr);
    cloud8.draw(cr);
    arrow.draw(cr);
    
	balloon.draw(cr);
	balloon.check_hit(arrow);
	balloon2.draw(cr);
	balloon2.check_hit(arrow);
	balloon3.draw(cr);
	balloon3.check_hit(arrow);
	balloon4.draw(cr);
	balloon4.check_hit(arrow);
	
    
    
    bird.draw(cr);
    bird.check_hit(arrow);
    
    
    bush1.draw(cr);
	bush2.draw(cr);
	bush3.draw(cr);
	bush4.draw(cr);
	bush5.draw(cr);
	
	scoreboard.display(cr);
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
