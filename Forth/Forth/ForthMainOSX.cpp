//
//  main.cpp
//  Forth
//
//  Created by Pat McElhatton on 3/28/17.
//  Copyright © 2017 Pat McElhatton. All rights reserved.
//

#include <iostream>
#include "Forth.h"
#include "ForthShell.h"

int main(int argc, const char * argv[])
{
    int nRetCode = 0;
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;
    
    /*
     if ( !InitSystem() )
    {
        nRetCode = 1;
    }
    else*/
    {
        nRetCode = 1;
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
    }
    
    return nRetCode;
}



