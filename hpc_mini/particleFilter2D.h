//
// Created by Xudong Zhang on 2019-03-07.
//

#ifndef NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H
#define NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H

#include <iostream>
#include <vector>
#include <set>
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
    detailWeights(): nodeId(0), particleId(0), weight(0){}
    detailWeights(int a, int b, double c) : nodeId(a), particleId(b), weight(c){}
};

template<typename mytype>
void show1DVectorContents(std::vector<mytype> arg){
    for(int i=0, ter=arg.size(); i<ter; i++){
        std::cout<<arg[i]<<std::endl;
    }
}

template<typename mytype>
void show1DVectorContents(std::vector<mytype> arg, int myRank){
    std::cout<<"--- "<<myRank<<" --- show particles ----"<<std::endl;
    for(int i=0, ter=arg.size(); i<ter; i++){
        std::cout<<arg[i]<<", ";
    }
    std::cout<<std::endl;
    std::cout<<"--------------------------------"<<std::endl;
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
//}






class particleFilter2DColor {
public:
    std::vector<std::vector<pix> >  face;
    std::vector<std::vector<pix> >  search_space;
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
//    bool goodPU = false;
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

    particleFilter2DColor(const std::vector<std::vector<pix> > & Face,
                    const int& FaceX,
                    const int& FaceY,
                    const std::vector<std::vector<pix> > & frameTesting,
                     const std::vector<std::vector<pix> > & searchSpace,
                     const int& N,
                     const int& numProcs,
                     const int& dim,
                     const double& controlStd,
                     const double& simStd):
                     face(Face), search_space(searchSpace), num_particles(N), num_Procs(numProcs),
                     dimensions(dim),control_std(controlStd), sim_std(simStd){
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
//        show1DVectorContents(idxs);

//        displace(particle, control_std);
//        std::cout<<"control_std: "<<control_std<<std::endl;
//        showPairVectorContents(particle);

//        sampling(frameTesting);
//        resampling();
//        showPairVectorContents(particle);
//        estimation();
//        showPrediction();
    }

    void update(std::vector<std::vector<pix> >* frame){

        displace(particle, control_std); //std::cout<< "Pass displace: "<<std::endl;
        sampling(*frame);    //std::cout<< "Pass sampling: "<<std::endl;
//        resampling();  //std::cout<< "Pass resampling: "<<std::endl;
//        estimation();   //std::cout<< "Pass estimation: "<<std::endl;
    }


    void displace(std::vector<std::pair<int, int> >& particle, const double& control_std){
        std::normal_distribution< > dist_normal_double(0.0, control_std);
        int leftEdge, topEdge, rightEdge, downEdge, faceWhalf = (int)face_w/2, faceHhalf = (int)face_h/2;
        int tempFirst = 0, tempSecond = 0;
        for(int i=0, ter=particle.size(); i<ter; i++) {
            particle[i].first += (int) dist_normal_double(gen);
            particle[i].second += (int) dist_normal_double(gen);
        }

//        for(int i=0, ter=particle.size(); i<ter; i++) {
//            std::cout<<"i: "<< i <<std::endl;
//            std::cout<< "particle[i].first: "<<particle[i].first<<", particle[i].second : "<<particle[i].second <<std::endl;
//            tempFirst = particle[i].first + (int) dist_normal_double(gen);
//            tempSecond = particle[i].second + (int) dist_normal_double(gen);
//            std::cout<< "tempFirst: "<< tempFirst <<", tempSecond: "<< tempSecond <<std::endl;
//
//            leftEdge = tempFirst - faceWhalf;
//            rightEdge = tempFirst + faceWhalf;
//
//            topEdge = tempSecond - faceHhalf;
//            downEdge = tempSecond + faceHhalf;
//
//            std::cout<< "leftEdge: "<< leftEdge <<", rightEdge: "<< rightEdge <<", space_w: "<< space_w <<std::endl;
//            std::cout<< "topEdge: "<< topEdge <<", downEdge: "<< downEdge <<", space_h: "<< space_h <<std::endl;
//
//
//            if (leftEdge < 0 || rightEdge > space_w || topEdge < 0 || downEdge > space_h){
//                i--;
//                std::cout<<"i--: "<< i << std::endl; waitKey(1000);
//                continue;
//            }
//            particle[i].first = tempFirst;
//            particle[i].second = tempSecond;
//            std::cout<< "particle[i].first: "<<particle[i].first<<", particle[i].second : "<<particle[i].second <<std::endl;
//        }
    }

    void sampling(std::vector<std::vector<pix> > img){

//        face_w = Face.cols;
//        face_h = Face.rows;
//        std::cout<<"-> face_w: "<<face_w<<", face_h: "<<face_h<<std::endl;

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
//        std::cout << "----- weights ------" << std::endl;
//        show1DVectorContents(weight); std::cout<<"\n";
        weiNormalizor(weight);
//        show1DVectorContents(weight);
//        std::cout << "--------------------" << std::endl;
    }

    void resampling(){
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
                    particle[index].first = subParticle[i].first;
                    particle[index].second = subParticle[i].second;
                    index++;
                    if(index >= num_particles) break;
                }
                else
                    break;
            }
        }
        // calculate the cumulated weight:


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


        //2. use the particle with maximum probability.
//        prediction.first = particle[bestParticleIndex].first;;
//        prediction.second = particle[bestParticleIndex].second;;

    }
/*
    void showWindow(Mat& frame, std::pair<int, int> prediction){
//        std::cout<<"prediction.first: "<<prediction.first<<", prediction.second: "<<prediction.second<<std::endl;
        Point pt1 = Point_<int>(prediction.first-face_w/2, prediction.second-face_h/2);
        Point pt2 = Point_<int>(prediction.first+face_w/2, prediction.second+face_h/2);
        rectangle(frame, pt1, pt2, Scalar(0,255,0), 1, 8, 0);

    }

    void showFace(Mat& frame){

//        Mat RGBFace;
//        cvtColor(face, RGBFace, COLOR_GRAY2BGR);
//        RGBFace.copyTo(frame(Rect(0,0,face.cols, face.rows)));
        face.copyTo(frame(Rect(0,0,face.cols, face.rows)));

    }

    void showParticles(Mat& frame){
        int radius = 2;
        const Scalar& color = Scalar(255,255,0);
        int thickness = -1;
        for(int i=0; i<num_particles; i++) {
            Point pt = Point_<int>(particle[i].first, particle[i].second);
            circle(frame, pt, 2, color, thickness);
        }
    }

    void showPatNumber(Mat& frame){
//        const string& text = ;

        Point pt = Point_<int>(space_w-260, 20);
        putText(frame, "Minimum Transfer", pt, FONT_HERSHEY_TRIPLEX, 0.5, Scalar(65, 66, 244),1);

        char text[50];
        sprintf(text,"Number of Particle: %i",num_Procs * num_particles);
        pt = Point_<int>(space_w-260, 40);
        putText(frame, text, pt, FONT_HERSHEY_TRIPLEX, 0.5, Scalar(65, 66, 244),1);

        char coreInfo[50];
        sprintf(coreInfo,"Node Number: %i",num_Procs);
        pt = Point_<int>(space_w-260, 60);
        putText(frame, coreInfo, pt, FONT_HERSHEY_TRIPLEX, 0.5, Scalar(65, 66, 244),1);

        char sysNoise[50];
        sprintf(sysNoise,"System Noise: %2.2f",control_std);
        pt = Point_<int>(space_w-256, 80);
        putText(frame, sysNoise, pt, FONT_HERSHEY_TRIPLEX, 0.5, Scalar(65, 66, 244),1);

        char meaNoise[50];
        sprintf(meaNoise,"Measurement Number: %2.2f",sim_std);
        pt = Point_<int>(space_w-260, 100);
        putText(frame, meaNoise, pt, FONT_HERSHEY_TRIPLEX, 0.5, Scalar(65, 66, 244),1);

    }

    void showPrediction(Mat& frame, std::pair<int, int> prediction, const int & NperCore, const int & numProcs){
        showWindow(frame, prediction);
        showFace(frame);
        showParticles(frame);
        showPatNumber(frame);
    }
*/
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
    
    
    //    cout<<"sum: "<< sum <<endl;
    RMSE = sqrt(sum)/ter;
}

void makeFace(std::vector<std::vector<pix> > Frame,
              std::vector<std::vector<pix> >& Face,
              const int x,
              const int y,
              const int width,
              const int height){

    Face.assign(height, std::vector<pix>());
//    cout<<"pass 0.0"<<endl;
    for(int j=0; j<height; j++){
//        for(int i=x; i<x+width; i++){
//
//        }
        std::vector<pix>::iterator beginIterator = Frame[j+y].begin()+x;
        Face[j].assign(beginIterator, beginIterator+width);
    }
//    cout<<"pass 0.1"<<endl;
}

#endif //NICKPFTRACKING_PARTICLEFILTER2DCOLOR_H
