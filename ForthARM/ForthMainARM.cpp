// ForthMain.cpp : Defines the entry point for the console application.
//

#include "StdAfx.h"
#include "ForthShell.h"
#include "ForthInput.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
 
static struct termios oldTermSettings;

// kbhit from http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
static void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    newt = oldTermSettings;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
  {
    tcsetattr( STDIN_FILENO, TCSANOW, &oldTermSettings);
  }
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  changemode( 1 ); 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  int result = FD_ISSET(STDIN_FILENO, &rdfs);
  changemode( 0 ); 
  return result;
}

// getch from http://stackoverflow.com/questions/7469139/what-is-equivalent-to-getch-getche-in-linux
int getch()
{
    char buf = 0;
    changemode( 0 ); 
    struct termios oldSettings = {0};
    struct termios newSettings = {0};
    fflush(stdout);
    
    if ( tcgetattr(0, &oldSettings) < 0 )
	{
        perror("tcgetattr()");
    }
    if ( tcgetattr(0, &newSettings) < 0 )
	{
        perror("tcgetattr()");
    }
    newSettings.c_lflag &= ~(ICANON | ECHO);
    newSettings.c_cc[VMIN] = 1;
    newSettings.c_cc[VTIME] = 0;
    if ( tcsetattr(0, TCSANOW, &newSettings) < 0 )
    {
        perror("tcsetattr ICANON");
    }
    if(read(0,&buf,1)<0)
    {
        perror("read()");
    }
    //old.c_lflag |= (ICANON | ECHO);
    if ( tcsetattr(0, TCSADRAIN, &oldSettings) < 0 )
	{
        perror ("tcsetattr ~ICANON");
    }
    changemode( 0 );
    return (int) buf;
}


using namespace std;


int main(int argc, char* argv[] )
{
    int nRetCode = 0;
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;

    tcgetattr( STDIN_FILENO, &oldTermSettings);
    
    pShell = new ForthShell;
    pShell->SetCommandLine( argc, (const char **) (argv));
    //pShell->SetEnvironmentVars( (const char **) envp );
#if 0
    if ( argc > 1 )
    {

        //
        // interpret the forth file named on the command line
        //
        FILE *pInFile = fopen( argv[1], "r" );
        if ( pInFile != NULL )
        {
            pInStream = new ForthFileInputStream(pInFile);
            nRetCode = pShell->Run( pInStream );
            fclose( pInFile );

        }
    }
    else
#endif
    {

        //
        // run forth in interactive mode
        //
        pInStream = new ForthConsoleInputStream;
        nRetCode = pShell->Run( pInStream );

    }
    delete pShell;

    return nRetCode;
}

