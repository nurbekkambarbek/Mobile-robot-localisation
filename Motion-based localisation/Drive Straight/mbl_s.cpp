#include "Aria.h"
#include "ArActionAvoidFront.h"
#include "ariaUtil.h"
#include "ArRobot.h"
#include <iostream>
#include <fstream>
#include <cmath>
#define pb push_back
using namespace std;
	double vr, vl, Es_x=0, Es_y=0, Es_th=0;
	double x_g=5000.0, y_g=5000.0;	
	double me_x=0, me_y=0, me_th=0;
	double X_true, Y_true, Th_true, dt=0.1;
	double S0, S1, S2, S3, S4, S5, S6, R=50, D=100, Wb=280;
	double pos_cov[3][3]={{1e-6, 0, 0}, {0, 1e-6, 0}, {0, 0, 1e-6}};	//Sigma or Sum
	double delta_v=0, delta_w=0, dl, dr, Q[3][3], d_sigma[2][2];
	double C[3][2], f[2][3], h[3][3], c[3][3];	
	ofstream mbl_Straight("mbl_Straight.txt");
	ofstream mbl_Straight_cov("mbl_Straight_cov.txt");
	int cnt=0;
bool handleDebugMessage(ArRobotPacket *pkt)
{
  	if(pkt->getID() != ArCommands::MARCDEBUG) return false;
  	char msg[256];
  	pkt->bufToStr(msg, sizeof(msg));
  	msg[255] = 0;
  	ArLog::log(ArLog::Terse, "Controller Firmware Debug: %s", msg);
  	return true;
}

class SonarMonitor{
private:
  	ArRobot* myRobot;
  	int mySonarNum;
  	ArTime myTimer;
  	ArFunctorC<SonarMonitor> myCB;

void gVel(int V, int W){
	vr=(2.0*V+W*Wb*1.0)/(2.0*R);
	vl=(2.0*V-W*Wb*1.0)/(2.0*R);
}


void dead_reckon(){
	delta_v=dt*myRobot->getVel();									//changes in linear velocity
	delta_w=dt*(myRobot->getRotVel()*3.1415/180);					//changes in angular velocity

	dr=dt*vr, dl=dt*vl;												//distances travelled by right and left wheels consequently
	
	double dif_x, dif_y, dif_th;
	dif_x=Es_x-me_x;												// s(k-1) - mu(k-1)
	dif_y=Es_y-me_y;
	dif_th=Es_th-me_th;
	
	double H[3][3]={{1, 0, -delta_v*sin(me_th)},					//H is Jacobian matrix
					{0, 1,  delta_v*cos(me_th)}, 
					{0, 0, 				1 	  }};

	me_x=Es_x+(delta_v*cos(Es_th));									
	me_y=Es_y+(delta_v*sin(Es_th));									//linearized motion model updated
	me_th=Es_th+delta_w;											
	

	Es_x=me_x+dif_x-delta_v*sin(me_th)*dif_th;
	Es_y=me_y+dif_x-delta_v*cos(me_th)*dif_th;						//update pose
	Es_th=me_th+dif_th;

	dr=abs(dr), dl=abs(dl);											//only positive for calculating the robot motion covariance
	double Kq=10; 
	d_sigma[0][0]=Kq*dr; 											//finding this covariance with tuning parameter "Kq"
	d_sigma[1][1]=Kq*dl;
	double d_s=(dr+dl)/2, d_th=(dr-dl)/Wb;	
	double FF[3][2]={{0.5*cos(Es_th+d_th/2 - d_s/(2*Wb)*sin(Es_th+d_th/2)), 0.5*cos(Es_th+d_th/2 + d_s/(2*Wb)*sin(Es_th+d_th/2))},
					 {0.5*sin(Es_th+d_th/2 + d_s/(2*Wb)*cos(Es_th+d_th/2)), 0.5*sin(Es_th+d_th/2 - d_s/(2*Wb)*cos(Es_th+d_th/2))}, 				         {1.0/Wb,								-1.0/Wb			}};
	
	for(int i=0;i<3;i++){
        for(int j=0;j<2;j++){
            f[j][i]=FF[i][j];										//transposing F2
        }
    }
	memset(C, 0, sizeof(C));
  	for(int i=0;i<3;i++){
    	for(int j=0;j<2;j++){
       		for(int k=0;k<2;k++){
          		C[i][j]+=FF[i][k]*d_sigma[k][j]; 					// C=F2*Sig
       		}
    	}
  	}
	memset(Q, 0, sizeof(Q));
  	for(int i=0;i<3;i++){
    	for(int j=0;j<3;j++){
   			for(int k=0;k<2;k++){
      			Q[i][j]+=C[i][k]*f[k][j];							//Q matrix was generated! Q=F2*Sig*F2'
      		}
    	}
  	}
	//lets find robot pose covariance
	
	double h[3][3];
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			h[j][i]=H[i][j];										//transpose of Jacobian H
		}
	}
	double CC[3][3];
	memset(CC, 0, sizeof(CC));
	for(int i=0;i<3;i++){
  		for(int j=0;j<3;j++){
    		for(int k=0;k<3;k++){
   	   			CC[i][j]+=H[i][k]*pos_cov[k][j];					// CC=H*pos_cov;
    		}
  		}
  	}
	memset(pos_cov, 0, sizeof(pos_cov));
  	for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            for(int k=0;k<3;k++){
                pos_cov[i][j]+=CC[i][k]*h[k][j];					//pos_cov matrix was generated!
            }
        }
  	}
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			pos_cov[i][j]+=Q[i][j];									//And now "pos_cov= H * pos_cov * H[T] + Q"
		}	
	}
	printf("%.3f", Es_x);
	printf("%s%.3f\n","    ", Es_y);
}
void drive_straight(){
	printf("%d\n", myTimer.mSecSince());
	if(mbl_Straight.is_open()){	
		mbl_Straight<<Es_x<<"   "<<Es_y<<"    ";								//save data in file to draw path in MATLAB
	}
	if(mbl_Straight.is_open()){	
		mbl_Straight<<myRobot->getVel()<<"  "<<myRobot->getRotVel()<<endl;	//save data in file to draw path in MATLAB
	}
	myRobot->setVel(200);	
	cout<<"vel = "<<myRobot->getVel()<<endl;
	
	int x=int(myTimer.mSecSince()/2000);
	cout<<x<<' '<<cnt<<' '<<endl;	
	if(x-cnt!=0){
		cnt=x;
		if(mbl_Straight_cov.is_open()){	
			mbl_Straight_cov<<Es_x<<' '<<Es_y<<' '<<0<<endl;
			for(int i=0;i<3;i++){
				for(int j=0;j<3;j++){
					mbl_Straight_cov<<pos_cov[i][j]<<" ";
				}
				mbl_Straight_cov<<endl;
			}			
		}
	}
	if(myTimer.mSecSince() > 10000){
		myRobot->stopRunning();
	}
}

public:
  SonarMonitor(ArRobot* r, int s) : 
    myRobot(r), mySonarNum(s), myCB(this, &SonarMonitor::cycleCallback){
    ArLog::log(ArLog::Normal, "Sonar Monitor: Monitoring sonar transducer #%d", s);
    myTimer.setToNow();
	myRobot->addSensorInterpTask("SonarMonitor", 1, &myCB);
  }
  void cycleCallback(){
	double v=myRobot->getVel(), w=myRobot->getRotVel();
	gVel(v, w);
	dead_reckon();
	drive_straight();		
  }
};

int main(int argc, char** argv){
  // Initialize some global data
	
  Aria::init();
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();
  ArRobot robot;
  ArRobotConnector robotConnector(&parser, &robot);
  ArAnalogGyro gyro(&robot);
  robot.addPacketHandler(new ArGlobalRetFunctor1<bool, ArRobotPacket*>(&handleDebugMessage));
  if (!robotConnector.connectRobot()){
   	if (!parser.checkHelpAndWarnUnparsed()){
    	ArLog::log(ArLog::Terse, "Could not connect to robot");
    }
   	else{
   		ArLog::log(ArLog::Terse, "Error, could not connect to robot.");
   		Aria::logOptions();
   		Aria::exit(1);
   	}
  }
  if(!robot.isConnected()){
    ArLog::log(ArLog::Terse, "Internal error: isConnected() is false!");
  }
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed()){    
    Aria::logOptions();
    Aria::exit(1);
    return 1;
  }
  ArSonarDevice sonarDev;
  SonarMonitor sonarMon(&robot, 2); //front
  robot.addRangeDevice(&sonarDev);
  robot.runAsync(true);
  robot.lock();
  robot.comInt(ArCommands::ENABLE, 1);
  robot.unlock();
  robot.waitForRunExit();
  Aria::exit(0);
  return 0;
}

