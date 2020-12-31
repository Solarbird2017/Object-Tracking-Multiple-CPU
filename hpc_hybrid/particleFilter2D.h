//
// Created by Xudong Zhang on 2019-03-07.
//

#ifndef NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H
#define NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H


//#include "opencv2/opencv.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <random>
#include <math.h>
#include "../mySet.h"
#include "../fileIO.h"
#include "logFile.h"



void makeFace(std::vector<std::vector<pix> > Frame,std::vector<std::vector<pix> >& Face,
              const int x,const int y,const int width,const int height);


std::random_device rd;
std::mt19937 gen(rd());
//std::uniform_real_distribution< > dist_uni_double(0.0, 1.0);



std::pair<int, int> random2dCoord(const int& space_w, const int& space_h, const int& ww, const int& hh){
//    const int a=space_w - ww, b=space_h - hh;
    std::uniform_int_distribution<int> w(ww, space_w - ww);
    std::uniform_int_distribution<int> h(hh, space_h - hh);
    return std::make_pair(w(gen), h(gen));

}

int randIndex(const int& N){
    std::uniform_int_distribution<int> integer(0, N-1);
    return integer(gen);
}

class detailWeights{
public:
    int nodeId;
    int particleId;
    double weight;
public:
    detailWeights(): nodeId(0), particleId(0), weight(0.0){}
    detailWeights(int a, int b, double c) : nodeId(a), particleId(b), weight(c){}
};

template<typename mytype>
void show1DVectorContents(std::vector<mytype> arg){
    for(int i=0, ter=arg.size(); i<ter; i++){
        std::cout<<arg[i]<<std::endl;
    }
}

template<typename mytype1, typename mytype2>
void showPairVectorContents(std::vector<std::pair<mytype1, mytype2> > arg){
    std::cout<<"------ show particles ------"<<std::endl;
    for(int i=0, ter=arg.size(); i<ter; i++){
        std::cout<<arg[i].first<<", "<<arg[i].second<<std::endl;
    }
    std::cout<<"----------------------------"<<std::endl;
}


void weiCalculator(double& singleWeight, const std::vector<std::vector<pix> > & face,
                   const std::vector<std::vector<pix> >& tempFace, const double& sim_std) {
    if (face.size() != tempFace.size() || face[0].size() != tempFace[0].size())
        std::cout << "Error: true face and particle face has different dimension !!! " << std::endl;

//    std::cout << "face = "<< std::endl << " "  << face << std::endl << std::endl;
//    std::cout << "tempFace = "<< std::endl << " "  << tempFace << std::endl << std::endl;
    int temp0 = 0, temp1 = 0, temp2 = 0, sqrSum = 0;
    int sz = face.size(), ter = face[0].size(), dim = 3;
    for (int j = 0; j < sz; j++) {
        for (int i = 0; i < ter; i++) {
            temp0 = face[j][i].b - tempFace[j][i].b; // blue
            temp1 = 1.5 * (face[j][i].g - tempFace[j][i].g); // green
            temp2 = face[j][i].r - tempFace[j][i].r; // red

            sqrSum += temp0 * temp0 + temp1 * temp1 + temp2 * temp2;
        }
    }

    double mse = (double) sqrSum / (sz * ter * dim);
    singleWeight = exp(-mse / (2 * sim_std * sim_std));
}

void weiNormalizor(std::vector<double>& arg){
    double sum = 0;

    for(int i=0, ter=arg.size(); i<ter; i++) {
        sum += arg[i];
    }

    for(int i=0, ter=arg.size(); i<ter; i++) {
        arg[i] /= sum;
    }

}

void weiNormalizor(std::vector<detailWeights>& totalWeights){
    double sum = 0;
    int ter = totalWeights.size();

    for(int i=0; i<ter; i++) {
        sum += totalWeights[i].weight;
    }

    for(int i=0; i<ter; i++) {
        totalWeights[i].weight /= sum;
    }

}


//void showInfo(Mat* img, const int & counter){
//    std::cout<<"-----"<< counter <<"------"<<std::endl;
//    std::cout<<"1. depth: "<<(*img).depth()<<std::endl;   //-> 0.
//    std::cout<<"2. cols: "<<(*img).cols<<std::endl;  //-> 1280.
//    std::cout<<"3. rows: "<<(*img).rows<<std::endl;  //-> 720.
//    std::cout<<"4. channels: "<<(*img).channels()<<std::endl;    //-> 3.
//    std::cout<<"5. type: "<<(*img).type()<<std::endl;    //-> 16. CV_8UC3, //-> 0. CV_8UC1
//    imshow("img", *img);
//    std::cout<<"-----------"<<std::endl;
//    waitKey(5000);
//
//}






class particleFilter2DColor {
public:
    std::vector<std::vector<pix> > face;    // 7. modify.
    std::vector<std::vector<pix> > search_space;    // 8. Modify.
    int num_particles=0;
    int num_Procs=0;
    int dimensions=2;
    double control_std=0.0;
    double sim_std=0.0;

    int space_w=0;
    int space_h=0;
    int face_w=0;
    int face_h=0;

    std::vector<std::pair<int, int> > particle;
    std::vector<std::pair<int, int> > parTemp;

    std::vector<double> weight;
    std::vector<double> oriWeis;
    std::vector<detailWeights> deWei;
    std::vector<detailWeights> totalWeights;
    std::vector<std::vector<int> > goodParVec;  // on PU0 with 4 cours: [2,4,12,0], empty on other PU.

    std::vector<int> selfCopyVec;   // assign to be N's -1.
    std::vector<int> GoodParselfCopyVec;   // assign to be N's -1.
    int selfCopyVecLength=0;    // # of good particle on self-PU.
//    std::vector<int> GoodparNumOnEachPU;

    std::vector<t1Piece> t1;
    std::vector<t2Piece> t2;

    std::vector<int> v1;
    std::vector<bool> v2;

    std::vector<route> TransSchedule;
    int SchSize=0;

    std::vector<int> idxs;
    std::pair<int, int> prediction;
    std::pair<int, int> prediction_MPI_sum; // sum of coordinations of prediction on all the particles.
    int bestParticleIndex = 0;
    std::vector<std::pair<int, int> > GT;
    std::vector<std::pair<int, int> >predVec;
    double th = 0.1;
    double Neff=0;
    int skipGlobal = 0;

    int NofparTrans = 0;
    int timeStep = 0;
    std::vector<int> cumNparTrans;
    double RMSE = 0;    //time-averaged root mean square error (RMSE).
    double timeComsumption = 0;


public:
    particleFilter2DColor(){
//        std::cout<<"No Parameters parsed !!!"<<std::endl;
    }
    ~particleFilter2DColor(){
//        std::cout << "Object is being deleted" <<std::endl;
    }

    particleFilter2DColor(const std::vector<std::vector<pix> >& Face,   // 4.Modif.
                          const int& FaceX,
                          const int& FaceY,
                          const std::vector<std::vector<pix> >& frameTesting, // 5.Modif.
                          const std::vector<std::vector<pix> >& searchSpace, // 6.Modif.
                     const int& N,
                     const int& numProcs,
                     const int& dim,
                     const double& controlStd,
                     const double& simStd,
                     const double& adaptThres):
                     face(Face), search_space(searchSpace), num_particles(N), num_Procs(numProcs),
                     dimensions(dim),control_std(controlStd), sim_std(simStd), th(adaptThres * numProcs * N){

        deWei.assign(N,detailWeights(0,0,0));

//        std::cout<<"Entered !!!"<<std::endl;
        face_w = (int)Face[0].size();   // 8. Modify.
        face_h = (int)Face.size();  // 8. Modify.
        space_w = (int)searchSpace[0].size();   // 8. Modify.
        space_h = (int)searchSpace.size();  // 8. Modify.

//        std::cout<<"face_w: "<<face_w<<std::endl;
//        std::cout<<"face_h: "<<face_h<<std::endl;
//        std::cout<<"space_w: "<<space_w<<std::endl;
//        std::cout<<"space_h: "<<space_h<<std::endl;

        // 0. initial s1 and s2:
        GoodParselfCopyVec.assign(N, -1);
        selfCopyVec.assign(N, -1);
        v1.assign(numProcs, 0);
        v2.assign(numProcs, false);

        // 1. Assigning N random particle:
        particle.assign(N, std::pair<int, int>(0, 0));
        for(int i=0; i<N; i++){
//            particle[i] = random2dCoord(space_w, space_h, face_w/2, face_h/2);  //xudong: uniform distribution on image space. Wrong!!!
            particle[i].first = (int)(FaceX+face_w/2);
            particle[i].second = (int)(FaceY+face_h/2);
        }
//        showPairVectorContents(particle);


        // 2. Calculating the weights:
        weight.assign(N,1.0/N);
//        show1DVectorContents(weight);

        // 3. Assigning the particle indices:
        idxs.assign(N,0);
        for(int i=0; i<N; i++){
            idxs[i] = i;
        }
    }

    void update(std::vector<std::vector<pix> >* frame){

        displace(particle, control_std); //std::cout<< "Pass displace: "<<std::endl;
        sampling(*frame);    //std::cout<< "Pass sampling: "<<std::endl;
    }
    void calNeff(){
        int ter = totalWeights.size();
        double sum = 0.0;
        for(int i=0; i<ter; i++) {
            double temp = totalWeights[i].weight;
            sum += temp * temp;
        }
        Neff = 1/sum;
    }

    void displace(std::vector<std::pair<int, int> >& particle, const double& control_std){
        std::normal_distribution< > dist_normal_double(0.0, control_std);
        int leftEdge, topEdge, rightEdge, downEdge, faceWhalf = (int)face_w/2, faceHhalf = (int)face_h/2;
        int tempFirst = 0, tempSecond = 0;
        for(int i=0, ter=particle.size(); i<ter; i++) {
            particle[i].first += (int) dist_normal_double(gen);
            particle[i].second += (int) dist_normal_double(gen);
        }
    }

    void sampling(std::vector<std::vector<pix> > img){
        for(int i=0; i<num_particles; i++){

            int leftEdge = particle[i].first - (int)face_w/2;
            int  topEdge = particle[i].second - (int)face_h/2;

            //Dealing with particles outside the frame boundary, then corresponding weight=0.
            if(leftEdge < 0 || leftEdge+face_w > space_w || topEdge < 0 || topEdge+face_h > space_h){
                weight[i] = 0;
//                std::cout << "Particle is outside the frame!" << std::endl;
                continue;
            }
            std::vector<std::vector<pix> > tempFace;
            makeFace(img, tempFace, leftEdge, topEdge, face_w, face_h);

//            Mat tempFace = img(cv::Rect(leftEdge, topEdge, face_w, face_h));
            weiCalculator(weight[i], face, tempFace, sim_std);
//            std::cout << weight[i] << std::endl;

        }
        oriWeis.assign(weight.begin(), weight.end());   // copy wieght to oriWeis.
        weiNormalizor(weight);
    }

    void resampling(){
//        std::cout<<"Execute Local resampling..."<< std::endl;
        int bestParticleWeight = 0;
        double subSum = 0;
        double step = 1.0/num_particles;
        int k=1;
        int index = 0;
        std::vector<std::pair<int, int> > subParticle = particle;
        for(int i=0; i<num_particles; i++){
            subSum += weight[i];
            if(weight[i] > bestParticleWeight){
                bestParticleWeight = weight[i];
                bestParticleIndex = i;
            }

            for( ; k<=num_particles; k++){
                if(subSum >= step * k - 0.0001){
                    index++;
                    if(index >= num_particles)
                        break;
                    particle[index].first = subParticle[i].first;
                    particle[index].second = subParticle[i].second;
                }
                else
                    break;
            }
        }
    }

    void estimation(){
        //1. use the mean.
        int sum_left_edge = 0;
        int sum_top_edge = 0;
        for(int i=0; i<num_particles; i++){
            sum_left_edge += particle[i].first;
            sum_top_edge += particle[i].second;
        }
        prediction.first = (int)sum_left_edge/num_particles;
        prediction.second = (int)sum_top_edge/num_particles;
        predVec.push_back(prediction);

    }
};


void selectGoodPars(const std::vector<detailWeights> totalWeights,
                    const int & numProcs,
                    std::vector<std::vector<int> >& goodParVec){

    double subSum = 0;
    int N_particles = totalWeights.size();
//    std::cout << "N: " << N_particles <<std::endl;
    double step = 1.0/N_particles;
    int k=1;
    int index = 0;

    for(int i=0; i<N_particles; i++){
        subSum += totalWeights[i].weight;

        for( ; k<=N_particles; k++){
//            if(subSum >= step * k ){
            if(subSum >= step * k - 0.0001){
                index++;
//                std::cout<<"index: "<< index <<", ";
                if(index > N_particles)
                    break;
                goodParVec[totalWeights[i].nodeId].push_back(totalWeights[i].particleId);


            }
            else
                break;
        }
    }
//    std::cout<<std::endl;
}

void calRMSE(double& RMSE,
             const std::vector<std::pair<int, int> >& GT,
             const std::vector<std::pair<int, int> >& predVec){
    const int ter = predVec.size(); // forced to calculate the RMSE even GT and preVec do not have the same size.
    int sum = 0;
    if(GT.size() != predVec.size()) {
//        std::cout << "Error -> GT and predVec have different size!!!" << std::endl;
    }
    for(int i=0; i<ter; i++){
        sum += (GT[i].first - predVec[i].first)*(GT[i].first - predVec[i].first) +
        (GT[i].second - predVec[i].second)*(GT[i].second - predVec[i].second);
    }
    RMSE = sqrt(sum)/ter;
}

void makeFace(std::vector<std::vector<pix> > Frame,
              std::vector<std::vector<pix> >& Face,
              const int x,
              const int y,
              const int width,
              const int height){

    Face.assign(height, std::vector<pix>());
    for(int j=0; j<height; j++){
        std::vector<pix>::iterator beginIterator = Frame[j+y].begin()+x;
        Face[j].assign(beginIterator, beginIterator+width);
    }
}

#endif //NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H
