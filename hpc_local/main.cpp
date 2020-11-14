#include "mpi.h"
#include <iostream>
#include <random>
#include <string>
#include "particleFilter2D.h"


#define debugging


MPI_Datatype createWeightType(){
    MPI_Datatype new_type;
    int count = 3;
    int blocklens[] = { 1,1,1 };
    MPI_Aint indices[3];
    indices[0] = (MPI_Aint)offsetof(class detailWeights, nodeId);
    indices[1] = (MPI_Aint)offsetof(class detailWeights, particleId);
    indices[2] = (MPI_Aint)offsetof(class detailWeights, weight);
    MPI_Datatype old_types[] = {MPI_INT, MPI_INT, MPI_DOUBLE};
    MPI_Type_create_struct(count,blocklens,indices,old_types,&new_type);
    MPI_Type_commit(&new_type);
    return new_type;
}

MPI_Datatype createRouteType(){
    MPI_Datatype new_type;
    int count = 6;
    int blocklens[] = { 1,1,1,1,1,1 };
    MPI_Aint indices[3];
    indices[0] = (MPI_Aint)offsetof(class route, sourceId);
    indices[1] = (MPI_Aint)offsetof(class route, sParticleId);
    indices[2] = (MPI_Aint)offsetof(class route, NumtoSend);
    indices[3] = (MPI_Aint)offsetof(class route, desId);
    indices[4] = (MPI_Aint)offsetof(class route, dParticleId);
    indices[5] = (MPI_Aint)offsetof(class route, NumtoRec);
    MPI_Datatype old_types[] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(count,blocklens,indices,old_types,&new_type);
    MPI_Type_commit(&new_type);
    return new_type;
}

const double precentageLocalTransPars = 0.2;    // Locally resampling, particles transferring precentages.

//const int timeInterval = 5;
//const double adaptThres = 0.2;  //0.1 results in a larger RMES. 0.5 -> RMSE:1.59862.
//const int NumofparticlesPerCore = 20;



int main(int argc, char** argv) {
    
    MPI_Status stat;
    int myRank, numProcs, ier;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
//    MPI_Datatype MPI_pair = createRecType();
//    int procSize = numberOfParticles/numProcs;


    MPI_Datatype MPI_pair;
    MPI_Type_contiguous(2, MPI_INT, &MPI_pair);
    MPI_Type_commit(&MPI_pair);

    MPI_Datatype MPI_Route;
    MPI_Type_contiguous(6, MPI_INT, &MPI_Route);
    MPI_Type_commit(&MPI_Route);
//    MPI_Datatype MPI_Route = createRouteType();

    MPI_Datatype MPI_WEI = createWeightType();
    int NumofparticlesPerCore = std::stoi(argv[1]);

    double startTimer, endTimer,  totalTime;

//int jk = std::stoi(argv[1]);
//            std::cout<<jk<<", "<<std::endl;
//    }

//
//    std::cout << "Hello" << std::endl;


    
    //----------- read frame from txt files --------------
    std::string outPutFoldername = "../inputVideoOutputTxt/";
    bool finishYesNo = true;
    std::vector<std::vector<pix> > RGBFrame;
    int index = 0;
    std::string fullFileName = outPutFoldername + std::to_string(index) + ".txt";
//    std::cout<< fullFileName <<std::endl;
    readRGBtxt(fullFileName, RGBFrame);
    
    std::vector<std::vector<pix> > RGBFace;
    int x=320, y=183, width=190, height=70; // for movingDucks.mov.
    //    cout<<"pass 0"<<endl;
    
    makeFace(RGBFrame, RGBFace, x, y, width, height);
    
    //    cout<<"pass 1"<<endl;
    //    Mat frame;
    //    RGBToMat(RGBFace, frame);
    //    imshow("xudong", frame);
    //    waitKey(3000);
    //    cout<<"pass 2"<<endl;
    particleFilter2DColor tracker = particleFilter2DColor(RGBFace, x,y, RGBFrame,
            RGBFrame, NumofparticlesPerCore, numProcs, 2, 20, 22); //for ducklings.

    readGT("../GT.txt", tracker.GT);


    //----------------------------------------------------
   int xudongcounter = 0;   // for debugging:



while(finishYesNo) {
    startTimer = MPI_Wtime();
    index++;
//    tracker.update(&frame);
    tracker.update( &RGBFrame );






    tracker.resampling();   // locally.


        // sending...
//        if(0 == myRank){
//            std::cout<<"Sending... "<< std::endl;
//        }

    for (int i = 0; i < (int)precentageLocalTransPars*numProcs*NumofparticlesPerCore; i++) {
        int index1 = randIndex(tracker.num_particles);
//            std::cout<<"index1: "<< index1 <<std::endl;
        int index2 = randIndex(tracker.num_particles);
//            std::cout<<"index2: "<< index2 <<std::endl;
        for (int j = 0; j < numProcs; j++) {
            int tag = (i+1)*(j+1)*((int)precentageLocalTransPars*numProcs*NumofparticlesPerCore)*numProcs;
            if (myRank == j) {
//                    std::cout<<"1. myRank: "<<myRank<<" send to destination: "
//                    <<(j + 1) % numProcs<<" -> index: "<< index1 <<", tag: "<<tag<<std::endl;

                MPI_Send(&(tracker.particle[index1]),1, MPI_pair, (j + 1) % numProcs, tag, MPI_COMM_WORLD);

//                    std::cout<<"particle: "<<tracker.particle[index1].first<<", "<<tracker.particle[index1].second<< std::endl;
            }
            else if (myRank == ((j + 1) % numProcs)) {
//                    std::cout<<"2. myRank: "<<myRank<<" receive from source: "
//                    <<j<<" -> receive index: "<< index2 <<", tag: "<<tag<< std::endl;

                MPI_Recv(&(tracker.particle[index2]),1, MPI_pair, j, tag, MPI_COMM_WORLD, &stat);

//                    std::cout<<"particle: "<<tracker.particle[index2].first<<", "<<tracker.particle[index2].second<< std::endl;
            }

        }
        MPI_Barrier(MPI_COMM_WORLD);
    }




//    if(0 == myRank){
//        std::cout<<"End of either global or local "<< std::endl;
//    }

    tracker.estimation();
    MPI_Barrier(MPI_COMM_WORLD);    // all processors are waiting here for following reducing.

    // gathering all the estimation and show estimation on myRank0:
    MPI_Reduce(&tracker.prediction.first, &tracker.prediction_MPI_sum.first, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tracker.prediction.second, &tracker.prediction_MPI_sum.second, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    tracker.timeStep++; // No matter global or local, timeStep = timeStep+1 in every time step.

//    if (0 == myRank) {
////        std::cout << "------------------- showing image... ----------------" << std::endl;
//        tracker.showPrediction(frame, timeInterval);
//        imshow("xudong", frame);
//    }
//    waitKey(1);
//
//    cap >> frame;
//    if (frame.empty())
//        break;

//    if(0 == myRank){
//        std::cout<<"tracker.timeStep on PU0: "<< tracker.timeStep <<std::endl;
//    }
    endTimer = MPI_Wtime();
    totalTime += endTimer - startTimer;
    xudongcounter++;
//    if(xudongcounter == 1)
//        break;

    fullFileName = outPutFoldername + std::to_string(index) + ".txt";
//    if(0 == myRank)
//        std::cout<< fullFileName <<std::endl;
    finishYesNo = readRGBtxt(fullFileName, RGBFrame);
    if(finishYesNo == false){break;}    // 3.Modif.
}





    MPI_Reduce(&totalTime, &tracker.timeComsumption, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(0 == myRank){
        calRMSE(tracker.RMSE,tracker.GT,tracker.predVec);

//        std::cout<<"timeComsumption (Second): "<< tracker.timeComsumption << std::endl;
//        std::cout<<"Number of Time Steps: "<< tracker.timeStep << std::endl;
//        std::cout<<"Number of PU: "<< tracker.num_Procs << std::endl;
//        std::cout<<"Number of Particles: "<< tracker.num_particles * tracker.num_Procs << std::endl;
//        std::cout<<"RMSE: "<< tracker.RMSE << std::endl;
//        std::cout<<"Neff Threshold: "<< tracker.th << std::endl;
//        std::cout<<"NofparTrans (global): "<< tracker.NofparTrans << std::endl;
//        std::cout<<"Number of Par trans between PU (Local): "<< (int)precentageLocalTransPars*numProcs*NumofparticlesPerCore << std::endl;
//        std::cout<<"timeInterval: "<< timeInterval << std::endl;

        writeResultsToLog(std::to_string( tracker.timeComsumption ),
                          std::to_string( tracker.timeStep ),
                          std::to_string( tracker.num_Procs ),
                          std::to_string( tracker.num_particles * tracker.num_Procs ),
                          std::to_string( tracker.RMSE ),

//                          std::to_string( tracker.NofparTrans ),
                          std::to_string((int)(precentageLocalTransPars*numProcs*NumofparticlesPerCore*tracker.timeStep) ));
    }
     
//     // When everything done, release the video capture object
//     cap.~VideoCapture ();
//     cap.release();
//     destroyAllWindows();   // Closes all the frames

//    std::cout<<"Pass the end"<<std::endl;



    MPI_Type_free(&MPI_pair);
    MPI_Type_free(&MPI_WEI);
    MPI_Finalize();


    return 0;
}
