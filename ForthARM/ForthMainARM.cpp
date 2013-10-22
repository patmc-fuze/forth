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
 
// from http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
static void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
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
  return FD_ISSET(STDIN_FILENO, &rdfs);
}

// from http://stackoverflow.com/questions/7469139/what-is-equivalent-to-getch-getche-in-linux
int getch()
{
    char buf=0;
    changemode( 0 ); 
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
        perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        perror ("tcsetattr ~ICANON");
    return (int) buf;
}


using namespace std;


int main(int argc, char* argv[] )
{
    int nRetCode = 0;
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;

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
