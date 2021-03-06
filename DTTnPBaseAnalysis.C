#include "DTTnPBaseAnalysis.h"

DTTnPBaseAnalysis::DTTnPBaseAnalysis(const std::string & configFile) 
{

  pharseConfig(configFile);
  TFile *fileIn = new TFile(m_sampleConfig.fileName,"read");

  TTree * tree = 0;
  fileIn->GetObject("DTTree",tree);

  fileIn->GetObject("triggerFilterNames",m_triggerFilterNames);

  m_hltFilterId = -1;
  Int_t hltFilterNameId = 0;
  
  for (const auto & filterName : (*m_triggerFilterNames))
    {
      if (filterName.find(m_tnpConfig.tag_hltFilter) != std::string::npos)
	{
	  m_hltFilterId = hltFilterNameId;
	  break;
	}
      hltFilterNameId++;
    }

  if (m_hltFilterId>=0)
    std::cout << "[DTTnPBaseAnalysis::DTTnPBaseAnalysis] Found match for HLT filter : " 
	      << m_triggerFilterNames->at(m_hltFilterId) << std::endl;
  else
    std::cout << "[DTTnPBaseAnalysis::DTTnPBaseAnalysis] Not found match for any HLT filter in ntuples, TnP will fail!"  << std::endl;

  Init(tree);

}

void DTTnPBaseAnalysis::pharseConfig(const std::string & configFile)
{

  boost::property_tree::ptree pt;
  
  try
    {
      boost::property_tree::ini_parser::read_ini(configFile, pt);
    }
  catch (boost::property_tree::ini_parser::ini_parser_error iniParseErr)
    {
      std::cout << "[DTTnPBaseAnalysis::pharseConfig] Can't open : " 
		<< iniParseErr.filename()
		<< "\n\tin line : " << iniParseErr.line()
		<< "\n\thas error :" << iniParseErr.message()
		<< std::endl;
      throw std::runtime_error("Bad INI parsing");
    }

  for( auto vt : pt )
    {
      if (vt.first.find("TagAndProbe") != std::string::npos)
	m_tnpConfig = TagAndProbeConfig(vt);
      else
	m_sampleConfig = SampleConfig(vt);
    }

}


void DTTnPBaseAnalysis::Loop()
{

  TFile outputFile(m_sampleConfig.outputFileName,"recreate");
  outputFile.cd();

  book();

  if (fChain == 0) return;

  Long64_t nentries = (m_sampleConfig.nEvents > 0 && 
		       fChain->GetEntriesFast() > m_sampleConfig.nEvents) ? 
                       m_sampleConfig.nEvents : fChain->GetEntriesFast();

  Long64_t nbytes = 0, nb = 0;
  for (Long64_t jentry=0; jentry<nentries;jentry++) 
    {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;

      if(jentry % 10000 == 0) 
	std::cout << "[DTTnPBaseAnalysis::Loop] processed : " 
		  << jentry << " entries\r" << std::flush;

      bool hasGoodRun = false;

      for (const auto & run : m_sampleConfig.runs)
	{	  
	  if (run == 0 ||
	      run == runnumber)
	    {
	      hasGoodRun = true;
	      break;
	    }
	    
	}

      if(!hasGoodRun)
	continue;

      auto tnpPairs = tnpSelection();

      for(const auto & pair : tnpPairs) 
	{ 

	  fill(pair.second);

	}

    }

  std::cout << std::endl; 
  outputFile.Write();
  outputFile.Close();

}

void DTTnPBaseAnalysis::book()
{

  m_plots["pairMass"] = new TH1F("pairMass",
				 "tag and probe pair mass;mass [GeV];#entries/GeV",
				 100,50.,150.); 
  m_plots["pairDz"] = new TH1F("pairDz",
			       "tag and probe pair dZ;dZ(tag,probe);#entries/0.2",
			       100,-5.,5.); 
  m_plots["probePtVsPairDr"] = new TH2F("probePtVsPairDr",
					"probe p_{T} vs tag and probe dR;probe p_{T} [GeV];tag and probe dR",
					100,0.,1000., 100,0.,2*TMath::Pi());
  m_plots["probeNPixelHits"]  = new TH1F("probeNPixelHits",
				 "probe # pixel hits;# pixel hits;#entries",
				 10,-0.5,9.5); 
  m_plots["probeNTrkLayers"]  = new TH1F("probeNTrkLayers",
				 "probe # tracker layers;# tracker layers;#entries",
				 30,-0.5,29.5); 
  m_plots["probeNRPCLayers"]  = new TH1F("probeNRPCLayers",
				 "probe # RPC layers;# tracker layers;#entries",
				 30,-0.5,29.5); 
  m_plots["probeReliso"]  = new TH1F("probeReliso",
				 "probe relative trk iso;isolation;#entries",
				 100,0.,5.); 
  m_plots["probeOrigAlgo"]  = new TH1F("probeOrigAlgo",
				 "probe original algo;original algo;#entries",
				 20,-0.5,19.5); 
}

vector<std::pair<Int_t,Int_t>> DTTnPBaseAnalysis::tnpSelection()
{
  
  vector<std::pair<Int_t,Int_t>> pairs;
  
  for(Int_t iTag = 0; iTag < Nmuons; ++iTag) 
    {
      
      TLorentzVector tagVec;
      tagVec.SetXYZM(Mu_px->at(iTag),
		     Mu_py->at(iTag),
		     Mu_pz->at(iTag),
		     0.106);
      
      bool tagQuality = 
	Mu_isMuGlobal->at(iTag)     == 1 &&
	Mu_isMuTrackerArb->at(iTag) == 1 &&
	Mu_normchi2_glb->at(iTag)      < 10 &&
	Mu_numberOfMatchedStations->at(iTag) >= 2 &&
	Mu_numberOfHits_sta->at(iTag)        >  0 && 
	Mu_numberOfPixelHits_trk->at(iTag)   >= 1 &&
	Mu_numberOfTrackerLayers_trk->at(iTag) >= 6 &&
	Mu_tkIsoR03_glb->at(iTag) / tagVec.Pt() < m_tnpConfig.tag_isoCut &&
	tagVec.Pt() > m_tnpConfig.tag_minPt ;
      
      if(tagQuality && hasTrigger(iTag)) 
	{
	  
	  for(Int_t iProbe = 0; iProbe < Nmuons; ++iProbe) 
	    {
	      
	      if (iTag == iProbe) 
		continue;
	      
	      TLorentzVector probeVec;
	      probeVec.SetXYZM(Mu_px->at(iProbe),
			       Mu_py->at(iProbe),
			       Mu_pz->at(iProbe),
			       0.106);
	      
	      bool probeQuality =
		( Mu_isMuTrackerArb->at(iProbe) == 1 ||
		  Mu_isMuRPC->at(iProbe) == 1 ) &&
		Mu_origAlgo_trk->at(iProbe) != 14 && // the track is not created out of a STA mu based seeding
		Mu_numberOfPixelHits_trk->at(iProbe)     >= m_tnpConfig.probe_minPixelHits &&
		Mu_numberOfTrackerLayers_trk->at(iProbe) >= m_tnpConfig.probe_minTrkLayers &&
		Mu_tkIsoR03_trk->at(iProbe) / probeVec.Pt() < m_tnpConfig.probe_isoCut &&
		probeVec.Pt() > m_tnpConfig.probe_minPt;
	      
	      m_plots["probeNPixelHits"]->Fill(Mu_numberOfPixelHits_glb->at(iProbe));
 	      m_plots["probeNTrkLayers"]->Fill(Mu_numberOfTrackerHits_glb->at(iProbe));
 	      m_plots["probeNRPCLayers"]->Fill(Mu_isMuRPC->at(iProbe) ?
					       Mu_numberOfRPCLayers_rpc->at(iProbe) : 0);
 	      m_plots["probeReliso"]->Fill(Mu_tkIsoR03_glb->at(iProbe) / probeVec.Pt());
	      m_plots["probeOrigAlgo"]->Fill(Mu_origAlgo_trk->at(iProbe));

	      if (probeQuality)
		{
		  Float_t mass  = (tagVec + probeVec).M();
		  Float_t tnpDr = tagVec.DeltaR(probeVec);
		  Float_t tnpDz = Mu_dz_trk->at(iTag) - Mu_dz_trk->at(iProbe);

		  m_plots["pairMass"]->Fill(mass);
		  m_plots["probePtVsPairDr"]->Fill(probeVec.Pt(),tnpDr);
		  m_plots["pairDz"]->Fill(tnpDz);
		  
		  if (std::abs(tnpDz) < m_tnpConfig.pair_maxAbsDz && 
		      Mu_charge->at(iTag) * Mu_charge->at(iProbe) == -1 &&
		      mass  > m_tnpConfig.pair_minInvMass && 
		      mass  < m_tnpConfig.pair_maxInvMass &&
		      tnpDr > m_tnpConfig.pair_minDr)
		    {
		      pairs.push_back(std::make_pair(iTag,iProbe));
		      
		      break; // just one probe per tag
		    }
		} 
	    }
	}
    }
  
  return pairs;
  
}

bool DTTnPBaseAnalysis::hasTrigger(const Int_t iTag) 
{

  if(m_hltFilterId < 0)
    return false;
  
  return getXY<Float_t>(Mu_hlt_Dr,iTag,m_hltFilterId) < m_tnpConfig.tag_hltDrCut;
  
}

Int_t DTTnPBaseAnalysis::nMatchedCh(const Int_t iMu,
				    const Int_t iCh) 
{

  Int_t nMatchedCh = 0;
  UInt_t chMask = Mu_stationMask->at(iMu);

  for(int index = 0; index < 8; ++index)
    if ((chMask & 1<<index) && index != iCh-1)
      ++nMatchedCh;

  return nMatchedCh;
  
}

