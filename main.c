#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>


enum
{
	rows 	= 24,  // divided by 2 and 3 for good screen ratio, 24 is optimum
	columns = rows * 2,
	roadyh 	= rows,
	roadyw 	= columns/3,
	roadxh 	= rows/3,
	roadxw 	= columns,
	pass1y  = rows/3,
	pass2y  = 2*rows/3,
	pass1x 	= columns/3,
	pass2x	= 2*columns/3,
	carspeed = 100000,
	pedestrianspeed = 150000,
	buswait = carspeed*20,
	carsonroad = 5,
	stationx = columns*2/3+1,
	stationy = rows*2/3+4,

};


bool occTable[columns][rows];
int northmen = 0;

pthread_mutex_t pass1ns;
pthread_mutex_t pass1sn;
pthread_mutex_t updown;
pthread_mutex_t occupation;
pthread_mutex_t draw;
pthread_mutex_t leftright;
pthread_mutex_t cross;
pthread_mutex_t refmutex;
int north=0, south=0, east=0, west=0;


void printCross()
{

	pthread_mutex_lock(&refmutex);
	int i = 0;
	char *bckgr = calloc(roadyw, sizeof(bckgr));
	memset(bckgr, '0', sizeof(char) * columns);
	attron(COLOR_PAIR(2));
	for(i = roadxh; i < 2*roadxh; i++)
	{
		mvprintw(i, 0, bckgr);
	}

	char *road = calloc(roadyw, sizeof(road));
	memset(road, '0', sizeof(char) * roadyw);

	for(i = 0; i < rows; i++)
	{
		mvprintw(i, roadyw, road);
	}
	attroff(COLOR_PAIR(2));
	refresh();
	pthread_mutex_unlock(&refmutex);
}


void print()
{

	pthread_mutex_lock(&refmutex);
	char * bckgr = calloc(columns, sizeof(bckgr));
	memset(bckgr, 'g', sizeof(char) * columns);
	//mvprintw(2, 2, bckgr);

	attron(COLOR_PAIR(1));
	int i = 0;
	//print green
	for(i = 0; i < rows; i++)
	{
		printw(bckgr);
		printw("\n");
	}
	attroff(COLOR_PAIR(1)); /* turn color off */

	pthread_mutex_unlock(&refmutex);
	printCross();
}

void printCar(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(10));
	mvprintw(y-1, x-1,  "  ");
	attroff(COLOR_PAIR(3));
	pthread_mutex_unlock(&draw);
}

void waitCar(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(5));
	mvprintw(y-1, x-1,  "WW");
	attroff(COLOR_PAIR(5));
	pthread_mutex_unlock(&draw);
}

void clearCar(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(2));
	mvprintw(y-1,x-1, "  ");
	attroff(COLOR_PAIR(2));
	pthread_mutex_unlock(&draw);
}


/*the three functions below handle with occupation table, defined for field occupance*/
void busy(int x, int y)
{
	if((x>0)&&(x<columns)&&(y>0)&&(y<rows)) occTable[x][y] = 1;
}

void release(int x, int y)
{
	if((x>0)&&(x<columns)&&(y>0)&&(y<rows)) occTable[x][y] = 0;
}

bool isBusy(int x, int y)
{
	return occTable[x][y];
}


/*all cars and bus function looks the same - the difference is the start point*/
void * carns(void *arg)
{
	int xpos = columns/2 - roadyw/4;
	int ypos = 0;

	north++;

	for(ypos; ypos <=rows; ypos++)
	{
		while(isBusy(xpos,ypos+1))
		{
			usleep(1000);
			waitCar(xpos,ypos-1);
		}

		pthread_mutex_lock(&occupation);
		release(xpos,ypos-1);
		busy(xpos,ypos);
		pthread_mutex_unlock(&occupation);

		if(ypos == pass1y-1)
		{
			    clearCar(xpos,ypos-1);
				waitCar(xpos,ypos);
				pthread_mutex_lock(&pass1ns);
		}

		if(ypos == pass1y)
		{
			clearCar(xpos,ypos-1);
			waitCar(xpos, ypos);
			pthread_mutex_lock(&leftright);
		}

		if(ypos == pass2y)
		{
			pthread_mutex_unlock(&leftright);
		}


		if(ypos == pass1y+1)
		{
				pthread_mutex_unlock(&pass1ns);
		}

		printCar(xpos,ypos);
		clearCar(xpos,ypos-1);
		usleep(carspeed);

		pthread_mutex_lock(&refmutex);
		refresh();
		pthread_mutex_unlock(&refmutex);
	}

	clearCar(xpos,ypos-1);

	pthread_mutex_lock(&refmutex);
	refresh();
	pthread_mutex_unlock(&refmutex);

	north--;
	return (NULL);

}

void * carsn(void *arg)
{

	int xpos = columns/2 + roadyw/4;
	int ypos = rows-1;

	south++;

	for(ypos; ypos >=-1; ypos--)
	{
		while(isBusy(xpos,ypos-1))
		{
			usleep(1000);
			waitCar(xpos,ypos+1);
		}
		pthread_mutex_lock(&occupation);
		release(xpos,ypos+1);
		busy(xpos,ypos);
		pthread_mutex_unlock(&occupation);


		if(ypos == pass1y+1)
		{
			clearCar(xpos,ypos+1);
			waitCar(xpos,ypos);
			pthread_mutex_lock(&pass1sn);
		}
		if(ypos == pass1y-1)
		{
			pthread_mutex_unlock(&pass1sn);
		}

		if(ypos == pass2y)
		{
			clearCar(xpos,ypos+1);
			waitCar(xpos, ypos);
			pthread_mutex_lock(&leftright);
		}

		if(ypos == pass1y)
		{
			pthread_mutex_unlock(&leftright);
		}

		printCar(xpos,ypos);
		clearCar(xpos,ypos+1);
		usleep(carspeed);
		pthread_mutex_lock(&refmutex);
		refresh();
		pthread_mutex_unlock(&refmutex);
	}

	south--;

	return (NULL);

}

void * carew(void *arg)
{
	int xpos = 0;
	int ypos = rows/2 - roadxh/4;

	east++;

	for(xpos = columns-3; xpos >=-1; xpos--)
	{

		while(isBusy(xpos-3,ypos))
		{
			usleep(1000);
			waitCar(xpos+1,ypos);
		}
		pthread_mutex_lock(&occupation);
		release(xpos+1,ypos);
		busy(xpos,ypos);
		pthread_mutex_unlock(&occupation);


		if(xpos == pass2x)
		{
				clearCar(xpos+1,ypos);
				waitCar(xpos, ypos);
				pthread_mutex_lock(&leftright);
		}

		if(xpos == pass1x)
		{
			pthread_mutex_unlock(&leftright);
		}

		printCar(xpos,ypos);
		clearCar(xpos+2,ypos);
		usleep(carspeed);
		pthread_mutex_lock(&refmutex);
		refresh();
		pthread_mutex_unlock(&refmutex);
	}
	clearCar(xpos+2,ypos);
	pthread_mutex_lock(&refmutex);
	refresh();
	pthread_mutex_unlock(&refmutex);

	east--;

	return (NULL);

}

void * carwe(void *arg)
{
	int xpos = 0;
	int ypos = rows/2 + roadxh/4;

	west++;

	for(xpos = -1; xpos <columns; xpos++)
	{
		while(isBusy(xpos+3,ypos))
		{
			usleep(1000);
			waitCar(xpos-1,ypos);
		}
		pthread_mutex_lock(&occupation);
		release(xpos-1,ypos);
		busy(xpos,ypos);
		pthread_mutex_unlock(&occupation);

		if(xpos == pass1x)
		{
				clearCar(xpos-1,ypos);
				waitCar(xpos, ypos);
				pthread_mutex_lock(&leftright);
		}

		if(xpos == pass2x)
		{
			pthread_mutex_unlock(&leftright);
		}

		printCar(xpos,ypos);
		clearCar(xpos-2,ypos);
		usleep(carspeed);
		pthread_mutex_lock(&refmutex);
		refresh();
		pthread_mutex_unlock(&refmutex);
	}
	clearCar(xpos-1,ypos);
	pthread_mutex_lock(&occupation);
	release(xpos-1,ypos);
	pthread_mutex_unlock(&occupation);
	pthread_mutex_lock(&refmutex);
	refresh();
	pthread_mutex_unlock(&refmutex);

	west--;

	return (NULL);

}


void printPedestrian(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(4));
	mvprintw(y-1, x-1, "p");
	attroff(COLOR_PAIR(4));
	pthread_mutex_unlock(&draw);

}

/*show that pedestrian waits on mutex - changing color to yellow*/
void waitPedestrian(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(5));
	mvprintw(y-1, x-1,  "P");
	attroff(COLOR_PAIR(5));
	pthread_mutex_unlock(&draw);
}

/*function that clears the previous field of pedestrian*/
void clearPedestrian(int x, int y)
{
	pthread_mutex_lock(&draw);
	attron(COLOR_PAIR(2));
	mvprintw(y-1, x-1, "p");
	attroff(COLOR_PAIR(2));
	pthread_mutex_unlock(&draw);

}


/*thread to print pedestrian in the north*/
void * northman(void * arg)
{
	northmen = 1;
	int xpos = columns/3+2;
	int ypos = rows/3;
	for(xpos; xpos <=columns/3 + roadyw; xpos++)
	{
		if(xpos == columns/3+2)
		{
			clearPedestrian(xpos-1,ypos);
			waitPedestrian(xpos,ypos);

			pthread_mutex_lock(&pass1ns);
			pthread_mutex_lock(&pass1sn);
		}

		printPedestrian(xpos,ypos);
		clearPedestrian(xpos-1,ypos);
		usleep(pedestrianspeed);

		pthread_mutex_lock(&refmutex);
		refresh();
		pthread_mutex_unlock(&refmutex);
	}

	clearPedestrian(xpos-1,ypos);
	refresh();

	pthread_mutex_unlock(&pass1ns);
	pthread_mutex_unlock(&pass1sn);

	northmen = 0;
	return (NULL);

}

void * drawingThread(void * arg)
{
	int end = 1;
	while(end != 1)
	{
		refresh();
		usleep(1000);
	}
	return (NULL);
}

int main(int argc, char*argv[])
{

	/*Init all needed mutex*/
	pthread_mutex_init(&refmutex,NULL);
	pthread_mutex_init(&updown,NULL);
	pthread_mutex_init(&leftright,NULL);
	pthread_mutex_init(&occupation,NULL);
	pthread_mutex_init(&pass1ns,NULL);
	pthread_mutex_init(&pass1sn,NULL);
	pthread_mutex_init(&draw,NULL);
	initscr();


	int debug = 0;	//set only to see the fields of ncurses "debug mode"
	start_color();

	/*assign color pairs*/
	if(debug)
	{
	init_pair(1, COLOR_BLACK, COLOR_GREEN); /* create foreground / background combination */
	init_pair(2, COLOR_BLACK, COLOR_WHITE); /* create foreground / background combination */
	init_pair(3, COLOR_BLACK, COLOR_BLACK); /* create foreground / background combination */
	init_pair(4, COLOR_WHITE, COLOR_BLACK); /* create foreground / background combination */
	init_pair(5, COLOR_BLACK, COLOR_YELLOW); /* create foreground / background combination */
	init_pair(6, COLOR_BLACK, COLOR_RED); /* create foreground / background combination */
	}
	else
	{
		init_pair(0, COLOR_WHITE, COLOR_WHITE); /* create foreground / background combination */
		init_pair(1, COLOR_GREEN, COLOR_GREEN); /* create foreground / background combination */
		init_pair(2, COLOR_WHITE, COLOR_WHITE); /* create foreground / background combination */
		init_pair(3, COLOR_BLACK, COLOR_BLACK); /* create foreground / background combination */
		init_pair(4, COLOR_WHITE, COLOR_BLACK); /* create foreground / background combination */
		init_pair(5, COLOR_BLACK, COLOR_YELLOW); /* create foreground / background combination */
		init_pair(6, COLOR_WHITE, COLOR_RED); /* create foreground / background combination */

	}
	print();
	int tids[30];
    int i = 1;
    int running = 1;

    /*set to hide the cursor during getch() function*/
    noecho();

	char k;
	while((k=getch())!='q')
	{
		usleep(carspeed);
		switch(k)
		{
			case 'w':  if(south<carsonroad) pthread_create(&tids[i++], NULL, carsn,NULL);
			break;
			case 'a':  if(east<carsonroad) pthread_create(&tids[i++], NULL, carew,NULL);
			break;
			case 's':  if(north<carsonroad) pthread_create(&tids[i++], NULL, carns,NULL);
			break;
			case 'd':  if(west<carsonroad) pthread_create(&tids[i++], NULL, carwe,NULL);
			break;
			case 'f':  if(northmen==0) pthread_create(&tids[i++], NULL, northman,NULL);
			break;
		}
	}

	endwin();			/* End curses mode		  */
	return 0;
}
