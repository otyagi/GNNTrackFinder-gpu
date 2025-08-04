/* Copyright (C) 2012-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Andrey Lebedev */

/**
 * \file CbmCluster.h
 * \brief Base class for cluster objects.
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2012
 */

#ifndef CBMCLUSTER_H_
#define CBMCLUSTER_H_

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <TObject.h>     // for TObject

#include <cstdint>
#include <string>  // for string
#include <vector>  // for vector

class CbmMatch;

/**
 * \class CbmCluster
 * \brief Base class for cluster objects.
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2012
 */
class CbmCluster : public TObject {
public:
  /**
	 * \brief Constructor.
	 */
  CbmCluster();
  CbmCluster(const std::vector<int32_t>& indices, int32_t address);
  CbmCluster(const CbmCluster&);
  CbmCluster(CbmCluster&& other) noexcept;
  CbmCluster& operator=(const CbmCluster&);
  CbmCluster& operator=(CbmCluster&&) noexcept;

  /**
	 * \brief Destructor.
	 */
  virtual ~CbmCluster();

  /**
	 * \brief Add digi to cluster.
	 * \param[in] index Digi index in TClonesArray.
	 */
  void AddDigi(int32_t index) { fDigis.push_back(index); }

  /**
	 * \brief Add array of digi to cluster.
	 * \param[in] indices Array of digi indices in TClonesArray.
	 */
  void AddDigis(const std::vector<int32_t>& indices) { fDigis.insert(fDigis.end(), indices.begin(), indices.end()); }

  /**
	 * \brief Set array of digi to cluster. Overwrites existing array.
	 * \param[in] indices Array of digi indices in TClonesArray.
	 */
  void SetDigis(const std::vector<int32_t>& indices) { fDigis.assign(indices.begin(), indices.end()); }

  /**
	 * \brief Number of digis in cluster.
	 * \return Number of digis in cluster.
	 */
  int32_t GetNofDigis() const { return fDigis.size(); }

  /**
	 * \brief Get digi at position index.
	 * \param[in] index Position of digi in array.
	 * \return Digi index in TClonesArray.
	 */
  int32_t GetDigi(int32_t index) const { return fDigis[index]; }

  /**
	 * \brief Get array of digi indices.
	 * \return Array of digi indices in TClonesArray.
	 */
  const std::vector<int32_t>& GetDigis() const { return fDigis; }

  /**
	 * \brief Remove all digis.
	 */
  void ClearDigis() { fDigis.clear(); }

  /** Accessors **/
  int32_t GetAddress() const { return fAddress; }
  CbmMatch* GetMatch() const { return fMatch; }

  /** Modifiers **/
  void SetAddress(int32_t address) { fAddress = address; }
  void SetMatch(CbmMatch* match);

  /**
    * \brief Return string representation of the object.
    * \return String representation of the object.
    **/
  virtual std::string ToString() const;

private:
  std::vector<int32_t> fDigis;  ///< Array of digi indices
  int32_t fAddress;             ///< Unique detector ID
  CbmMatch* fMatch;           ///< link to Monte-Carlo information

  ClassDef(CbmCluster, 2);
};

#endif /* CBMCLUSTER_H_ */
