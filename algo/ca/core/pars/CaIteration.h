/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   L1CAIteration.h
 * @brief  Declaration of the L1CAIteration class 
 * @since  05.02.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#pragma once  // include this header only once per compilation unit

#include "CaVector.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>

#include <bitset>
#include <iomanip>
#include <string>

namespace cbm::algo::ca
{
  /// \class cbm::algo::ca::Iteration
  /// \brief A set of parameters for the CA Track finder iteration
  ///
  /// Each iteration utilizes special physics cuts and run condition to find tracks of a particular
  /// class (e.g., fast primary tracks or secondary electron tracks). Hits associated with tracks
  /// reconstructed during current iteration are removed from the further iterations.
  class Iteration {
   public:
    /// \brief Default constructor
    Iteration() = default;

    /// \brief Copy constructor
    Iteration(const Iteration& other) = default;

    /// \brief Copy constructor
    Iteration(const Iteration& other, const std::string& name) : Iteration(other) { SetName(name); }

    /// \brief Move constructor
    Iteration(Iteration&& other) noexcept = default;

    /// \brief Constructor from L1CAIteration type
    Iteration(const std::string& name);

    /// \brief Destructor
    ~Iteration() noexcept = default;

    /// \brief Copy assignment operator
    Iteration& operator=(const Iteration& other) = default;

    /// \brief Move assignment operator
    Iteration& operator=(Iteration&& other) = default;

    /// \brief Checks parameters consistency
    bool Check() const;

    /// \brief Gets doublet chi2 upper cut
    float GetDoubletChi2Cut() const { return fDoubletChi2Cut; }

    /// \brief flag check: electrons/positrons - true, heavy charged - false
    bool GetElectronFlag() const { return fIsElectron; }

    /// \brief Sets flag: true - extends track candidates with unused hits
    bool GetExtendTracksFlag() const { return fIsExtendTracks; }

    /// \brief Gets station index of the first station used in tracking
    int GetFirstStationIndex() const { return fFirstStationIndex; }

    /// \brief Gets flag: true - triplets are also built with skipping <= GetMaxStationGap stations
    int GetMaxStationGap() const { return fMaxStationGap; }

    /// \brief Gets correction for accounting overlaping and iff z
    float GetMaxDZ() const { return fMaxDZ; }

    /// \brief Gets max considered q/p for tracks
    float GetMaxQp() const { return fMaxQp; }

    /// \brief Gets max slope (tx\ty) in 3D hit position of a triplet
    float GetMaxSlope() const { return fMaxSlope; }

    /// \brief Gets max slope (tx\ty) in primary vertex
    float GetMaxSlopePV() const { return fMaxSlopePV; }

    /// \brief Gets min n hits
    int GetMinNhits() const { return fMinNhits; }

    /// \brief Gets min n hits for tracks that start on station 0
    int GetMinNhitsStation0() const { return fMinNhitsStation0; }

    /// \brief Gets the name of the iteration
    const std::string& GetName() const { return fName; }

    /// \brief Gets size of region [TODO: units??] to attach new hits to the created track
    float GetPickGather() const { return fPickGather; }

    /// \brief Checks flag: true - only primary tracks are searched, false - [all or only secondary?]
    bool GetPrimaryFlag() const { return fIsPrimary; }

    /// \brief Gets sigma target position in X direction [cm]
    float GetTargetPosSigmaX() const { return fTargetPosSigmaX; }

    /// \brief Gets sigma target position in Y direction [cm]
    float GetTargetPosSigmaY() const { return fTargetPosSigmaY; }

    /// \brief Gets track chi2 upper cut
    float GetTrackChi2Cut() const { return fTrackChi2Cut; }

    /// (DEBUG!) Sets flag:
    ///   true:
    ///     all the triplets found on this iteration will be converted to tracks,
    ///     all the iterations following after this one will be rejected from the
    ///     iterations sequence;
    ///   false (default):
    ///     tracks are built from triplets, and the minimal amount of hits used in
    ///     each track equals four. In case of primary tracks the first measurement
    ///     is taken from the target, and the other three measurements are taken from
    ///     the triplet.
    bool GetTrackFromTripletsFlag() const { return fIsTrackFromTriplets; }

    /// \brief Gets triplet chi2 upper cut
    float GetTripletChi2Cut() const { return fTripletChi2Cut; }

    /// \brief Gets triplet chi2 upper cut
    float GetTripletFinalChi2Cut() const { return fTripletFinalChi2Cut; }

    /// \brief Gets min value of dp/dp_error, for which two tiplets are neighbours
    float GetTripletLinkChi2() const { return fTripletLinkChi2; }

    /// \brief Sets doublet chi2 upper cut
    void SetDoubletChi2Cut(float input) { fDoubletChi2Cut = input; }

    /// \brief Sets flag: electron tracks - true, heavy ion tracks - false
    void SetElectronFlag(bool flag) { fIsElectron = flag; }

    /// \brief Sets flag: true - extends track candidates with unused hits
    void SetExtendTracksFlag(bool flag) { fIsExtendTracks = flag; }

    /// \brief Sets index of first station used in tracking
    void SetFirstStationIndex(int index) { fFirstStationIndex = index; }

    /// \brief Sets flag: true - triplets are built also skipping <= GetMaxStationGap stations
    void SetMaxStationGap(int nSkipped) { fMaxStationGap = nSkipped; }

    /// \brief TODO: select a more proper name
    void SetMaxDZ(float input) { fMaxDZ = input; }

    /// \brief Sets max considered q/p for tracks
    void SetMaxQp(float input) { fMaxQp = input; }

    /// \brief Sets max slope (tx\ty) in 3D hit position of a triplet
    void SetMaxSlope(float input) { fMaxSlope = input; }

    /// \brief Sets max slope (tx\ty) in primary vertex
    void SetMaxSlopePV(float input) { fMaxSlopePV = input; }

    /// \brief Sets flag: true - skip track candidates with level = 0
    void SetMinNhits(int val) { fMinNhits = val; }

    /// \brief Sets min n hits for tracks that start on station 0
    void SetMinNhitsStation0(int val) { fMinNhitsStation0 = val; }

    /// \brief Sets name of the iteration
    void SetName(const std::string& name) { fName = name; }

    /// \brief Sets size of region [TODO: units??] to attach new hits to the created track
    void SetPickGather(float input) { fPickGather = input; }

    /// \brief Sets flag: primary tracks - true, secondary tracks - false
    void SetPrimaryFlag(bool flag) { fIsPrimary = flag; }

    /// \brief  Sets sigma of target positions in XY plane
    /// \param  sigmaX  Sigma value in X direction [cm]
    /// \param  sigmaX  Sigma value in Y direction [cm]
    void SetTargetPosSigmaXY(float sigmaX, float sigmaY);

    /// \brief Sets track chi2 upper cut
    void SetTrackChi2Cut(float input) { fTrackChi2Cut = input; }

    /// \brief Sets flag:
    ///   true:
    ///     all the triplets found on this iteration will be converted to tracks,
    ///     all the iterations following after this one will be rejected from the
    ///     iterations sequence;
    ///   false (default):
    ///     tracks are built from triplets, and the minimal amount of hits used in
    ///     each track equals four. In case of primary tracks the first measurement
    ///     is taken from the target, and the other three measurements are taken from
    ///     the triplet.
    void SetTrackFromTripletsFlag(bool flag) { fIsTrackFromTriplets = flag; }

    /// \brief Sets triplet chi2 upper cut
    void SetTripletChi2Cut(float input) { fTripletChi2Cut = input; }

    /// \brief Sets triplet chi2 upper cut
    void SetTripletFinalChi2Cut(float input) { fTripletFinalChi2Cut = input; }

    /// \brief Sets min value of dp/dp_error, for which two tiplets are neighbours
    void SetTripletLinkChi2(float input) { fTripletLinkChi2 = input; }

    /// \brief String representation of the class contents
    /// \param indentLevel  Level of indentation for the text (in terms of \t symbols)
    std::string ToString(int indentLevel = 0) const;

    /// \brief  Forms a string, representing a table of iterations from the vector of iterations
    /// \param  vIterations  Vector of iterations
    /// \return Iterations table represented with a string
    static std::string ToTableFromVector(const Vector<Iteration>& vIterations);

   private:
    /** Basic fields **/
    std::string fName{""};  ///< Iteration name

    /** Track finder dependent cuts **/
    // TODO: Iteratively change the literals to floats (S.Zharko)
    // NOTE: For each new cut one should not forget to create a setter and a getter, insert the value
    //       initialization in the copy constructor and the Swap operator as well as a string repre-
    //       sentation to the ToString method (S.Zharko)
    float fTrackChi2Cut        = 10.f;                 ///< Track chi2 upper cut
    float fTripletChi2Cut      = 21.1075f;             ///< Triplet chi2 upper cut
    float fTripletFinalChi2Cut = 21.1075f;             ///< Triplet chi2 upper cut
    float fDoubletChi2Cut      = 11.3449 * 2.f / 3.f;  ///< Doublet chi2 upper cut
    float fPickGather          = 3.0;                  ///< Size of region to attach new hits to the created track
    float fTripletLinkChi2     = 25.0;       ///< Min value of dp^2/dp_error^2, for which two tiplets are neighbours
    float fMaxQp               = 1.0 / 0.5;  ///< Max considered q/p for tracks
    float fMaxSlopePV          = 1.1;        ///< Max slope (tx\ty) in primary vertex
    float fMaxSlope            = 2.748;      ///< Max slope (tx\ty) in 3D hit position of a triplet
    float fMaxDZ               = 0.f;        ///< Correction for accounting overlaping and iff z [cm]
    float fTargetPosSigmaX     = 0;          ///< Constraint on target position in X direction [cm]
    float fTargetPosSigmaY     = 0;          ///< Constraint on target position in Y direction [cm]
    int fFirstStationIndex     = 0;          ///< First station, used for tracking
    int fMinNhits              = 3;          ///< min n hits on the tracks
    int fMinNhitsStation0      = 3;          ///< min n hits for tracks that start on station 0
    bool fIsPrimary            = false;      ///< Flag: true - only primary tracks are searched for
    bool fIsElectron           = false;      ///< Flag: true - only electrons are searched for
    bool fIsExtendTracks       = false;      ///< Flag: true - extends track candidates with unused hits
    int fMaxStationGap         = 0;          ///< Flag: true - find triplets with fMaxStationGap missing stations


    /// @brief Flag to select triplets on the iteration as tracks
    ///   In ordinary cases, the shortest track consists from four hits. For primary track the target is accounted as
    /// the first hit, and the other three hits are taken from the stations. For secondary track all the hits are selected
    /// from the stations only.
    ///   If the fIsTrackFromTriplets flag is turned on, all of the triplets found on this iterations will be considered
    /// as tracks.
    ///
    /// @note The only one iteration with the fIsTrackFromTriplets flag turned on can exist in the tracking iterations
    ///       sequence and this iteration should be the last in the tracking sequence.
    bool fIsTrackFromTriplets = false;

    /// Serialization method, used to save ca::Hit objects into binary or text file in a defined order
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fName;
      ar& fTrackChi2Cut;
      ar& fTripletChi2Cut;
      ar& fTripletFinalChi2Cut;
      ar& fDoubletChi2Cut;
      ar& fPickGather;
      ar& fTripletLinkChi2;
      ar& fMaxQp;
      ar& fMaxSlopePV;
      ar& fMaxSlope;
      ar& fMaxDZ;
      ar& fTargetPosSigmaX;
      ar& fTargetPosSigmaY;
      ar& fFirstStationIndex;
      ar& fMinNhits;
      ar& fMinNhitsStation0;
      ar& fIsPrimary;
      ar& fIsElectron;
      ar& fIsExtendTracks;
      ar& fMaxStationGap;
      ar& fIsTrackFromTriplets;
    }
  };
}  // namespace cbm::algo::ca
