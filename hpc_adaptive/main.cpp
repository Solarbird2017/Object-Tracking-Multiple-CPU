#include "mpi.h"
#include <iostream>
#include <random>
#include <string>
#include "particleFilter2D.h"



/**********************************************************************************

 1. Compile command:
 mpicxx -w `pkg-config --libs opencv` main.cpp

 2. run: TMPDIR=/tmp

 3. Running command (4 cores, 10 particles/core):
 mpirun -np 4 a.out 10

************************************************************************************/



//#define debugging


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

const int timeInterval = 5;
const double adaptThres = 0.2;  //0.1 results in a larger RMES. 0.5 -> RMSE:1.59862.
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

    particleFilter2DColor tracker = particleFilter2DColor(RGBFace, x,y, RGBFrame,
            RGBFrame, NumofparticlesPerCore, numProcs, 2, 20, 22, adaptThres); //for ducklings.

    readGT("../GT.txt", tracker.GT);


    //----------------------------------------------------
   int xudongcounter = 0;   // for debugging:



while(finishYesNo) {
    startTimer = MPI_Wtime();
    index++;
//    tracker.update(&frame);
    tracker.update( &RGBFrame );


    if(0 == tracker.timeStep % timeInterval){
#ifdef debugging
        if(0 == myRank){
            std::cout<< index <<". Enter Global, ";
        }
#endif
         //--------------------------------- after sampling ------------------------------------------
         // xudong: all processors need to wait for resampling here !!!
         // resampling as followings:
         // 1. obtain the deWei.
         for (int i = 0, ter = tracker.num_particles; i < ter; i++) {
             // 1. deWei needs "myRank", so needs to do here, not inside class.
             tracker.deWei[i] = detailWeights(myRank, i, tracker.oriWeis[i]);
         }


         // 2. assign the space for totalWeight on myRank=0.
         if (0 == myRank) {
             tracker.totalWeights.assign(tracker.num_particles * numProcs, detailWeights());
         }



         // 3. copy the deWei on all the PU to CU (myRank=0)
         MPI_Barrier(MPI_COMM_WORLD);    // all processors are waiting here.
         MPI_Gather(&(tracker.deWei.front()), tracker.num_particles, MPI_WEI,
                    &(tracker.totalWeights.front()), tracker.num_particles, MPI_WEI, 0, MPI_COMM_WORLD);

        //-------------- following code decides to do global or local resampling ---------------------
         // 4. normalizing the totalWeights on CU (myRank=0).
    //        std::cout<<"myRank: "<<myRank<<", tracker.skipGlobal: " << tracker.skipGlobal <<std::endl;
    //        MPI_Barrier(MPI_COMM_WORLD);
         if (0 == myRank) {
             weiNormalizor(tracker.totalWeights);
             tracker.calNeff();
#ifdef debugging
             std::cout<<"Neff: "<< tracker.Neff <<", NeffTh: "<< tracker.th;
#endif
             if(tracker.Neff >= tracker.th){
                 tracker.skipGlobal = 1;

    //             std::cout<<"tracker.skipGlobal = 1 on PU0 " <<std::endl;
             }
         }
         MPI_Bcast(&tracker.skipGlobal, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //     std::cout<<"myRank: "<<myRank<<", tracker.skipGlobal: " << tracker.skipGlobal <<std::endl;
         MPI_Barrier(MPI_COMM_WORLD);


         if(tracker.skipGlobal == 1){
#ifdef debugging
             if (0 == myRank)
                std::cout<<" -> skip global " <<std::endl;
#endif
             goto LOCAL;
         }

#ifdef debugging
        if (0 == myRank) {
            std::cout<<" -> do not skip global " <<std::endl;
        }
#endif

         // 5. global resampling on CU (myRank=0) and calculate the 2d vector -> goodParVec.
         if (0 == myRank) {
             tracker.goodParVec.assign(numProcs, std::vector<int>());
             selectGoodPars(tracker.totalWeights, numProcs, tracker.goodParVec);

             //update v1 and v2:
             tracker.v1.assign(numProcs, 0);
             tracker.v2.assign(numProcs, false);

             for (int i = 0, ter = tracker.goodParVec.size(); i < ter; i++) {
                 tracker.v1[i] = tracker.goodParVec[i].size();
                 if (tracker.v1[i] != NumofparticlesPerCore) {
                     tracker.v2[i] = true;   // find the PU, which need send and receive pars.
                 }
             }

             if(checkv1(tracker.v1) != numProcs * NumofparticlesPerCore){
                 std::cout << "Error here, total # of particle = "<< checkgoodParVec(tracker.goodParVec)<<std::endl;
                 break;
             }
         }

         //6. sending the size of good particles for those poor PUs, for self-copying.
         if (0 == myRank) {
             for (int i = 0, ter = (int) tracker.goodParVec.size(); i < ter; i++) {
                 MPI_Send(&(tracker.v1[i]), 1, MPI_INT, i, i, MPI_COMM_WORLD);   // sending selfCopyVecLength from PU0.
             }
         }

         //7. Receiving the size of good particles and ini selfCopyVec on poor PUs.
         MPI_Barrier(MPI_COMM_WORLD);
         for (int i = 0; i < numProcs; i++) {
             if (i == myRank) {
                 MPI_Recv(&tracker.selfCopyVecLength, 1, MPI_INT, 0, i, MPI_COMM_WORLD,
                          &stat);   //receive selfCopyVecLength size.
                 tracker.selfCopyVec.assign(tracker.selfCopyVecLength, -1);
             }
         }


         // 8. sending the index of good particles from PU0 to those poor PUs for self-copying.
         if (0 == myRank) {
             for (int i = 0, ter = (int) tracker.goodParVec.size(); i < ter; i++) {
                 int Nofpars = (int) tracker.goodParVec[i].size();
                 if (Nofpars < NumofparticlesPerCore && Nofpars > 0) { // then i is the index for the poor PUs.
                     MPI_Send(&tracker.goodParVec[i][0], (int) tracker.goodParVec[i].size(), MPI_INT, i, i * numProcs,
                              MPI_COMM_WORLD);
                 }
             }
     //        std::cout << "3. ----------- Finish Sending goodParVec ----------" << std::endl;

         }
         MPI_Barrier(MPI_COMM_WORLD);
         // 9. Receiving the index of good particles for those poor PUs, for self-copying.
         for (int i = 0; i < numProcs; i++) {
             if (i == myRank) {
                 if (tracker.selfCopyVecLength < NumofparticlesPerCore && tracker.selfCopyVecLength > 0) {
     //                std::cout << " 5. myRank: "<< myRank << " is receiving from PU0. with tag: " << myRank * numProcs << std::endl;
                     MPI_Recv(&tracker.selfCopyVec[0], tracker.selfCopyVecLength, MPI_INT, 0, i * numProcs, MPI_COMM_WORLD,
                              &stat);
                 }
             }
         }

        tracker.parTemp.assign(tracker.particle.begin(), tracker.particle.end());
     // 10. On poor PUs, rearrange particle according to selfCopyVec.
         if (tracker.selfCopyVecLength < NumofparticlesPerCore && tracker.selfCopyVecLength > 0) {


             for (int i = 0, ter = (int) tracker.selfCopyVec.size(); i < ter; i++) {
                 tracker.particle[i].first = tracker.parTemp[tracker.selfCopyVec[i]].first;
                 tracker.particle[i].second = tracker.parTemp[tracker.selfCopyVec[i]].second;
             }


         }
        // 11. Clear and  Create t1 and t2 according to goodParVec.
        // s1 and s2 (useless) the vector containing the PUs with surplus and shortage particles.
         if (0 == myRank) {

             tracker.t1.clear();
             tracker.t2.clear();


             for (int i = 0, ter = (int) tracker.goodParVec.size(); i < ter; i++) {
                 int Nofpars = (int) tracker.goodParVec[i].size();

                 if (Nofpars == NumofparticlesPerCore) {
                     // no particle need to be exchanged.
                 } else if (Nofpars > NumofparticlesPerCore) {
                     // has surplus particles.
     //                tracker.s1.push_back(tracker.goodParVec[i]);
     //                for (int j = 0, ter = tracker.goodParVec[i].size(); j < ter; j++) {
     //                    tracker.s1[i][j] = tracker.goodParVec[i][j];
     //                }
                     std::vector<std::pair<int, int> > results;
                     Histogram(tracker.goodParVec[i], results);

                     int PUid = i;
                     for (int i = 0, ter = results.size(); i < ter; i++) {
                         int Npars = results[i].second;
                         int ParId = results[i].first;
                         tracker.t1.push_back(t1Piece(PUid, Npars, ParId));
                     }
                 } else {
                     // has shortage of particles.
     //                tracker.s2.push_back(tracker.goodParVec[i]);
     //                for (int j = 0, ter = tracker.goodParVec[i].size(); j < ter; j++) {
     //                    tracker.s2[i][j] = tracker.goodParVec[i][j];
     //                }

                     int PUid = i;
                     std::vector<int> parEntryidVec;
     //                std::cout<<"Pass 1"<<std::endl;
                     createParEntryidVec((int) tracker.goodParVec[i].size(), NumofparticlesPerCore,
                                         parEntryidVec); //obtain the pari entries vector.
     //                std::cout<<"Pass 2"<<std::endl;
                     int NumofneededPar = NumofparticlesPerCore - Nofpars;   // must be positive.
                     tracker.t2.push_back(t2Piece(PUid, NumofneededPar, parEntryidVec));
                 }
             }
             sort(tracker.t1.begin(), tracker.t1.end(), t1Sorter);
             sort(tracker.t2.begin(), tracker.t2.end(), t2Sorter);
         }


     // 12. Clear TransSchedule and Create routing schedules.
         if (0 == myRank) {
             tracker.TransSchedule.clear();

             int debugCounter = 0;

             while (true) {
                 debugCounter++;

                 int numParTrns = 0, t1Num, t2Num, surNum;
                 int k = 0;  // send the kth row of t1.
                 int j = 0;    // receive at jth row of t2.

                 // obtain numParTrns from t1.
                 for (int ter = tracker.t1.size(); k < ter; k++) {
                     if (tracker.v2[tracker.t1[k].PUid] == true) {
                         t1Num = tracker.t1[k].Npars;  // =12.
                         break;
                     }
                 }

                 // compare numParTrns with surplus number of parts.
                 surNum = tracker.v1[tracker.t1[k].PUid] - NumofparticlesPerCore;    //=5. must > 0
                 numParTrns = t1Num < surNum ? t1Num : surNum;

                 // obtain numParTrns from t1.
                 for (int ter = tracker.t2.size(); j < ter; j++) {
                     if (tracker.v2[tracker.t2[j].PUid] == true) {
                         t2Num = tracker.t2[j].Npars;  // =5.
                         break;
                     }
                 }

                 numParTrns = numParTrns < t2Num ? numParTrns : t2Num; //=5.

                 // add route into TransSchedule:
                 for (int i = 0; i < numParTrns; i++) {
                     tracker.TransSchedule.push_back(route(tracker.t1[k].PUid, tracker.t1[k].ParIds, 1,
                                                           tracker.t2[j].PUid, tracker.t2[j].ParIds.back(), 1));

                     if ( ! tracker.t2[j].ParIds.empty() ){
                         tracker.t2[j].ParIds.pop_back();
                     }
                     else{
                         std::cout << " Vector, named ParIds, is Empty, Error here." << std::endl;
     //                    goto label;
                     }
                 }


                 updateTablesVecs(k, j, tracker.t1, tracker.t2, tracker.v1, tracker.v2, numParTrns, NumofparticlesPerCore);

                 int trueNumber = 0;
                 for (int i = 0, ter = tracker.v2.size(); i < ter; i++) {
                     if (true == tracker.v2[i])
                         trueNumber++;
                 }
                 if (0 == trueNumber)
                     break;

             }

     //        std::cout << "------------------- Finish step 12. on PU0 ----------------" << std::endl;
             tracker.selfCopyVec.clear();
             tracker.selfCopyVec.assign(numProcs*NumofparticlesPerCore,-1);
             for (int i = 0, ter=tracker.t1.size(); i < ter; i++) {
                 int id = tracker.t1[i].PUid * NumofparticlesPerCore + tracker.t1[i].ParIds;
                 tracker.selfCopyVec[id] = tracker.t1[i].Npars;
             }

         }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Scatter(&tracker.selfCopyVec[0], NumofparticlesPerCore, MPI_INT, &tracker.GoodParselfCopyVec[0],
                    NumofparticlesPerCore, MPI_INT, 0, MPI_COMM_WORLD);


         // 13. PU0 Broadcasts the TransSchedule vector to the rest of PUs.
         MPI_Barrier(MPI_COMM_WORLD);
         if (0 == myRank) {
             int Num = tracker.TransSchedule.size();
             tracker.SchSize = Num;
             tracker.NofparTrans += Num;
             tracker.cumNparTrans.push_back(Num);
     //        std::cout << "------------------- Finish step 13. on PU0 ----------------" << std::endl;
         }
         //14. send the TransSchedule size.
         MPI_Bcast(&tracker.SchSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
         if (0 != myRank) {
             tracker.TransSchedule.assign(tracker.SchSize, route());
     //        std::cout << "------------------- Finish step 14. on PU0 ----------------" << std::endl;
         }


         MPI_Barrier(MPI_COMM_WORLD);
         // send the TransSchedule.
         MPI_Bcast(&tracker.TransSchedule[0], tracker.TransSchedule.size(), MPI_Route, 0, MPI_COMM_WORLD);

        // 15. Sending and Receiving particles according to the TransSchedule.
        MPI_Barrier(MPI_COMM_WORLD);

         for (int i = 0, ter = tracker.TransSchedule.size(); i < ter; i++) {
             int s = tracker.TransSchedule[i].sourceId;
             int sPid = tracker.TransSchedule[i].sParticleId;
             int d = tracker.TransSchedule[i].desId;
             int dPid = tracker.TransSchedule[i].dParticleId;

             if (myRank == s) {
                 MPI_Send(&tracker.particle[sPid], 1, MPI_pair, d, i, MPI_COMM_WORLD);
             }

             else if (myRank == d) {
                 MPI_Recv(&tracker.particle[dPid], 1, MPI_pair, s, i, MPI_COMM_WORLD, &stat);
             }

         }

        for (int i = 0, ter = tracker.GoodParselfCopyVec.size(); i < ter; i++) {
            int j = 0;
            int parSize = tracker.GoodParselfCopyVec[i];    //=4.
            if( parSize > 0 ){  //i=3.
                for (; j < NumofparticlesPerCore; j++) {
                    tracker.particle[j].first = tracker.parTemp[i].first;
                    tracker.particle[j].second = tracker.parTemp[i].second;
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
//     showPairVectorContents(tracker.particle);



 //    showPairVectorContents(tracker.particle);

    }
    else {
        LOCAL:
//        MPI_Barrier(MPI_COMM_WORLD);

#ifdef debugging
        if(0 == myRank){
            if(1 == tracker.skipGlobal){
                std::cout<<index<<". Enter Local from skipping global "<< std::endl;
            }
            else
             std::cout<<index<<". Enter Local "<< std::endl;
        }
#endif
        tracker.skipGlobal = 0;

        tracker.resampling();   // locally.

        for (int i = 0; i < (int)precentageLocalTransPars*numProcs*NumofparticlesPerCore; i++) {
            int index1 = randIndex(tracker.num_particles);
//            std::cout<<"index1: "<< index1 <<std::endl;
            int index2 = randIndex(tracker.num_particles);
//            std::cout<<"index2: "<< index2 <<std::endl;
            for (int j = 0; j < numProcs; j++) {
                int tag = (i+1)*(j+1)*((int)precentageLocalTransPars*numProcs*NumofparticlesPerCore)*numProcs;
                if (myRank == j) {
                    MPI_Send(&(tracker.particle[index1]),1, MPI_pair, (j + 1) % numProcs, tag, MPI_COMM_WORLD);
                }
                else if (myRank == ((j + 1) % numProcs)) {
                    MPI_Recv(&(tracker.particle[index2]),1, MPI_pair, j, tag, MPI_COMM_WORLD, &stat);
                }

            }
            MPI_Barrier(MPI_COMM_WORLD);
        }

    }

    tracker.estimation();
    MPI_Barrier(MPI_COMM_WORLD);    // all processors are waiting here for following reducing.

    // gathering all the estimation and show estimation on myRank0:
    MPI_Reduce(&tracker.prediction.first, &tracker.prediction_MPI_sum.first, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tracker.prediction.second, &tracker.prediction_MPI_sum.second, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    tracker.timeStep++; // No matter global or local, timeStep = timeStep+1 in every time step.
    endTimer = MPI_Wtime();
    totalTime += endTimer - startTimer;
    xudongcounter++;
//    if(xudongcounter == 1)
//        break;

    fullFileName = outPutFoldername + std::to_string(index) + ".txt";
#ifdef debugging
    if(0 == myRank) {
        std::cout << fullFileName << std::endl;
    }
#endif
    finishYesNo = readRGBtxt(fullFileName, RGBFrame);
    if(finishYesNo == false){break;}    // 3.Modif.
}

    MPI_Reduce(&totalTime, &tracker.timeComsumption, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(0 == myRank){
        calRMSE(tracker.RMSE,tracker.GT,tracker.predVec);
        writeResultsToLog(std::to_string( tracker.timeComsumption ),
                          std::to_string( tracker.timeStep ),
                          std::to_string( tracker.num_Procs ),
                          std::to_string( tracker.num_particles * tracker.num_Procs ),
                          std::to_string( tracker.RMSE ),
                          std::to_string( tracker.th ),
                          std::to_string( tracker.NofparTrans ),
                          std::to_string((int)(precentageLocalTransPars * numProcs * NumofparticlesPerCore * tracker.timeStep) ),
                          std::to_string( timeInterval ) );
    }

    MPI_Type_free(&MPI_pair);
    MPI_Type_free(&MPI_WEI);
    MPI_Finalize();


    return 0;
}
