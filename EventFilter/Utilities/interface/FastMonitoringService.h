#ifndef EvFFastMonitoringService_H
#define EvFFastMonitoringService_H 1


#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/LuminosityBlockID.h"
#include "DataFormats/Provenance/interface/Timestamp.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/ParameterSetID.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "boost/filesystem.hpp"

#include "EventFilter/Utilities/interface/MicroStateService.h"
#include "EventFilter/Utilities/interface/FastMonitoringThread.h"

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_map>

/*Description
  this is an evolution of the MicroStateService intended to be run standalone in cmsRun or similar
  As such, it has to independently create a monitoring thread and run it in each forked process, which needs 
  to be arranged following the standard CMSSW procedure.
  We try to use boost threads for uniformity with the rest of the framework, even if they suck a bit.
  A legenda for use by the monitoring process in the DAQ needs to be generated as soon as convenient - since 
  no access to the EventProcessor is granted, this needs to wait until after beginJob is executed.
  At the same time, we try to spare time in the monitoring by avoiding even a single string lookup and using the 
  moduledesc pointer to key into the map instead.
  As a bonus, we can now add to the monitored status the current path (and possibly associate modules to a path...)
  this intermediate info will be called "ministate" :D
  The general counters and status variables (event number, number of processed events, number of passed and stored 
  events, luminosity section etc. are also monitored here.

  NOTA BENE!!! with respect to the MicroStateService, no string or string pointers are used for the microstates.
  NOTA BENE!!! the state of the edm::EventProcessor cannot be monitored directly from within a service, so a 
  different solution must be identified for that (especially one needs to identify error states). 
  NOTA BENE!!! to keep backward compatibility with the MicroStateService, a common base class with abstract interface,
  exposing the single  method to be used by all other packages (except EventFilter/Processor, 
  which should continue to use the concrete class interface) will be defined 

*/
class FedRawDataInputSource;

namespace edm {
  class ConfigurationDescriptions;
}


namespace evf{

  template<typename T>
    struct ContainableAtomic {
    ContainableAtomic(): m_value{} {}
    ContainableAtomic(T iValue): m_value(iValue) {}
    ContainableAtomic(ContainableAtomic<T> const& iOther): m_value(iOther.m_value.load()) {}
    ContainableAtomic<T>& operator=(const void* iValue) {
      m_value.store(iValue,std::memory_order_relaxed);
      return *this;
    }
    operator T() { return m_value.load(std::memory_order_relaxed); }
    
    std::atomic<T> m_value;
  };

  class FastMonitoringService : public MicroStateService
    {
      struct Encoding
      {
	Encoding(unsigned int res): reserved_(res), current_(reserved_), currentReserved_(0)
	{
	  if (reserved_)
	    dummiesForReserved_ = new edm::ModuleDescription[reserved_];
	  //	  completeReservedWithDummies();
	}
	~Encoding()
	{
	  if (reserved_)
	    delete[] dummiesForReserved_;
	}
	//trick: only encode state when sending it over (i.e. every sec)
	int encode(const void *add){
	  std::unordered_map<const void *, int>::const_iterator it=quickReference_.find(add);
	  return (it!=quickReference_.end()) ? (*it).second : 0;
	}
	const void* decode(unsigned int index){return decoder_[index];}
	void fillReserved(const void* add, unsigned int i){
	  //	  translation_[*name]=current_; 
	  quickReference_[add]=i; 
	  if(decoder_.size()<=i)
	    decoder_.push_back(add);
	  else
	    decoder_[currentReserved_] = add;
	}
	void updateReserved(const void* add){
	  fillReserved(add,currentReserved_);
	  currentReserved_++;
	}
	void completeReservedWithDummies()
	{
	  for(unsigned int i = currentReserved_; i<reserved_; i++)
	    fillReserved(dummiesForReserved_+i,i);
	}
	void update(const void* add){
	  //	  translation_[*name]=current_; 
	  quickReference_[add]=current_; 
	  decoder_.push_back(add); 
	  current_++;
	}
	unsigned int vecsize() {
	  return decoder_.size();
	}
	std::unordered_map<const void *,int> quickReference_;
	std::vector<const void *> decoder_;
	unsigned int reserved_;
	int current_;
	int currentReserved_;
	edm::ModuleDescription *dummiesForReserved_;
      };
    public:

      // the names of the states - some of them are never reached in an online app
      static const std::string macroStateNames[FastMonitoringThread::MCOUNT];
      static const std::string inputStateNames[FastMonitoringThread::inCOUNT]; 
      // Reserved names for microstates
      // moved into base class in EventFilter/Utilities for compatibility with MicroStateServiceClassic
      static const std::string nopath_;
      FastMonitoringService(const edm::ParameterSet&,edm::ActivityRegistry&);
      ~FastMonitoringService();
      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
     
      std::string makePathLegendaJson();
      std::string makeModuleLegendaJson();
      std::string makeInputLegendaJson();

      void preallocate(edm::service::SystemBounds const&);
      void jobFailure();
      void preBeginJob(edm::PathsAndConsumesOfModulesBase const&,edm::ProcessContext const& pc);

      void preModuleBeginJob(edm::ModuleDescription const&);
      void postBeginJob();
      void postEndJob();

      void postGlobalBeginRun(edm::GlobalContext const&);
      void preGlobalBeginLumi(edm::GlobalContext const&);
      void preGlobalEndLumi(edm::GlobalContext const&);
      void postGlobalEndLumi(edm::GlobalContext const&);

      void preStreamBeginLumi(edm::StreamContext const&);
      void postStreamBeginLumi(edm::StreamContext const&);
      void preStreamEndLumi(edm::StreamContext const&);
      void postStreamEndLumi(edm::StreamContext const&);
      void prePathEvent(edm::StreamContext const&, edm::PathContext const&);
      void preEvent(edm::StreamContext const&);
      void postEvent(edm::StreamContext const&);
      void preSourceEvent(edm::StreamID);
      void postSourceEvent(edm::StreamID);
      void preModuleEvent(edm::StreamContext const&, edm::ModuleCallingContext const&);
      void postModuleEvent(edm::StreamContext const&, edm::ModuleCallingContext const&);
      void preStreamEarlyTermination(edm::StreamContext const&, edm::TerminationOrigin);
      void preGlobalEarlyTermination(edm::GlobalContext const&, edm::TerminationOrigin);
      void preSourceEarlyTermination(edm::TerminationOrigin);
      void setExceptionDetected(unsigned int ls);

      //this is still needed for use in special functions like DQM which are in turn framework services
      void setMicroState(MicroStateService::Microstate);
      void setMicroState(edm::StreamID, MicroStateService::Microstate);

      void accumulateFileSize(unsigned int lumi, unsigned long fileSize);
      void startedLookingForFile();
      void stoppedLookingForFile(unsigned int lumi);
      void reportLockWait(unsigned int ls, double waitTime, unsigned int lockCount);
      unsigned int getEventsProcessedForLumi(unsigned int lumi, bool * abortFlag=nullptr);
      bool getAbortFlagForLumi(unsigned int lumi);
      bool shouldWriteFiles(unsigned int lumi, unsigned int* proc=nullptr)
      {
        unsigned int processed = getEventsProcessedForLumi(lumi);
        if (proc) *proc = processed;
        return !getAbortFlagForLumi(lumi) && (processed || emptyLumisectionMode_);
      }
      std::string getRunDirName() const { return runDirectory_.stem().string(); }
      void setInputSource(FedRawDataInputSource *inputSource) {inputSource_=inputSource;}
      void setInState(FastMonitoringThread::InputState inputState) {inputState_=inputState;}
      void setInStateSup(FastMonitoringThread::InputState inputState) {inputSupervisorState_=inputState;}

    private:

      void doSnapshot(const unsigned int ls, const bool isGlobalEOL);

      void doStreamEOLSnapshot(const unsigned int ls, const unsigned int streamID) {
	//pick up only event count here
	fmt_.jsonMonitor_->snapStreamAtomic(ls,streamID);
      }

      void dowork() { // the function to be called in the thread. Thread completes when function returns.
        monInit_.exchange(true,std::memory_order_acquire);
	while (!fmt_.m_stoprequest) {
	  edm::LogInfo("FastMonitoringService") << "Current states: Ms=" << fmt_.m_data.fastMacrostateJ_.value()
	            << " ms=" << encPath_[0].encode(ministate_[0])
	            << " us=" << encModule_.encode(microstate_[0])
	            << " is=" << inputStateNames[inputState_]
	            << " iss="<< inputStateNames[inputSupervisorState_]
                    << std::endl;

	  {
            std::lock_guard<std::mutex> lock(fmt_.monlock_);

            doSnapshot(lastGlobalLumi_,false);

            if (fastMonIntervals_ && (snapCounter_%fastMonIntervals_)==0) {
              if (filePerFwkStream_) {
                std::vector<std::string> CSVv;
                for (unsigned int i=0;i<nStreams_;i++) {
                  CSVv.push_back(fmt_.jsonMonitor_->getCSVString((int)i));
                }
                fmt_.monlock_.unlock();
                for (unsigned int i=0;i<nStreams_;i++) {
                  if (CSVv[i].size())
                    fmt_.jsonMonitor_->outputCSV(fastPathList_[i],CSVv[i]);
                }
              }
              else {
                std::string CSV = fmt_.jsonMonitor_->getCSVString();
                //release mutex before writing out fast path file
                fmt_.monlock_.unlock();
                if (CSV.size())
                  fmt_.jsonMonitor_->outputCSV(fastPath_,CSV);
              }
            }

            snapCounter_++;
            
          }
	  ::sleep(sleepTime_);
	}
      }

      //the actual monitoring thread is held by a separate class object for ease of maintenance
      FastMonitoringThread fmt_;
      Encoding encModule_;
      std::vector<Encoding> encPath_;
      FedRawDataInputSource * inputSource_ = nullptr;
      std::atomic<FastMonitoringThread::InputState> inputState_           { FastMonitoringThread::InputState::inInit };
      std::atomic<FastMonitoringThread::InputState> inputSupervisorState_ { FastMonitoringThread::InputState::inInit };

      unsigned int nStreams_;
      unsigned int nThreads_;
      int sleepTime_;
      unsigned int fastMonIntervals_;
      unsigned int snapCounter_ = 0;
      std::string microstateDefPath_, fastMicrostateDefPath_;
      std::string fastName_, fastPath_, slowName_;
      bool filePerFwkStream_;

      //variables that are used by/monitored by FastMonitoringThread / FastMonitor

      std::map<unsigned int, timeval> lumiStartTime_;//needed for multiplexed begin/end lumis
      timeval fileLookStart_, fileLookStop_;//this could also be calculated in the input source

      unsigned int lastGlobalLumi_;
      std::queue<unsigned int> lastGlobalLumisClosed_;
      bool isGlobalLumiTransition_;
      std::atomic<bool> isInitTransition_;
      unsigned int lumiFromSource_;

      //global state
      std::atomic<FastMonitoringThread::Macrostate> macrostate_;

      //per stream
      std::vector<ContainableAtomic<const void*>> ministate_;
      std::vector<ContainableAtomic<const void*>> microstate_;
      std::vector<ContainableAtomic<const void*>> threadMicrostate_;

      //variables measuring source statistics (global)
      //unordered_map is not used because of very few elements stored concurrently
      std::map<unsigned int, double> avgLeadTime_;
      std::map<unsigned int, unsigned int> filesProcessedDuringLumi_;
      //helpers for source statistics:
      std::map<unsigned int, unsigned long> accuSize_;
      std::vector<double> leadTimes_;
      std::map<unsigned int, std::pair<double,unsigned int>> lockStatsDuringLumi_;

      //for output module
      std::map<unsigned int, std::pair<unsigned int,bool>> processedEventsPerLumi_;

      //flag used to block EOL until event count is picked up by caches (not certain that this is really an issue)
      //to disable this behavior, set #ATOMIC_LEVEL 0 or 1 in DataPoint.h
      std::vector<std::atomic<bool>*> streamCounterUpdating_;

      std::vector<unsigned long> firstEventId_;
      std::vector<std::atomic<bool>*> collectedPathList_;
      std::vector<ContainableAtomic<unsigned int>> eventCountForPathInit_;
      std::vector<bool> pathNamesReady_;

      boost::filesystem::path workingDirectory_, runDirectory_;

      bool threadIDAvailable_ = false;

      std::atomic<unsigned long> totalEventsProcessed_;

      std::string moduleLegendFile_;
      std::string moduleLegendFileJson_;
      std::string pathLegendFile_;
      std::string pathLegendFileJson_;
      std::string inputLegendFileJson_;
      bool pathLegendWritten_ = false;
      unsigned int nOutputModules_ =0;

      std::atomic<bool> monInit_;
      bool exception_detected_ = false;
      std::vector<unsigned int> exceptionInLS_;
      bool emptyLumisectionMode_ = false;
      std::vector<std::string> fastPathList_;

    };

}

#endif
