//
// Created by Nick Zhang on 2019-03-27.
//

#ifndef ADAPTIVE_HPC_FILEIO_H
#define ADAPTIVE_HPC_FILEIO_H



#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using std::cout;
using std::endl;


class pix{
public:
    int r;
    int g;
    int b;
    pix():r(0), g(0), b(0) {}
    pix(int x,int y, int z) : r(x), g(y), b(z) {}
};

bool readRGBtxt(std::string filename, std::vector<std::vector<pix> >& RGB){
    int rows, cols, x;    
    std::ifstream inFile;
    inFile.open(filename);
    if (!inFile) {
//        std::cout << "Finish or Error: Unable to open file\n" ;
        return false;
    }
    inFile >> rows;
    inFile >> cols;
    
    //    std::cout<<"rows: "<< rows <<", cols: "<< cols << std::endl;
    RGB.assign(rows, std::vector<pix>(cols, pix(0,0,0)));
    
    for(int i=0; i< rows; i++){
        for(int j=0; j< cols; j++){
            for(int k=0; k< 3; k++){   // read those 3 values.
                inFile >> x;
                if(0 == k%3){    // read 1st column.
                    //                    tempPix = pix(0, 0, 0);
                    //                    tempPix.r = x;
                    RGB[i][j].r = x;
                    //                    cout<<"x: "<< x <<", RGB[i][j].r : "<< RGB[i][j].r  <<endl;
                }
                else if(1 == k%3){   // read 2nd column.
                    //                    tempPix.g = x;
                    RGB[i][j].g = x;
                }
                else if(2 == k%3){   // read 3rd column.
                    //                    tempPix.b = x;
                    RGB[i][j].b = x;
                }
                //                counter++;
            }
        }
    }
    inFile.close();
    return true;
}

void readGT(std::string filename, std::vector<std::pair<int, int> >& GT){
    int x;
    int cout = 0;
    std::pair<int, int> tempPair;
    std::ifstream inFile;
    inFile.open(filename);
    if (!inFile) {
        std::cout << "Error: Unable to open file";
        exit(1);
    }
    
    while (inFile >> x) {
        
        if(0 == cout%2){
            tempPair = std::make_pair(0,0);
            tempPair.first = x;
        }
        else{
            tempPair.second = x;
            GT.push_back(tempPair);
        }
        cout++;
        
    }
    
    inFile.close();
    
}

void showGT(std::vector<std::pair<int, int> >* GT){
    for(std::pair<int, int> t : *GT)
        std::cout << t.first <<", " << t.second << std::endl;
    
}

void showRGB2DVec(std::vector<std::vector<pix> > *RGB){
    for(int i=0; i< (*RGB).size(); i++) {
        for (int j = 0; j < (*RGB)[0].size(); j++) {
            std::cout <<(*RGB)[i][j].r << " "<< (*RGB)[i][j].g << " "<< (*RGB)[i][j].b << std::endl;
        }
    }
}


#endif //ADAPTIVE_HPC_FILEIO_H

