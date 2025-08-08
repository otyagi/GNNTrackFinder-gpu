#pragma once  // include this header only once per compilation unit

#include "AlgoFairloggerCompat.h"

#include <algorithm>
#include <random>
#include <vector>


typedef std::vector<std::vector<float>> Matrix;  // template this

class MLPutil {
 private:
 public:
  /**
     * @brief loads data from file
     * @param fileName : name of file containing data
    */
  static void loadDataEmbed(std::vector<std::string>& filePaths, std::vector<Matrix>& EpochInputs,
                            std::vector<IntMatrix>& EpochMCInfo, int trackType, std::vector<int>& nHitsEvent)
  {
    int numEvents = filePaths.size();

    std::vector<Matrix> Inputs;
    Inputs.resize(numEvents);
    std::vector<Matrix> MCInfo;
    MCInfo.resize(numEvents);

    static std::ifstream file;
    std::string line, num;
    for (int iEvent = 0; iEvent < numEvents; iEvent++) {
      file.open(filePaths[iEvent].data());
      if (!file) LOG(error) << "Could not open file: " << filePaths[iEvent];

      int iHit = 0;
      // read data from file
      // File columns: x,y,z,time,station_id,track_id,is_primary,get_p,not_found_two_iter
      while (std::getline(file, line)) {
        Inputs[iEvent].push_back(std::vector<float>(3));
        MCInfo[iEvent].push_back(std::vector<float>(5));
        std::stringstream str(line);
        for (int lineIndex = 0; lineIndex < 9; lineIndex++) {
          std::getline(str, num, ' ');
          if (lineIndex < 3) {  // x,y,z,time(skipped)
            Inputs[iEvent][iHit][lineIndex] = std::stof(num);
          }
          else if (lineIndex > 3) {  // station_id,track_id,is_primary,get_p,not_found_two_iter
            MCInfo[iEvent][iHit][lineIndex - 4] = std::stof(num);
          }
        }
        iHit++;
      }
      std::cout << "All hits in event " << iEvent << ": " << Inputs[iEvent].size() << std::endl;
      file.close();
    }

    /// move coordinates so primary vertex is at origin. z' = z + 44.0
    /// rescale coordinates
    const float XYZscaling = 1.0f;
    EpochInputs.resize(numEvents);
    EpochMCInfo.resize(numEvents);
    for (int iEvent = 0; iEvent < numEvents; iEvent++) {
      int iHitSelected = 0;
      for (int iHit = 0; iHit < (int) Inputs[iEvent].size(); iHit++) {
        if (trackType == 0) {                                                       // fastPrim
          if (MCInfo[iEvent][iHit][2] != 1.0f || MCInfo[iEvent][iHit][3] < 1.0f) {  // select hits from fastPrim tracks
            continue;
          }
        }
        else if (trackType == 3) {                                                  // secondary
          if (MCInfo[iEvent][iHit][2] != 0.0f || MCInfo[iEvent][iHit][3] < 0.1f) {  // select hits from secondary tracks
            continue;
          }
        }
        EpochInputs[iEvent].push_back(std::vector<float>(3));
        EpochInputs[iEvent].back()[0] = (Inputs[iEvent][iHit][0] - 0.0f) / XYZscaling;
        EpochInputs[iEvent].back()[1] = (Inputs[iEvent][iHit][1] - 0.0f) / XYZscaling;
        EpochInputs[iEvent].back()[2] = (Inputs[iEvent][iHit][2] + 44.0f) / XYZscaling;

        EpochMCInfo[iEvent].push_back(std::vector<int>(2));
        EpochMCInfo[iEvent].back()[0] = (int) MCInfo[iEvent][iHit][0];
        EpochMCInfo[iEvent].back()[1] = (int) MCInfo[iEvent][iHit][1];
        iHitSelected++;
      }
      nHitsEvent.push_back(iHitSelected);
      std::cout << "num selected:" << EpochInputs[iEvent].size() << std::endl;
    }
  }


  /**
	 * @brief shuffles data and labels in the same order
	 * @param data, labels 
	 * @return None
     * @todo: check template version and remove this.
	 */
  static void shuffleData(std::vector<std::vector<float>>& data, std::vector<int>& labels)
  {

    std::vector<int> indices;
    for (std::size_t i = 0; i < data.size(); i++) {
      indices.push_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    std::vector<std::vector<float>> tempData;
    std::vector<int> tempLabels;
    for (std::size_t i = 0; i < data.size(); i++) {
      tempData.push_back(data[indices[i]]);
      tempLabels.push_back(labels[indices[i]]);
    }
    data   = tempData;
    labels = tempLabels;
  }

  template<typename T1, typename T2>
  static void shuffleData(std::vector<T1>& data, std::vector<T2>& labels)
  {

    if (data.size() != labels.size()) {
      LOG(error) << "Data and labels must be of same size";
      return;
    }

    std::vector<int> indices;
    for (std::size_t i = 0; i < data.size(); i++) {
      indices.push_back(i);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    std::vector<T1> tempData;
    std::vector<T2> tempLabels;
    for (std::size_t i = 0; i < data.size(); i++) {
      tempData.push_back(data[indices[i]]);
      tempLabels.push_back(labels[indices[i]]);
    }
    data   = tempData;
    labels = tempLabels;
  }


  /**
     * @brief calculates squared distance between two hits 
     * 
     */
  static float hitDistanceSq(std::vector<float>& hit1, std::vector<float>& hit2)
  {

    int dim    = hit1.size();
    float dist = 0.0f;
    for (int i = 0; i < dim; i++) {
      dist += (hit1[i] - hit2[i]) * (hit1[i] - hit2[i]);
    }

    return dist;
  }

  /**
     * @brief accepts activations and MC info.
     * Writes edge order to file. To plot use L1AlgoDraw::drawEdgeOrderHisto()
     * Writes edge order only among MC points. With fake hits, the order will on average be higher.
    */
  static void kNNHisto(const std::vector<std::vector<Matrix>>& activationsAllEvents,
                       const std::vector<IntMatrix>& MCInfoAllEvents, const int nLayers)
  {
    std::vector<int> edgeOrder;
    std::cout << "Number of events in kNN Histo: " << activationsAllEvents.size() << std::endl;

    for (int iEvent = 0; iEvent < (int) activationsAllEvents.size(); iEvent++) {
      const auto& activations = activationsAllEvents[iEvent];
      const auto& MCInfo      = MCInfoAllEvents[iEvent];
      std::cout << "Number of hits: " << activations.size() << std::endl;
      std::cout << "Num MC Info: " << MCInfo.size() << std::endl;
      /// for some testing
      // // create map with key mcid and value as number of times it appears
      // std::map<int, int> mcIDCount;
      // for (int iHit = 0; iHit < (int) MCInfo.size(); iHit++) {
      //   int mcID = MCInfo[iHit][1];
      //   if (mcIDCount.find(mcID) == mcIDCount.end()) {
      //     mcIDCount[mcID] = 1;
      //   }
      //   else {
      //     mcIDCount[mcID]++;
      //   }
      // }
      // // print mcID and count
      // for (auto it = mcIDCount.begin(); it != mcIDCount.end(); it++) {
      //   std::cout << "MCID: " << it->first << ", Count: " << it->second << std::endl;
      // }

      std::vector<std::vector<float>> embeddedCoords;
      int nHits = activations.size();
      embeddedCoords.resize(nHits);

      // coords are in activations[hitIndex][nLayers]
      for (int iHit = 0; iHit < nHits; iHit++) {
        embeddedCoords[iHit].resize(activations[iHit][nLayers].size());
        std::copy(activations[iHit][nLayers].begin(), activations[iHit][nLayers].end(), embeddedCoords[iHit].begin());
      }

      // find true MC doublets
      /// find adjacent hits in tracks. Condition for this is MCID must match and station must differ by exactly 1
      std::vector<std::vector<int>> MCdoublets;
      std::vector<float> distList;
      for (int iHit = 0; iHit < nHits; iHit++) {
        int sta  = MCInfo[iHit][0];
        int mcID = MCInfo[iHit][1];
        if (mcID == -1) continue;  // fake hit
        for (int jHit = 0; jHit < nHits; jHit++) {
          if (MCInfo[jHit][1] == mcID && MCInfo[jHit][0] == (sta + 1)) {
            MCdoublets.push_back({iHit, jHit});
            distList.push_back(hitDistanceSq(embeddedCoords[iHit], embeddedCoords[jHit]));
          }
        }
      }

      std::cout << "Number of MC doublets: " << MCdoublets.size() << std::endl;

      /// find kNN order of each hit pair in MCdoublets
      for (int iDoub = 0; iDoub < (int) MCdoublets.size(); iDoub++) {
        // find number of hits in embededCoords closer to hit1 than hit2
        int hit1       = MCdoublets[iDoub][0];
        int hit2       = MCdoublets[iDoub][1];
        float dist     = hitDistanceSq(embeddedCoords[hit1], embeddedCoords[hit2]);
        int nCloserAll = 0;  // order among all hits
        int nCloserAdj = 0;  // order among hits on next station
        for (int iHit = 0; iHit < (int) embeddedCoords.size(); iHit++) {
          if (iHit == hit1) continue;  // skip hit1
          if (hitDistanceSq(embeddedCoords[iHit], embeddedCoords[hit1]) <= dist) {
            nCloserAll++;
            if (MCInfo[iHit][0] == (MCInfo[hit1][0] + 1)) nCloserAdj++;  // on next station
          }
        }
        edgeOrder.push_back(nCloserAdj);

        /// debug: print 5 columns: hit1 MC ID, hit2 MC ID, dist, nCloserAll, nCloserAdj
        // if (iDoub%10 ==0){
        //     std::cout<<MCInfo[hit1][1]<<","<<MCInfo[hit2][1]<<","<<dist<<","<<nCloserAll<<","<<nCloserAdj<<std::endl;
        //     std::cout<<embeddedCoords[hit1][0]<<","<<embeddedCoords[hit1][1]<<","<<embeddedCoords[hit1][2]<<","<<embeddedCoords[hit1][3]<<std::endl;
        // }
      }

      std::cout << "Event " + std::to_string(iEvent) + ": Num entries in edgeOrder- " << edgeOrder.size() << std::endl;
    }

    /// print distances for all adjacent edges
    // for (int iHit = 0; iHit < nHits; iHit++){
    //     int sta = MCInfo[iHit][0];
    //     int mcID = MCInfo[iHit][1];
    //     for (int jHit = 0; jHit < nHits; jHit++){
    //         if( MCInfo[jHit][0] == (sta+1) ) continue; // skip non-adjacent hits
    //         if( MCInfo[jHit][1] == mcID && MCInfo[jHit][1] != -1 ){
    //             std::cout<<"True hit dist.= "<<hitDistanceSq(embeddedCoords[iHit], embeddedCoords[jHit])<<std::endl;
    //         }
    //         else{
    //             std::cout<<"Fake hit dist.= "<<hitDistanceSq(embeddedCoords[iHit], embeddedCoords[jHit])<<std::endl;
    //         }
    //     }
    // }

    // write edge order to file
    std::string fName = "/u/otyagi/cbmroot/NN/output/embed_EdgeOrder.txt";
    std::ofstream file(fName);
    if (!file) std::cerr << "Could not open file :" << fName << std::endl;
    file << edgeOrder.size() << std::endl;
    for (int i = 0; i < (int) edgeOrder.size(); i++) {
      file << edgeOrder[i] << std::endl;
    }
    file.close();
    std::cout << "Edge order written to file: " << fName << std::endl;
  }

  /**
   * @brief loads data prepared for edge classifier training
   * Reads all true and fake data separately, shuffles it, truncates upto nTrue and nFake and adds to data
   */
  static void loadTrainDataEdgeClassifier(const int nTrainingEx, Matrix& data, std::vector<int>& label)
  {
    // const std::string fileNameTrue  = "/u/otyagi/cbmroot/NN/data/edges_true_ev_0_813.txt";
    const std::string fileNameTrue  = "/u/otyagi/cbmroot/NN/data/secondary_edges_true_ev_100_999.txt";
    const std::string fileNameFalse = "/u/otyagi/cbmroot/NN/data/edges_fake_ev_0_813.txt";

    std::string line, num;

    Matrix dataTrue;
    Matrix dataFake;

    const int station = -1;  // -1 to use all stations

    const int nMaxLinesRead = 10 * nTrainingEx;
    readEdgeClassifierData(fileNameTrue, dataTrue, nMaxLinesRead, station);
    std::vector<int> labelTrue(dataTrue.size(), 0);
    readEdgeClassifierData(fileNameFalse, dataFake, nMaxLinesRead, station);
    std::vector<int> labelFake(dataFake.size(), 1);

    // shuffle true and fake data and labels
    shuffleData<std::vector<float>, int>(dataTrue, labelTrue);
    shuffleData<std::vector<float>, int>(dataFake, labelFake);

    // truncate upto nTrue and nFake and add to data
    const int nTrue = nTrainingEx / 2;
    const int nFake = nTrainingEx / 2;
    data.resize(nTrue + nFake);
    label.resize(nTrue + nFake);
    std::copy(dataTrue.begin(), dataTrue.begin() + nTrue, data.begin());
    std::copy(dataFake.begin(), dataFake.begin() + nFake, data.begin() + nTrue);
    std::copy(labelTrue.begin(), labelTrue.begin() + nTrue, label.begin());
    std::copy(labelFake.begin(), labelFake.begin() + nFake, label.begin() + nTrue);

    // shuffle again
    shuffleData(data, label);

    std::cout << "Num training examples: " << data.size() << std::endl;
  }

  static void readEdgeClassifierData(const std::string& fName, Matrix& data, const int nMaxLinesRead, const int station)
  {
    const float targX = 0.0f, targY = 0.0f, targZ = -44.0f;
    const float scalingXYZ = 5.0f;
    const float scalingSta = 1.0f;

    static std::ifstream file;
    std::string line, num;
    file.open(fName);
    if (!file) std::cerr << "Could not read training data from file :" << fName << std::endl;
    /// read until end of file or until nMaxLinesRead
    /// Each line is one example with 9 entries: eventNumber,x1,y1,z1,sta1(int),x2,y2,z2,sta2(int)
    while (std::getline(file, line) && (int) data.size() < nMaxLinesRead) {
      std::stringstream str(line);
      std::vector<float> tempData;
      for (int i = 0; i < 9; i++) {
        std::getline(str, num, ' ');
        if (i == 0) continue;  // skip event number

        if (i == 3 || i == 7) {
          tempData.push_back((std::stof(num) - targZ) / scalingXYZ);
        }
        else if (i == 1 || i == 5) {
          tempData.push_back((std::stof(num) - targX) / scalingXYZ);
        }
        else if (i == 2 || i == 6) {
          tempData.push_back((std::stof(num) - targY) / scalingXYZ);
        }
        else {
          tempData.push_back(std::stof(num) / scalingSta);
        }
      }
      /// tempData[] = {x1,y1,z1,sta1,x2,y2,z2,sta2}
      /// only want to train on chosen station
      if (station != -1 && tempData[3] != station) {
        continue;
      }
      // remove station entries
      tempData.erase(tempData.begin() + 3);
      tempData.erase(tempData.begin() + 6);
      // erase z
      // tempData.erase(tempData.begin() + 2);
      // tempData.erase(tempData.begin() + 4);
      data.push_back(tempData);
    }
    file.close();
  }

  /**
   * @brief loads data prepared for candidate classifier training
   * Reads all true and fake data separately, shuffles it, truncates upto nTrue and nFake and adds to data
   */
  static void loadTrainDataCandClassifier(const int nTrainingEx, Matrix& data, std::vector<int>& label,
                                          const bool useTestingData)
  {
    std::string fileNameTrue, fileNameFalse;
    int max_ndf = -1;
    if (useTestingData) {
      LOG(info) << "Using testing data for validation...";
      fileNameTrue  = "/u/otyagi/cbmroot/NN/data/CandClassifier/true_purity70_last_iter_cand_fit_info_ev_0_50.txt";
      fileNameFalse = "/u/otyagi/cbmroot/NN/data/CandClassifier/fake_last_iter_cand_fit_info_ev_0_50.txt";
      max_ndf       = 7.0f;  // use same as used in training model.
    }
    else {
      LOG(info) << "Using training data...";
      fileNameTrue  = "/u/otyagi/cbmroot/NN/data/CandClassifier/true_purity70_last_iter_cand_fit_info_ev_200_899.txt"; // 234k
      fileNameFalse = "/u/otyagi/cbmroot/NN/data/CandClassifier/fake_last_iter_cand_fit_info_ev_200_899.txt"; // 468k
      max_ndf       = 7;
    }

    Matrix dataTrue;
    Matrix dataFake;
    // LOG(info) << "Reading data from files: " << fileNameTrue << "\n and " << fileNameFalse;
    const int nMaxLinesRead = 10 * nTrainingEx;
    readCandClassifierData(fileNameTrue, dataTrue, nMaxLinesRead, max_ndf);
    LOG(info) << "Num true examples: " << dataTrue.size();
    std::vector<int> labelTrue(dataTrue.size(), 0);
    readCandClassifierData(fileNameFalse, dataFake, nMaxLinesRead, max_ndf);
    LOG(info) << "Num fake examples: " << dataFake.size();
    std::vector<int> labelFake(dataFake.size(), 1);
    LOG(info) << "Data read.";

    if (useTestingData) {
      // Just use all examples do not balance the data or shuffle
      std::cout << "Using all examples from both classes for testing.\n";
      data.resize(dataTrue.size() + dataFake.size());
      label.resize(dataTrue.size() + dataFake.size());
      std::copy(dataTrue.begin(), dataTrue.end(), data.begin());
      std::copy(dataFake.begin(), dataFake.end(), data.begin() + dataTrue.size());
      std::copy(labelTrue.begin(), labelTrue.end(), label.begin());
      std::copy(labelFake.begin(), labelFake.end(), label.begin() + labelTrue.size());
      std::cout << "Num testing examples: " << data.size() << std::endl;
      std::cout << "Data loaded for testing." << std::endl;
      return;  // no need to truncate data
    }
    else {  // training
      const int nReadFile   = std::min(dataTrue.size(), dataFake.size());
      int nFinalExEachClass = std::min(nReadFile, nTrainingEx);
      std::cout << "Num examples in each class: " << nFinalExEachClass << '\n';

      shuffleData<std::vector<float>, int>(dataTrue, labelTrue);
      shuffleData<std::vector<float>, int>(dataFake, labelFake);
      std::cout << "Data shuffled." << std::endl;

      // truncate upto nTrue and nFake and add to data
      data.resize(2 * nFinalExEachClass);
      label.resize(2 * nFinalExEachClass);
      std::copy(dataTrue.begin(), dataTrue.begin() + nFinalExEachClass, data.begin());
      std::copy(dataFake.begin(), dataFake.begin() + nFinalExEachClass, data.begin() + nFinalExEachClass);
      std::copy(labelTrue.begin(), labelTrue.begin() + nFinalExEachClass, label.begin());
      std::copy(labelFake.begin(), labelFake.begin() + nFinalExEachClass, label.begin() + nFinalExEachClass);
      shuffleData(data, label);

      std::cout << "Num training examples: " << data.size() << std::endl;
    }
  }


  static void readCandClassifierData(const std::string& fName, Matrix& data, const int nMaxLinesRead, const int max_ndf)
  {
    static std::ifstream file;
    std::string line, num;
    file.open(fName);
    if (!file) std::cerr << "Could not read training data from file :" << fName << std::endl;
    // LOG(info) << "File opened: " << fName;

    data.reserve(nMaxLinesRead);

    {  // model 0 to 7
      /// read until end of file or until nMaxLinesRead
      /// For models 0 to 7 : Each line is one example with 12 entries
      /// Columns - chi2, tx, ty, p_reco, C00, C11, C22, C33, C44, ndf(int), p_mc, pdg(int)
      // bool isLineValid = true;  // if any entry is NaN or Inf, skip the line
      // while (std::getline(file, line) && (int) data.size() < nMaxLinesRead) {
      //   isLineValid = true;
      //   std::stringstream str(line);
      //   std::vector<float> tempData;
      //   for (int i = 0; i < 12; i++) {
      //     std::getline(str, num, ' ');
      //     float temp = std::stof(num);
      //     if (isnan(temp) || isinf(temp)) {
      //       isLineValid = false;
      //       break;
      //     }
      //     if (i == 0 && temp > 100.0f) {  // remove large chi2. Make training set harder and more realistic
      //       isLineValid = false;
      //       break;
      //     }
      //     if (i == 4 && temp > 1e-5) {  // C00
      //       isLineValid = false;
      //       break;
      //     }
      //     if (i == 5 && temp > 1e-3) {  // C11
      //       isLineValid = false;
      //       break;
      //     }
      //     if ((i == 6 || i == 7) && temp > 1e-2) {  // C22, C33
      //       isLineValid = false;
      //       break;
      //     }
      //     tempData.push_back(temp);
      //   }
      //   if (!isLineValid) continue;

      //   /// only want to train on chosen ndf
      //   if (max_ndf != -1 && tempData[9] > (float) max_ndf) {
      //     continue;
      //   }
      //   // scale chi2
      //   tempData[0] /= 50.0f;
      //   // scale C00, C11
      //   tempData[4] *= 1e5f;
      //   tempData[5] *= 1e3f;
      //   // scale C22, C33
      //   tempData[6] *= 1e2f;
      //   tempData[7] *= 1e2f;

      //   // remove unwanted elements
      //   tempData.erase(tempData.begin() + 9, tempData.end());

      //   data.push_back(tempData);
      // }
    }

    // From model 8 onwards: Each line is one example with 16 entries:
    // Columns - chi2, tx, ty, 1/(q/p), C00, C11, C22, C33, C44, ndf(int), x, y, z, p_mc, pdg(int), isPrimary
    bool isLineValid = true;  // if any entry is NaN or Inf, skip the line
    while (std::getline(file, line) && (int) data.size() < nMaxLinesRead) {
      isLineValid = true;
      std::stringstream str(line);
      std::vector<float> tempData;
      for (int i = 0; i < 16; i++) {
        std::getline(str, num, ' ');
        float temp = std::stof(num);
        if (isnan(temp) || isinf(temp)) {
          isLineValid = false;
          break;
        }
        if (i == 0 && temp > 100.0f) {  // remove large chi2. Make training set harder and more realistic
          isLineValid = false;
          break;
        }
        if (i == 4 && temp > 1e-5) {  // C00
          isLineValid = false;
          break;
        }
        if (i == 5 && temp > 1e-3) {  // C11
          isLineValid = false;
          break;
        }
        if ((i == 6 || i == 7) && temp > 1e-2) {  // C22, C33
          isLineValid = false;
          break;
        }
        tempData.push_back(temp);
      }
      if (!isLineValid) continue;

      /// only want to train on chosen ndf
      if (max_ndf != -1 && tempData[9] > (float) max_ndf) {
        continue;
      }
      // scale chi2
      tempData[0] /= 50.0f;
      // scale C00, C11
      tempData[4] *= 1e5f;
      tempData[5] *= 1e3f;
      // scale C22, C33
      tempData[6] *= 1e2f;
      tempData[7] *= 1e2f;
      tempData[9] /= 10.0f;   // scale ndf
      tempData[10] /= 50.0f;  // scale x
      tempData[11] /= 50.0f;  // scale y
      tempData[12] += 44.0f;  // move z to origin
      tempData[12] /= 50.0f;  // scale z

      // remove unwanted elements
      tempData.erase(tempData.begin() + 13, tempData.end());

      data.push_back(tempData);
    }

    file.close();
    // LOG(info) << "File closed: " << fName;
  }

  static void fileEdgesDistribution()
  {
    std::cout << "Edge distribution in training data" << std::endl;
    std::vector<int> edgeDistribution(12, 0);

    std::ifstream file;
    std::string line, num;
    const std::string fName = "/u/otyagi/cbmroot/NN/data/edges_true_ev_0_813.txt";
    file.open(fName);
    if (!file) std::cerr << "Could not read training data from file :" << fName << std::endl;
    /// Each line is one example with 9 entries: eventNumber,x1,y1,z1,sta1(int),x2,y2,z2,sta2(int)
    while (std::getline(file, line)) {
      std::stringstream str(line);
      std::vector<float> tempData;
      for (int i = 0; i < 9; i++) {
        std::getline(str, num, ' ');
        if (i == 4) {
          edgeDistribution[std::stoi(num)]++;
          continue;
        }
      }
    }
    file.close();

    for (int i = 0; i < 12; i++) {
      std::cout << "Station " << i << ": " << edgeDistribution[i] << std::endl;
    }
    std::cout << "Total edges: " << std::accumulate(edgeDistribution.begin(), edgeDistribution.end(), 0) << std::endl;
  }

  /**
	 * @brief load triplet classifier data
	 * @return None
	 */
  static void loadDataTripletClassifier(const int nTrainingEx, Matrix& data, std::vector<int>& label, const int mode)
  {
    std::string fileNameTrue, fileNameFalse;
    int nTrue, nFake;
    if (mode == 0) {  // training mode
      fileNameTrue  = "/u/otyagi/cbmroot/NN/data/triplets_true_ev_0_20.txt";
      fileNameFalse = "/u/otyagi/cbmroot/NN/data/triplets_fake_ev_0_20.txt";
      nTrue         = nTrainingEx * 0.5;
      nFake         = nTrainingEx * 0.5;
    }
    else if (mode == 1) {  // validation mode
      fileNameTrue  = "/u/otyagi/cbmroot/NN/data/triplets_true_ev_900_910.txt";
      fileNameFalse = "/u/otyagi/cbmroot/NN/data/triplets_fake_ev_900_910.txt";
      nTrue         = nTrainingEx * 0.01;
      nFake         = nTrainingEx * 0.99;
    }
    else {
      LOG(error) << "Invalid mode for loading data";
    }

    static std::ifstream file;
    std::string line, num;

    Matrix dataTrue;
    Matrix dataFake;

    const int nMaxLinesRead = 2 * nTrainingEx;
    readTripletClassifierData(fileNameTrue, dataTrue, nMaxLinesRead);
    std::vector<int> labelTrue(dataTrue.size(), 0);
    readTripletClassifierData(fileNameFalse, dataFake, nMaxLinesRead);
    std::vector<int> labelFake(dataFake.size(), 1);

    // shuffle true and fake data and labels
    shuffleData<std::vector<float>, int>(dataTrue, labelTrue);
    shuffleData<std::vector<float>, int>(dataFake, labelFake);

    // truncate upto nTrue and nFake and add to data
    data.resize(nTrue + nFake);
    label.resize(nTrue + nFake);
    std::copy(dataTrue.begin(), dataTrue.begin() + nTrue, data.begin());
    std::copy(dataFake.begin(), dataFake.begin() + nFake, data.begin() + nTrue);
    std::copy(labelTrue.begin(), labelTrue.begin() + nTrue, label.begin());
    std::copy(labelFake.begin(), labelFake.begin() + nFake, label.begin() + nTrue);

    // shuffle again
    shuffleData(data, label);
  };

  /// NOTE: In data preparation, true files have extra column for track ID which is absent in fake files
  static void readTripletClassifierData(const std::string& fName, Matrix& data, const int nMaxLinesRead)
  {
    const float targX = 0.0f, targY = 0.0f, targZ = 0.0f;
    const float scalingXYZ = 1.0f;
    const float scalingSta = 1.0f;

    static std::ifstream file;
    std::string line, num;
    file.open(fName);
    if (!file) std::cerr << "Could not read training data from file :" << fName << std::endl;
    /// read until end of file or until nMaxLinesRead
    /// Each line is one example with eventNumber trackID x1 y1 z1 sta1 x2 y2 z2 sta2 x3 y3 z3 sta3
    while (std::getline(file, line) && (int) data.size() < nMaxLinesRead) {
      std::stringstream str(line);
      std::vector<float> tempData;
      for (int i = 0; i < 14; i++) {
        std::getline(str, num, ' ');
        if (i == 0 || i == 1)
          continue;  // skip event number and track id
        else if (i == 2 || i == 6 || i == 10) {
          tempData.push_back((std::stof(num) - targX) / scalingXYZ);
        }
        else if (i == 3 || i == 7 || i == 11) {
          tempData.push_back((std::stof(num) - targY) / scalingXYZ);
        }
        else if (i == 4 || i == 8 || i == 12) {
          tempData.push_back((std::stof(num) - targZ) / scalingXYZ);
        }
        else {  // station
          tempData.push_back(std::stof(num) / scalingSta);
        }
      }
      data.push_back(tempData);
    }
    file.close();
  }

  /// calculates curvature of three hits
  static float calcCurvature(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
  {
    float k = 0.0f;

    // calc distance between points
    float d12 = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
    float d23 = sqrt((x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2) + (z3 - z2) * (z3 - z2));
    float d31 = sqrt((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3) + (z1 - z3) * (z1 - z3));

    // check if triplet is collinear.
    if ((d12 + d23) == d31) {
      k = 0;
      return k;
    }

    // calc area of triangle (Heron's formula)
    float s    = (d12 + d23 + d31) / 2.0f;
    float area = sqrt(s * (s - d12) * (s - d23) * (s - d31));

    // calc curvature
    k = (4.0f * area) / (d12 * d23 * d31);

    return k;
  }
};
