/* Copyright (C) 2024 National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

/**
 * \file CbmTrdGeoFactory.h
 * \brief Builder class for the TRD chamber geometry.
 * \author Alexandru Bercuci <abercuci@niham.nipne.ro>
 * \date 01/10/2023
 *
 * This class manages the relative spatial positioning of the following components of the TRD chamber :
 * - Chamber : initialize and place all components of the TRD chamber
 *    - Radiator :
 *    - Window :
 *    - Volume : 
 *    - BackPanel
 *    - FEE
 */

#ifndef CBMTRDGEOFACTORY_H_
#define CBMTRDGEOFACTORY_H_

#include "CbmTrdDefs.h"  // for trd namespace

#include <FairTask.h>

#include <TString.h>

#include <array>
#include <string>

class TGeoMedium;
class TGeoTranslation;
class TGeoVolume;
class TGeoVolumeAssembly;
using namespace std;
using namespace cbm::trd;
namespace cbm::trd::geo
{
  enum class eException
  {
    invalid_type = 0,  //!
    invalid_id         //!
  };

  /** \struct info_t
   * \brief Information to be storred in the geoManager path.
   * Based on legacy class CbmTrdGeoHandler.
   */
  struct info_t {
    int address   = -1;  //! global addressing of element
    int id        = -1;  //! local (wrt installation) id of element
    int superId   = -1;  //! global (wrt setup) id of element
    int type      = -1;  //! version of element wrt superType family
    int superType = -1;  //! global type wrt TRD design
    int rotation  = -1;  //! rotation angle 0,1,2,3
  };
  /** \function ReadModuleInfo
   * \brief Identify information related to the module encrypted in the geoManager path.
   * Based on legacy function "void CbmTrdGeoHandler::NavigateTo(const TString& path)"
   * \param[in] modTxt : module name in gGeoManager
   * \param[in] info : object to be filled with decrypted info
   */
  bool ReadModuleInfo(const char* modTxt, info_t& info);

  /** \function ReadFebInfo
   * \brief Identify information related to one FEB encrypted in the geoManager path.
   * \param[in] febTxt : FEB name in gGeoManager
   * \param[in] info : object to be filled with decrypted info
   */
  bool ReadFebInfo(const char* febTxt, info_t& info);

  /** \function WriteModuleInfo
   * \brief Put up information related to the module and store it in the geoManager path.
   * Based on legacy version "void Create_TRD_Geometry.C macro"
   */
  int WriteModuleInfo(info_t* info);

  /** \function WriteFebInfo
   * \brief Put up information related to the FEB and store it in the geoManager path.
   */
  int WriteFebInfo(info_t* info);


  // chamber generic geometrical definitions
  static double sizeX, sizeY;                                  //! total area
  static double activeAreaX, activeAreaY;                      //! active area
  static map<const string, const TGeoMedium*> fMaterial = {};  //! material mapping (name, value)
  const TGeoMedium* GetMaterial(const char* mname);


  /** \brief Generic Chamber builder 
  **/
  class ChamberBuilder : public FairTask {
   public:
    enum class eGeoPart : int
    {
      kRadiator = 0,
      kWindow,
      kVolume,
      kBackPanel,
      kFEB,
      kNparts
    };
    enum eConfig
    {
      kRadiator = 8  //! include radiator in chamber geometry
        ,
      kFEB  //! include FEB in chamber geometry
    };
    class Component;
    class Radiator;
    class Window;
    class Volume;
    class BackPanel;
    class FEB;
    /** \brief Constructor for the chamber. Adds all elements according to config.       
     * \param[in] typ TRD chamber type [1, 3, 5, 7].
    **/
    ChamberBuilder(int typ = 1);

    TGeoVolume* GetModule() const { return fVol; }
    bool HasFEB() const { return TESTBIT(fConfig, eConfig::kFEB); }
    bool HasRadiator() const { return TESTBIT(fConfig, eConfig::kRadiator); }
    /** \brief Init task **/
    virtual InitStatus Init();
    /** \brief Executed task **/
    virtual void Exec(Option_t*);
    /** \brief Finish task **/
    virtual void Finish() { ; }

    void SetFEB(bool feb = true) { feb ? SETBIT(fConfig, eConfig::kFEB) : CLRBIT(fConfig, eConfig::kFEB); }
    void SetRadiator(bool rad = true)
    {
      rad ? SETBIT(fConfig, eConfig::kRadiator) : CLRBIT(fConfig, eConfig::kRadiator);
    }

   private:
    ChamberBuilder(const ChamberBuilder&);
    ChamberBuilder operator=(const ChamberBuilder&);

    short fConfig                                         = 0;   //! bit map of the setter flags
    short fChmbTyp                                        = 1;   //! chamber type [1, 3, 5, 7]
    array<Component*, (int) eGeoPart::kNparts> fComponent = {};  //! list of chamber component  builders
    static const int Nfebs                                = faspFeb[1].nmax;
    const double feb_pos[Nfebs][2] = {{-18, -21.6}, {0, -21.6},  {18, -21.6},  {-18, -10.8}, {0, -10.8},
                                      {18, -10.8},  {-18, 0.0},  {0, 0.0},     {18, 0.0},    {-18, +10.8},
                                      {0, +10.8},   {18, +10.8}, {-18, +21.6}, {0, +21.6},   {18, +21.6}};
    TGeoVolume* fVol               = nullptr;  //! the geo volume itself
    ClassDef(ChamberBuilder, 1)                // Manager of the TRD support structure
  };


  /** \brief Generic sub-component 
  **/
  class ChamberBuilder::Component : public FairTask {
   public:
    friend class ChamberBuilder;
    /** \brief Constructor of the TRD chamber component.
      * It links the chamber class
    **/
    Component(const char* name) : FairTask(name)
    {
      sizeX       = 0;
      sizeY       = 0;
      activeAreaX = 0;
      activeAreaY = 0;
    }
    /** Construct the chamber component volume in the reference of the chamber
      * The function has to be implemented for each particular component of the TRD chamber
      * \return The dimension of the component on the z axis  
      */
    /** \brief Init task **/
    virtual InitStatus Init() = 0;
    /** \brief Executed task **/
    virtual void Exec(Option_t*) { ; }
    /** \brief Finish task **/
    virtual void Finish() { ; }
    virtual double GetCenter() const;
    virtual double GetHeight() const { return fHeight; }

    static const char* fgName[(int) eGeoPart::kNparts];

   protected:
    TGeoVolume* fVol = nullptr;  //! the geo volume itself

   private:
    Component(const Component&);
    //Component operator=(const Component&);

    double fHeight = 0;                     //! height of the component (local calculation)
    ClassDef(ChamberBuilder::Component, 1)  // Model for the TRD generic chamber component builder
  };

  class ChamberBuilder::Radiator : public ChamberBuilder::Component {
   public:
    Radiator();
    /** \brief Init task **/
    virtual InitStatus Init();

   private:
    Radiator(const Radiator&);
    const double radiator_thickness = 30.0;
    ClassDef(ChamberBuilder::Radiator, 1)  // Model for the TRD radiator
  };

  class ChamberBuilder::Window : public ChamberBuilder::Component {
   public:
    /** \brief Constructor of entrance window for the TRD chamber
    **/
    Window();
    /** \brief Init task **/
    virtual InitStatus Init();

   private:
    Window(const Window&);

    const double winIn_C_thickness    = 0.012;  //! 100um C foil + 25um KaptonAl
    const double winIn_HC_thickness   = 0.9;    //! 9mm HC structure
    const double WIN_FrameX_thickness = 0.5;    //! entrance window framing 5x9 mm2
    const double WIN_FrameY_thickness = 0.6;    //! entrance window framing 5x9 mm2
    const double WIN_OutX_thickness   = 0.35;   //! outside framing at win 3.5x9 mm2
    const double WIN_OutY_thickness   = 0.30;   //! outside framing at win 3.5x9 mm2
    ClassDef(ChamberBuilder::Window, 1)         // Model for the TRD2D entrance window
  };

  /** \brief Inner class describing a :
  **/
  class ChamberBuilder::Volume : public ChamberBuilder::Component {
   public:
    /** \brief Constructor.
    **/
    Volume();
    /** \brief Init task **/
    virtual InitStatus Init();

   private:
    Volume(const Volume&);

    const double gas_extra          = 0.6;                 //! extra volume of gas parallel to wires
    const double gas_thickness      = 1.2;                 //! active volume thickness
    const double ridge_height       = 0.29;                //! closure to pad-plane dimension
    const double ledge_thickness    = gas_thickness / 3.;  //! ledge thickness supporting A/K wires
    const double cathode_width      = 1.0;                 //! cathode
    const double anode_width        = 0.65;                //! anode
    const double dist_width         = 0.40;                //! distance from anode to PP
    const double WIN_OutX_thickness = 0.35;                //! outside framing
    const double WIN_OutY_thickness = 0.30;                //! half of outside framing
    ClassDef(ChamberBuilder::Volume, 1)                    // Model for the TRD active volume
  };

  /** \brief Inner class describing the back panel of composed of
   * - pad plane
   * - honey-comb structure for reinforcement
   * electric shielding
  **/
  class ChamberBuilder::BackPanel : public ChamberBuilder::Component {
   public:
    /** \brief Constructor.
    **/
    BackPanel();
    /** \brief Init task **/
    virtual InitStatus Init();

   private:
    BackPanel(const BackPanel&);
    TGeoTranslation* tr = nullptr;
    TString sexpr       = "";

    const double pp_pads_thickness  = 0.0020;  //! cu coverage of PP PCB
    const double pp_pcb_thickness   = 0.0230;  //! PP support PCB thickness
    const double hc_thickness       = 2.30;    //! Honneycomb backpanel support thickness
    const double hc_unitx           = 6.0;     //! area opearted by one FASP 6 x 2.7 cm2
    const double hc_unity           = 2.7;     //! area opearted by one FASP 6 x 2.7 cm2
    const double hc_holex           = 2.4;     //! dimension of flat-cable hole x-dimension (along wires)
    const double hc_holey           = 0.8;     //! dimension of flat-cable hole x-dimension (along wires)
    const double cu_pcb_thickness   = 0.0150;  //! Electric shield PCB support thickness
    const double cu_thickness       = 0.0020;  //! Electric shield Cu covarage thickness
    const double BKP_Frame_width    = 1.;      //! Global width of the perimetral frame (including indentation)
    const double BKP_Frame_closure  = 0.25;    //! Perimetral frame indentation
    const double BKP_OutX_thickness = 0.50;    //! outside framing
    const double BKP_OutY_thickness = 0.45;    //! outside framing
    const double BKP_OutY_correct   = 0.15;    //! framing overlap between th BP and frame

    ClassDef(ChamberBuilder::BackPanel, 1)  // Model for the TRD back panel
  };
  /** \brief Inner class describing the geometry of the TRD Front End Electronics (FEE):
  **/
  class ChamberBuilder::FEB : public ChamberBuilder::Component {
   public:
    /** \brief Constructor.
    **/
    FEB();
    /** \brief Init task **/
    virtual InitStatus Init();

   private:
    FEB(const FEB&);
    //ChamberBuilder::FEB operator=(const FEB&);
    const double FASP_x    = 1.10;    //!
    const double FASP_y    = 1.10;    //!
    const double FASP_z    = 0.10;    //!
    const double FPGA_x    = 2.20;    //!
    const double FPGA_y    = 2.20;    //!
    const double FPGA_z    = 0.18;    //!
    const double ADC_x     = 0.9;     //!
    const double ADC_y     = 1.5;     //!
    const double ADC_z     = 0.1;     //!
    const double DCDC_x    = 1.5012;  //!
    const double DCDC_y    = 0.8992;  //!
    const double DCDC_z    = 0.4319;  //!
    const double ConnFC_x  = 2.37;    //!
    const double ConnFC_y  = 0.535;   //!
    const double ConnFC_z  = 0.266;   //!
    const double ConnBRG_x = 0.5;     //!
    const double ConnBRG_y = 4.58;    //!
    const double ConnBRG_z = 0.658;   //!

    static const int FASPRO_Nly                = 18;    //!
    static const int FASPRO_Nfasp              = 12;    //!
    static const int FASPRO_Nadc               = 6;     //!
    static const int FASPRO_Nfpga              = 3;     //!
    static const int FASPRO_Ndcdc              = 3;     //!
    const double FASPRO_zspace                 = 1.0;   //! gap size between boards
    const double FASPRO_length                 = 17.8;  //! length of FASP FEBs in cm
    const double FASPRO_width                  = 10.6;  //! width of FASP FEBs in cm
    const double FASPRO_hole_x                 = 2.2;   //!
    const double FASPRO_hole_y                 = 0.4;   //!
    const double FASPRO_ly_cu[FASPRO_Nly][2]   = {      // FASPRO(Cu) layer thickness [um] and covarage [%]
                                                {54, 95}, {34, 10}, {16, 95}, {16, 10}, {16, 95}, {34, 10},
                                                {16, 95}, {16, 10}, {16, 95}, {16, 95}, {16, 10}, {16, 95},
                                                {34, 10}, {16, 95}, {16, 10}, {16, 95}, {34, 10}, {54, 95}};
    const double FASPRO_ly_pcb[FASPRO_Nly - 1] = {// FASPRO(FR4) layer thickness [um]
                                                  100, 133, 100, 127, 100, 133, 100, 127, 100,
                                                  127, 100, 133, 100, 127, 100, 133, 100};
    const double HOLE_pos[FASPRO_Nfasp][2]     = {{-6, -3.55}, {-6, -1.55}, {-6, 1.55}, {-6, 3.55},
                                              {0, -3.55},  {0, -1.55},  {0, 1.55},  {0, 3.55},
                                              {+6, -3.55}, {+6, -1.55}, {+6, 1.55}, {+6, 3.55}};
    const double FASP_pos[FASPRO_Nfasp][2]     = {{-6, -4.5}, {-6, -2.5}, {-6, +2.5}, {-6, +4.5}, {0, -4.5},  {0, -2.5},
                                              {0, +2.5},  {0, +4.5},  {+6, -4.5}, {+6, -2.5}, {+6, +2.5}, {+6, +4.5}};
    const double ADC_pos[FASPRO_Nadc][2]       = {{-4.15, -3.35}, {1.85, -3.35}, {7.85, -3.35},
                                            {-4.15, +3.35}, {1.85, +3.35}, {7.85, +3.35}};
    const double FPGA_pos[FASPRO_Nfpga][2]     = {{-6, 0}, {0, 0}, {+6, 0}};
    const double DCDC_pos[FASPRO_Ndcdc][2]     = {{-3, 0.1}, {3, -1.2}, {2.89, 0.1}};
    const double ConnFC_pos[FASPRO_Nfasp][2]   = {{-6, -4.9}, {-6, -2.9}, {-6, 2.9},  {-6, 4.9},  {0, -4.9}, {0, -2.9},
                                                {0, 2.9},   {0, 4.9},   {+6, -4.9}, {+6, -2.9}, {+6, 2.9}, {+6, 4.9}};
    const double ConnBRG_pos[2][2]             = {{-8.4, 0}, {+8.4, 0}};

    ClassDef(ChamberBuilder::FEB, 1)  // Model for the TRD FEB geometry
  };
}  // namespace cbm::trd::geo
#endif  // CBMTRDGEOFACTORY_H_
