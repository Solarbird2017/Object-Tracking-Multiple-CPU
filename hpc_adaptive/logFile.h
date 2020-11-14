//
// Created by Nick Zhang on 2019-03-30.
//

#ifndef HPC_ADAPTIVE_LOGFILE_H
#define HPC_ADAPTIVE_LOGFILE_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

using std::string;
using std::fstream;

void writeResultsToLog(string timecom, string Nsteps, string NPUs,
        string NPs, string rmse, string NeffThre, string NPstransGlo,
        string NPstransLoc, string TimeInterval ){
    string logFile = "log.txt";
    fstream filestr;
    filestr.open (logFile.c_str(), fstream::in | fstream::out | fstream::trunc);

    filestr << "Results of Adaptive Resampling: " << endl;

    filestr << "timeComsumption (Second): " << timecom << endl;
    filestr << "Number of Time Steps: " << Nsteps << endl;
    filestr << "Number of PU: " << NPUs << endl;
    filestr << "Number of Particles: " << NPs << endl;
    filestr << "RMSE: " << rmse << endl;
    filestr << "Neff Threshold: " << NeffThre << endl;
    filestr << "NofparTrans (Global): " << NPstransGlo<< endl;
    filestr << "Number of Par trans between PU (Local): " << NPstransLoc << endl;
    filestr << "TimeInterval: " << TimeInterval << endl;
    filestr.close();
}
#endif //HPC_ADAPTIVE_LOGFILE_H

