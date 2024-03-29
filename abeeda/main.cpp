
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include "globalConst.h"
#include "tHMM.h"
#include "tAgent.h"

using namespace std;

//double replacementRate=0.1;
double perSiteMutationRate=0.005;
int update=1;
int repeats=1;
int maxAgent=1000;
int totalGenerations=100;
int successfull=0;

int xm[4]={0,1,0,-1};
int ym[4]={-1,0,1,0};

#define xDim 256
#define yDim 256
#define _empty 0
#define _agent 1
#define _food 2
#define _wall 3

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "helper.h"           /*  our own helper functions  */

#define ECHO_PORT          (2002)
#define MAX_LINE           (100000)

#define BUFLEN 512
#define NPACK 10
#define PORT 9930

int       list_s;                /*  listening socket          */
int       conn_s;                /*  connection socket         */
short int port;                  /*  port number               */
struct    sockaddr_in servaddr;  /*  socket address structure  */
char      buffer[MAX_LINE];      /*  character buffer          */
char     *endptr;                /*  for strtol()              */

void setupBroadcast(void);
void doBroadcast(string data);

int main(int argc, char *argv[])
{
	vector<tAgent*>agent,birth;
    tAgent* who[xDim][yDim];
    unsigned char area[xDim][yDim];
	tAgent *masterAgent;
	int i,j,x,y,action, numFood = 10;
	double deathAtBirthRate = 0.0;
    
	/*FILE *resFile;
	FILE *LOD;
	FILE *genomeFile;
    FILE *F,*G,*R;*/
    srand(getpid());
    //cout<<"attempt socked connection"<<endl;
    setupBroadcast();
    //cout<<"attempt complete"<<endl;
	srand(getpid());
    //setup area and all
    agent.clear();
    for (i=0; i<xDim; i++)
    {
        for(j=0;j<yDim;j++)
        {
            area[i][j]=_empty;
            who[i][j]=NULL;
            if(randDouble<0.01)
            {
                area[i][j]=_wall;
            }
            else
            {
                if(randDouble<0.8)
                {
                    area[i][j]=_food;
                }
            }
            
            if((i==0)||(j==0)||(i==xDim-1)||(j==yDim-1))
            {
                area[i][j]=_wall;
            }
        }
    }
	agent.resize(maxAgent);
	masterAgent=new tAgent;
	//masterAgent->setupRandomAgent(5000);
    masterAgent->loadAgent((char*)"startGenome.txt");
	masterAgent->setupPhenotype();
    //masterAgent->showPhenotype();
    //exit(0);
//    masterAgent->saveGenome(genomeFile);
//    fclose(genomeFile);
	for(i=0;i<agent.size();i++)
    {
		agent[i]=new tAgent;
		agent[i]->inherit(masterAgent,0.01,0);
        do
        {
            agent[i]->xPos=rand()%xDim;
            agent[i]->yPos=rand()%yDim;
            
        } while(area[agent[i]->xPos][agent[i]->yPos]!=_empty);
        agent[i]->direction=rand()&3;
        area[agent[i]->xPos][agent[i]->yPos]=_agent;
        who[agent[i]->xPos][agent[i]->yPos]=agent[i];
        //agent[i]->showPhenotype();
	}
	masterAgent->nrPointingAtMe--;
	cout<<"setup complete"<<endl;
    while(agent.size()>0 || birth.size()>0)
    //while(update<5000)    
    {
        update++;
        i=0;
        //birth.clear();
        
        double avgDormancyPeriod = 0.0;
        
        while(i<agent.size())
        {
            
            avgDormancyPeriod += agent[i]->dormancyPeriod;
            
            //death of an agent
            if (update - agent[i]->born > 100)
            //if (randDouble<0.001)
            {
                who[agent[i]->xPos][agent[i]->yPos]=NULL;
                area[agent[i]->xPos][agent[i]->yPos]=_empty; //_food;
                agent[i]->nrPointingAtMe--;
                if(agent[i]->nrPointingAtMe==0)
                    delete agent[i];
                
                agent.erase(agent.begin()+i);
            }
            else
            {
                //do agent
                //make inputs
                area[agent[i]->xPos][agent[i]->yPos]=_empty; //_food;
                who[agent[i]->xPos][agent[i]->yPos]=NULL;
                agent[i]->states[0]=agent[i]->direction&1;
                agent[i]->states[1]=(agent[i]->direction>>1)&1;
                
                agent[i]->states[2]=(area[agent[i]->xPos+xm[agent[i]->direction]][agent[i]->yPos+ym[agent[i]->direction]])&1;
                agent[i]->states[3]=(area[agent[i]->xPos+xm[agent[i]->direction]][agent[i]->yPos+ym[agent[i]->direction]]>>1)&1;
                agent[i]->states[4]=(area[agent[i]->xPos+xm[agent[i]->direction]][agent[i]->yPos+ym[agent[i]->direction]]>>2)&1;
                //update states
                agent[i]->updateStates();
                
                //evaluate actions
                action=((agent[i]->states[5]&1)<<2)+((agent[i]->states[6]&1)<<1)+(agent[i]->states[7]&1);
                //action=rand()&3;
                switch(action){
                    case 0: // nop
                        break;
                    case 1: // turn left
                        agent[i]->direction=(agent[i]->direction+1)&3;
                        break;
                    case 2: // turn right
                        agent[i]->direction=(agent[i]->direction-1)&3;
                        break;
                    case 3: //move forward
                        switch(area[agent[i]->xPos+xm[agent[i]->direction]][agent[i]->yPos+ym[agent[i]->direction]])
                        {
                            case _food:
                                
                                agent[i]->food++;
                                
                                if(agent[i]->food >= 5)
                                {
                                    tAgent *offspring=new tAgent();
                                    offspring->inherit(agent[i], 0.01, update);
                                    offspring->xPos=agent[i]->xPos;
                                    offspring->yPos=agent[i]->yPos;
                                    offspring->direction=rand()&3;
                                    agent[i]->food=0;
                                    agent[i]->offspring++;
                                    birth.push_back(offspring);
                                    area[offspring->xPos][offspring->yPos]=_agent;
                                    who[offspring->xPos][offspring->yPos]=offspring;
                                }
                                
                            case _empty:
                                agent[i]->xPos+=xm[agent[i]->direction];
                                agent[i]->yPos+=ym[agent[i]->direction];
                            break;
                                
                            default: //do nothing if not empty of food
                                break;
                        }
                        break;
                }
                area[agent[i]->xPos][agent[i]->yPos]=_agent;
                who[agent[i]->xPos][agent[i]->yPos]=agent[i];
                i++;
            }
        }
        
        // add newborns to population
        if (update % 100 == 0)
        {
            deathAtBirthRate = 0.0;
        }
        
        else if (update % 50 == 0)
        {
            deathAtBirthRate = 0.5;
        }
        
        for (int newbornIndex = 0; newbornIndex < birth.size(); ++newbornIndex)
        {
            if (update > birth[newbornIndex]->born + birth[newbornIndex]->dormancyPeriod)
            {
                if (randDouble > deathAtBirthRate)
                {
                    birth[newbornIndex]->born = update;
                    agent.insert(agent.end(), birth[newbornIndex]);
                }
                
                birth.erase(birth.begin() + newbornIndex);
            }
        }
        
        //agent.insert(agent.end(), birth.begin(), birth.end());
        //add food
        /*if (update % 50 == 0)
        {
            if (numFood == 10)
            {
                numFood = 0;
            }
            else
            {
                numFood = 10;
            }
        }*/
        
        for (int i = 0; i < numFood; ++i)
        {
            x=2+(rand()%(xDim-4));
            
            y=2+(rand()%(yDim-4));
            
            if(area[x][y]==_empty)
                area[x][y]=_food;
        }
        
        if((update&15)==0){
            //doBroadcast("a c string");
            string S;
            char line[100];
            for(i=0;i<xDim;i++)
                for(j=0;j<yDim;j++)
                    if((area[i][j]&3)!=_empty){
                        switch(area[i][j]&3){
                            case _agent: sprintf(line,"%i,%i,%i,%i,%i;",i,j,250,50,50); break;
                            case _food: sprintf(line,"%i,%i,%i,%i,%i;",i,j,50,255,70); break;
                            case _wall: sprintf(line,"%i,%i,%i,%i,%i;",i,j,128,128,128); break;
                        }
                        S.append(line);
                    }
            doBroadcast(S);
        }
        
        
        avgDormancyPeriod /= (double)agent.size();
        
        if((update%250)==0)
        {
            cout<<update<<" "<<(int)agent.size()<<" "<<avgDormancyPeriod<<endl;
        }
    }
    /*
    F=fopen("startGenome.txt","w+t");
    agent[0]->saveGenome(F);
    fclose(F);
    */
    return 0;
}


void setupBroadcast(void)
{
    port = ECHO_PORT;
	if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error creating listening socket.\n");
		//exit(EXIT_FAILURE);
    }
	/*  Set all bytes in socket address structure to
	 zero, and fill in the relevant data members   */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);
	/*  Bind our socket addresss to the 
	 listening socket, and call listen()  */
    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error calling bind()\n");
		exit(EXIT_FAILURE);
    }
    if ( listen(list_s, LISTENQ) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error calling listen()\n");
		exit(EXIT_FAILURE);
    }
}

void doBroadcast(string data)
{
    if ( (conn_s = accept(list_s, NULL, NULL) ) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling accept()\n");
        //exit(EXIT_FAILURE);
    }
    Writeline(conn_s, data.c_str(), data.length());
    
    if ( close(conn_s) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling close()\n");
        //exit(EXIT_FAILURE);
    }
}
