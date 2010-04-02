#ifndef DQM_L1TMONITORCLIENT_L1TGCTCLIENT_H
#define DQM_L1TMONITORCLIENT_L1TGCTCLIENT_H

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include <FWCore/Framework/interface/EDAnalyzer.h>
#include "DQMServices/Core/interface/DQMStore.h"
#include "DQMServices/Core/interface/MonitorElement.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

class L1TGCTClient: public edm::EDAnalyzer {

 public:

  /// Constructor
  L1TGCTClient(const edm::ParameterSet& ps);
  
  /// Destructor
  virtual ~L1TGCTClient();
 
 protected:

  /// BeginJob
  void beginJob(void);

  /// BeginRun
  void beginRun(const edm::Run& r, const edm::EventSetup& c);

  /// Analyze
  void analyze(const edm::Event& e, const edm::EventSetup& c) ;

  void beginLuminosityBlock(const edm::LuminosityBlock& lumiSeg, 
                            const edm::EventSetup& context) ;

  /// DQM Client Diagnostic
  void endLuminosityBlock(const edm::LuminosityBlock& lumiSeg, 
                          const edm::EventSetup& c);

  /// EndRun
  void endRun(const edm::Run& r, const edm::EventSetup& c);

  /// Endjob
  void endJob();

 private:

  DQMStore* dbe_;   
  std::string monitorDir_; 
  int counterLS_;      ///counter 
  int counterEvt_;     ///counter 
  int prescaleLS_;     ///units of lumi sections 
  int prescaleEvt_;    ///prescale on number of events

  MonitorElement *l1GctIsoEmHotChannelEtaPhiMap_; 
  MonitorElement *l1GctIsoEmDeadChannelEtaPhiMap_;
  MonitorElement *l1GctNonIsoEmHotChannelEtaPhiMap_;
  MonitorElement *l1GctNonIsoEmDeadChannelEtaPhiMap_; 
  MonitorElement *l1GctAllJetsHotChannelEtaPhiMap_;
  MonitorElement *l1GctAllJetsDeadChannelEtaPhiMap_;
  MonitorElement *l1GctTauJetsHotChannelEtaPhiMap_;
  MonitorElement *l1GctTauJetsDeadChannelEtaPhiMap_;

};

#endif
