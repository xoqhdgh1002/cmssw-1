// Package:    SinglePhotonJetPlusHOFilter
// Class:      SinglePhotonJetPlusHOFilter
// 
/*
 Description: [one line class summary]
Skimming of SinglePhoton data set for the study of HO absolute weight calculation
* Skimming Efficiency : ~ 2 %
 Implementation:
     [Notes on implementation]
     For Secondary Datasets (SD)
*/
//
// Original Author:  Gobinda Majumder & Suman Chatterjee
//         Created:  Fri July 29 14:52:17 IST 2016
// $Id$
//
//


// system include files
#include <memory>
#include <string>

#include <iostream>
#include <fstream>

// class declaration
//

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/ParticleFlowReco/interface/PFClusterFwd.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/EgammaCandidates/interface/PhotonFwd.h"


using namespace std;
using namespace edm;
using namespace reco;


class SinglePhotonJetPlusHOFilter : public edm::EDFilter {
   public:
      explicit SinglePhotonJetPlusHOFilter(const edm::ParameterSet&);
      ~SinglePhotonJetPlusHOFilter();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      double PhiInRange(const double& phi);
      double delta2R(double eta1, double phi1, double eta2, double phi2);
      virtual void beginJob() ;
      virtual bool filter(edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      
      virtual bool beginRun(edm::Run&, edm::EventSetup const&);
      virtual bool endRun(edm::Run&, edm::EventSetup const&);
      virtual bool beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);
      virtual bool endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&);

      // ----------member data ---------------------------
  int Nevt;
  int Njetp;
  int Npass;
  double jtptthr;
  double jtetath;
  double hothres;
  double pho_Ptcut;
  bool isOthHistFill;

  edm::EDGetTokenT<reco::PFJetCollection> tok_PFJets_;
  edm::EDGetTokenT<reco::PFClusterCollection> tok_hoht_;   
  edm::EDGetTokenT<edm::View<reco::Photon> >tok_photons_;

};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
SinglePhotonJetPlusHOFilter::SinglePhotonJetPlusHOFilter(const edm::ParameterSet& iConfig) { 

  tok_PFJets_ = consumes<reco::PFJetCollection>( iConfig.getParameter<edm::InputTag>("PFJets"));
  tok_hoht_ = consumes<reco::PFClusterCollection>( iConfig.getParameter<edm::InputTag>("particleFlowClusterHO"));
  tok_photons_ = consumes<edm::View<reco::Photon> >  ( iConfig.getParameter<edm::InputTag>("Photons")); 

   //now do what ever initialization is needed
  jtptthr = iConfig.getUntrackedParameter<double>("Ptcut", 90.0);
  jtetath = iConfig.getUntrackedParameter<double>("Etacut",1.5);
  hothres = iConfig.getUntrackedParameter<double>("HOcut", 5);
  pho_Ptcut = iConfig.getUntrackedParameter<double>("Pho_Ptcut",120) ;

  Nevt = 0;
  Njetp=0;
  Npass=0;
}

SinglePhotonJetPlusHOFilter::~SinglePhotonJetPlusHOFilter()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}

// ------------ method called on each new Event  ------------
bool
SinglePhotonJetPlusHOFilter::filter(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  using namespace std;
  using namespace edm;
  using namespace reco;  

  Nevt++;

  edm::Handle<reco::PFJetCollection> PFJets;
  iEvent.getByToken(tok_PFJets_,PFJets);   
  bool passed=false;
  vector <pair<double, double> > jetdirection;
  vector<double> jetspt; 
  if (PFJets.isValid()) {
    for (unsigned jet = 0; jet<PFJets->size(); jet++) {
      if(((*PFJets)[jet].pt()<jtptthr)||(abs((*PFJets)[jet].eta())>jtetath)) continue;
      
      std::pair<double, double> etaphi((*PFJets)[jet].eta(),(*PFJets)[jet].phi());
      jetdirection.push_back(etaphi);
      jetspt.push_back((*PFJets)[jet].pt());
      passed = true;
    }
  }
  if (!passed) return passed; 
  
  edm::Handle<edm::View<reco::Photon> > photons;
  iEvent.getByToken(tok_photons_, photons); 
  bool pho_passed=false ;
  vector <pair<double,double> > phodirection ;
  vector<double> phopT ;
  
  if(photons.isValid()){
    
    edm::View<reco::Photon>::const_iterator gamma1;
    for( gamma1 = photons->begin(); gamma1 != photons->end(); gamma1++ ) {
      if(gamma1->pt()<pho_Ptcut) continue ;
      std::pair<double,double> etaphi_pho(gamma1->eta(),gamma1->phi());
      phodirection.push_back(etaphi_pho) ;
      phopT.push_back(gamma1->pt()) ;
      
      pho_passed  =true ;
    }
  }
 
  if (!pho_passed) return false;
  Njetp++;
  bool isJetDir=false;

  edm::Handle<PFClusterCollection> hoht;
  iEvent.getByToken(tok_hoht_,hoht);
  if (hoht.isValid()) {
    if ((*hoht).size()>0) {
      for (unsigned ijet = 0; ijet< jetdirection.size(); ijet++) {
	
	bool matched=false;
	for (unsigned iph=0; iph<phodirection.size(); iph++) { 
	  if(abs(PhiInRange(phodirection[iph].second-jetdirection[ijet].second))>2.0) {
	    matched=true; break;
	  }
	}
	if (matched) { 
	  for (PFClusterCollection::const_iterator ij=(*hoht).begin(); ij!=(*hoht).end(); ij++){
	    double hoenr = (*ij).energy();
	    if (hoenr <hothres) continue;
	    
	    const math::XYZPoint&  cluster_pos = ij->position();
	    
	    double hoeta = cluster_pos.eta() ;
	    double hophi = cluster_pos.phi() ;
	    
	    double delta = delta2R(jetdirection[ijet].first, jetdirection[ijet].second, hoeta, hophi);
	    if (delta <0.5) { 
	      isJetDir=true;  break;
	    }
	  }
	}
	if (isJetDir) { break;}
      }
    }
  }

  if (isJetDir) {Npass++;}
  
  return isJetDir;

}

// ------------ method called once each job just before starting event loop  ------------
void 
SinglePhotonJetPlusHOFilter::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
SinglePhotonJetPlusHOFilter::endJob() {
  cout<<"End of SinglePhotonJetPlusHOFilter with event "<<Nevt<<" Jetpassed "<< Njetp<<" passed "<<Npass<<endl;

}

// ------------ method called when starting to processes a run  ------------
bool 
SinglePhotonJetPlusHOFilter::beginRun(edm::Run&, edm::EventSetup const&)
{ 
  return true;
}

// ------------ method called when ending the processing of a run  ------------
bool 
SinglePhotonJetPlusHOFilter::endRun(edm::Run&, edm::EventSetup const&)
{
  return true;
}

// ------------ method called when starting to processes a luminosity block  ------------
bool 
SinglePhotonJetPlusHOFilter::beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&)
{
  return true;
}

// ------------ method called when ending the processing of a luminosity block  ------------
bool 
SinglePhotonJetPlusHOFilter::endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&)
{
  return true;
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
SinglePhotonJetPlusHOFilter::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

double SinglePhotonJetPlusHOFilter::PhiInRange(const double& phi) {
  double phiout = phi;
  
  if( phiout > 2*M_PI || phiout < -2*M_PI) {
    phiout = fmod( phiout, 2*M_PI);
  }
  if (phiout <= -M_PI) phiout += 2*M_PI;
  else if (phiout >  M_PI) phiout -= 2*M_PI;
  
  return phiout;
}

double SinglePhotonJetPlusHOFilter::delta2R(double eta1, double phi1, double eta2, double phi2) {
  return sqrt(pow(eta1 - eta2,2) +pow(PhiInRange(phi1 - phi2),2));
}


//define this as a plug-in
DEFINE_FWK_MODULE(SinglePhotonJetPlusHOFilter);

/*

End of SinglePhotonJetPlusHOFilter with event 100 Jetpassed 16 passed 3


*/
