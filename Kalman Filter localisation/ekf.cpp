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
	ofstream mbl("mbl.txt");
	ofstream mbl_cov("mbl_cov.txt");
	ofstream ekf("ekf.txt");
	ofstream ekf_cov("ekf_cov.txt");
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

void EKF(){
	double Lx=4000, Ly=2000;
	double Gk[2][3]={{ -1,  0,  0 },
					 {  0, -1,  0 }};
	
	double Gkt[3][2];
	for(int i=0;i<2;i++){
        for(int j=0;j<3;j++){
            Gkt[j][i]=Gk[i][j];										//transposing Gk
        }
    }	

	double RR[2][2]={{1e-4, 0}, {0, 0}};


	double EC[2][3]={{0, 0, 0}, {0, 0, 0}}, Zk[2][2]={{0, 0}, {0, 0}};
	for (int i = 0; i < 2; i++){
        for (int j = 0; j < 3; j++){
            for (int k = 0; k < 3; k++){
                EC[i][j] += (Gk[i][k] * pos_cov[k][j]);				// EC=Zk=Gk*Pos_cov
            }
        }
    }
	for (int i = 0; i < 2; i++){
        for (int j = 0; j < 2; j++){
            for (int k = 0; k < 3; k++){
                Zk[i][j] += (EC[i][k] * Gkt[k][j]);					// Zk=Gk*Pos_cov*Gk_T
            }
        }
    }
	for(int i=0;i<2;i++){
		for(int j=0;j<2;j++){
			Zk[i][j]+=RR[i][j];										//Zk=Gk*Pos_cov*Gk_T+RR;
		}
	}
	

	double ECC[3][2]={{0, 0}, {0, 0}, {0, 0}}, Kk[3][2]={{0, 0}, {0, 0}, {0, 0}};	
	for (int i = 0; i < 3; i++){
        for (int j = 0; j < 2; j++){
            for (int k = 0; k < 3; k++){
                ECC[i][j] += (pos_cov[i][k] * Gkt[k][j]);			// EC=Kk=Pos_cov*Gk_T
            }
        }
    }
	
	double Zki[2][2]={{Zk[1][1], -Zk[0][1]}, 
					  {-Zk[1][0], Zk[0][0]}};
	double uu=Zk[0][0]*Zk[1][1]-Zk[0][1]*Zk[1][0];
	for(int i=0;i<2;i++){
		for(int j=0;j<2;j++){
			Zki[i][j]=Zki[i][j]/uu;									// Zki = Inverse of Zk	
		}
	}
	
	for (int i = 0; i < 3; i++){
        for (int j = 0; j < 2; j++){
            for (int k = 0; k < 2; k++){
                Kk[i][j] += (ECC[i][k] * Zki[k][j]);				// Kk=Pos_cov*Gk_T*Zk_inv
            }
        }
    }
	
	double z_sen_d=95.82+myRobot->getSonarRange(2)*0.978;
	double z_sen_th=120+myRobot->getSonarRange(5);					// landmark pose by sensor
	
	double z_real_d=Lx-Es_x;
	double z_real_th=Ly-Es_y;										// landmark pose real
	double z[2][1];

	
	cout<<"z_senX = "<<z_sen_d<<"      "<<"z_senY = "<<z_sen_th<<endl;
	//cout<<"z_realX = "<<z_real_d<<"      "<<"z_realY = "<<z_real_th<<endl;
	z[0][0]=z_sen_d - z_real_d;
	z[1][0]=z_sen_th - z_real_th;
	//cout<<"z_senX = "<<z_sen_d<<"      "<<"z_senY = "<<z_sen_th<<endl;
	//cout<<"z_realX = "<<z_real_d<<"      "<<"z_realY = "<<z_real_th<<endl;	 
	
	Es_x=Es_x + Kk[0][0]*z[0][0]+Kk[0][1]*z[1][0];
	Es_y=Es_y + Kk[1][0]*z[0][0]+Kk[1][1]*z[1][0];						// position is updated
	Es_th=Es_th +Kk[2][0]*z[0][0]+Kk[2][1]*z[1][0];
	
	double KG[3][3]={{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			for(int k=0;k<2;k++){
				KG[i][j]=-Kk[i][k] * Gk[k][j];						// KG=I-Kk * Gk
			}
		}
	}	
	double IKG[3][3]={{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			for(int k=0;k<3;k++){
				IKG[i][j] += KG[i][k] * pos_cov[k][j]; 				//IKG=pos_cov=(I - Kk*Gk) * Pos_cov;
			}
		}
	}
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			pos_cov[i][j]=IKG[i][j];								//update the Pos_cov;
		}
	}
	//printf("%s%.3f","X_estd = ", Es_x);
	//printf("%s%.3f\n","    Y_estd = ", Es_y);
	if(ekf.is_open()){	
		ekf<<Es_x<<"   "<<Es_y<<endl;								//save data in file to draw path in MATLAB		
	}
	

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
	double Kq=1; 
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
	if(mbl.is_open()){	
		mbl<<Es_x<<"   "<<Es_y<<"    ";								//save data in file to draw path in MATLAB
	}
	if(mbl.is_open()){	
		mbl<<myRobot->getVel()<<"  "<<myRobot->getRotVel()<<endl;	//save data in file to draw path in MATLAB
	}
}
void drive_straight(){
	printf("%d\n", myTimer.mSecSince());
	myRobot->setVel(200);	
	if(myTimer.mSecSince() > 10000){
		if(ekf_cov.is_open()){	
			for(int i=0;i<3;i++){
				for(int j=0;j<3;j++){
					if(i==j){
						pos_cov[i][i]=abs(pos_cov[i][i]);					
					}
					ekf_cov<<pos_cov[i][j]<<" ";
				}
				ekf_cov<<endl;
			}			
		}
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
	EKF();
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

