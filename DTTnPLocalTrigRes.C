#include "DTTnPLocalTrigRes.h"


DTTnPLocalTrigRes::DTTnPLocalTrigRes(const std::string & configFile) : DTTnPBaseAnalysis(configFile)
{

}

void DTTnPLocalTrigRes::Loop()
{

  TFile outputFile(m_sampleConfig.outputFileName,"recreate");
  outputFile.cd();

  book();

  if (fChain == 0) return;

  Long64_t nentries = (m_sampleConfig.nEvents > 0 && 
		       fChain->GetEntriesFast() > m_sampleConfig.nEvents) ? 
    m_sampleConfig.nEvents : fChain->GetEntriesFast();

  std::cout << "[DTTnPLocalTrigRes::Loop] going to process "
	    << nentries << " entries\n" << std::flush;

  Long64_t nbytes = 0, nb = 0;
  for (Long64_t jentry=0; jentry<nentries;jentry++) 
    {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;

      if(jentry % 10000 == 0) 
	std::cout << "[DTTnPLocalTrigRes::Loop] processed : " 
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

 
  
  endJob();

  outputFile.Write();
  outputFile.Close();

}

void DTTnPLocalTrigRes::book()
{

  DTTnPBaseAnalysis::book();

  
  m_plots["nOtherMatchedChVsEta"] = new TH2F("nOtherMatchedChVsEta",
					     "# of matched stations other than the one under investigation",
					     80,-1.2,1.2,5,-0.5,4.5);
  
/*  
  for (Int_t iCh = 1; iCh < 5; ++iCh) {
      std::stringstream iChTag;
      iChTag << "MB" << iCh;
      
      std::string hName = "probePt" + iChTag.str();
      m_plots[hName]  = new TH1F(hName.c_str(),
				 "probe p_{T};p_{T} [GeV];#entries/GeV",
				 200,0.,200.); 
      hName = "probeEta" + iChTag.str();
      m_plots[hName] = new TH1F(hName.c_str(),
				"probe #eta;#eta;#entries/0.05",
				56,-1.2,1.2); 
      hName = "probePhi" + iChTag.str();
      m_plots[hName] = new TH1F(hName.c_str(),
				"probe #phi;#phi;#entries/(pi*90)",
				180,-TMath::Pi(),TMath::Pi()); 
      
  }
*/
  
  Int_t nPhiBins  = 401;
  Float_t rangePhi  = 10.025;
  Int_t nPhibBins = 401;
  Float_t rangePhiB = 10.025;
  
  /// ---- Resolution plots per sector, wheel & chamber: 
  for (Int_t iCh = 1; iCh <= 4; ++iCh) {
    std::stringstream station;  station << "_MB" << iCh;
    std::cout << "Booking station: "<< station.str() << std::endl;
    
    std::string chTag = "_MB" + station.str();	  
    std::string hName = "TM_PhiResidualIn_SecVsWh" + chTag;
//NOTFORNOW    m_plots[hName.c_str()] = new TH2F(hName.c_str(), 
//NOTFORNOW				      "Trigger local position In - Segment local position (correlated triggers); sector; wheel", 
//NOTFORNOW      				      12,0.5,12.5,5,-2.5,2.5);
//NOTFORNOW    
//NOTFORNOW    hName = "TM_PhibResidualIn_SecVsWh" + chTag.c_str();
//NOTFORNOW    m_plots[hName.c_str()] = new TH1F(hName.c_str(), ,
//NOTFORNOW				      "Trigger local direction In - Segment local direction (correlated triggers); sector; wheel",
//NOTFORNOW      				      12,0.5,12.5,5,-2.5,2.5);
//NOTFORNOW    
//NOTFORNOW    hName = "TM_PhiResidualOut_SecVsWh" + chTag.c_str();
//NOTFORNOW    m_plots[hName.c_str()] = new TH1F(hName.c_str(),
//NOTFORNOW				      "Trigger local position Out - Segment local position (correlated triggers); sector; wheel",
//NOTFORNOW      				      12,0.5,12.5,5,-2.5,2.5);
//NOTFORNOW    
//NOTFORNOW    hName = "TM_PhibResidualOut_SecVsWh" + chTag.c_str();
//NOTFORNOW    m_plots[hName.c_str()] = new TH1F(hName.c_str(),
//NOTFORNOW				      "Trigger local direction Out - Segment local direction (correlated triggers); sector; wheel",
//NOTFORNOW      				      12,0.5,12.5,5,-2.5,2.5);					

    
    for (Int_t iWh = 1; iWh <= 5; ++iWh)    {
      std::stringstream wheel; 
      if (iWh<3 ) wheel << "_Whm" << iWh;	
      if (iWh==3) wheel << "_Wh0";
      if (iWh>3 ) wheel << "_Whp" << iWh-3;
      std::cout << "Booking wheel: " << wheel.str() << std::endl;
            	
      for (Int_t iSc = 1; iSc <= 14; ++iSc)    {
	std::stringstream sector; sector << "_Sec" << iSc;
	std::cout << "Booking sector: " << sector.str() << std::endl;
	
	if (iSc > 12 && iCh !=4) continue;
	
	std::string chTag = wheel.str() + sector.str() + station.str();	  
	hName = "TM_PhiResidualIn" + chTag;
	m_plots[hName.c_str()] = new TH1F(hName.c_str(), 
					  "Trigger local position In - Segment local position (correlated triggers); #phi", 
					  nPhiBins,-rangePhi,rangePhi);
	
	hName = "TM_PhibResidualIn" + chTag;
	m_plots[hName.c_str()] = new TH1F(hName.c_str(),
					  "Trigger local direction In - Segment local direction (correlated triggers); #phi",
					  nPhiBins,-rangePhi,rangePhi);
	
	hName = "TM_PhiResidualOut" + chTag;
	m_plots[hName.c_str()] = new TH1F(hName.c_str(),
					  "Trigger local position Out - Segment local position (correlated triggers); #phi",
					  nPhiBins,-rangePhi,rangePhi);
	
	hName = "TM_PhibResidualOut" + chTag;
	m_plots[hName.c_str()] = new TH1F(hName.c_str(),
					  "Trigger local direction Out - Segment local direction (correlated triggers); #phi",
					  nPhiBins,-rangePhi,rangePhi);
	
	hName = "TM_PhitkvsPhitrig" + chTag;
	m_plots[hName.c_str()] = new TH2F(hName.c_str(), 
					  "Local position: segment vs trigger",
					  100,-500.,500.,100,-500.,500.);
	
	hName = "TM_PhibtkvsPhibtrig" + chTag;
	m_plots[hName.c_str()] = new TH2F(hName.c_str(), 
					  "Local direction : segment vs trigger",
					  200,-40.,40.,200,-40.,40.);

	hName = "TM_PhibResidualvsTkPos" + chTag;
	m_plots[hName.c_str()] = new TH2F(hName.c_str(), 
					  "Local direction residual vs Segment Position",
					  100,-500.,500.,200,-10.,10.);
	
	hName = "TM_PhiResidualvsTkPos" + chTag;
	m_plots[hName.c_str()] = new TH2F(hName.c_str(), 
					  "Local Position residual vs Segment Position",
					  100,-500.,500.,200,-10.,10.);
      }
    }
  }
  
}

void DTTnPLocalTrigRes::fill(const Int_t iMu)
{

 

  TLorentzVector probeVec;
  probeVec.SetXYZM(Mu_px->at(iMu),
		   Mu_py->at(iMu),
		   Mu_pz->at(iMu),
		   0.106);


  /// ---- Resolution plots per sector, wheel & chamber: 
  for (Int_t iCh = 1; iCh <= 4; ++iCh) {
    std::stringstream station;  
    station << "_MB" << iCh;
    
    Int_t nMatchInOtherCh = DTTnPBaseAnalysis::nMatchedCh(iMu,iCh);
    m_plots["nOtherMatchedChVsEta"]->Fill(Mu_eta->at(iMu),nMatchInOtherCh);
    if ( nMatchInOtherCh < m_tnpConfig.probe_minNMatchedSeg )  continue;

    Int_t iPassingTMuxIn  = getPassingTrigIn(iMu,iCh);  // TMuxIn primitive id in that station
    Int_t iPassingTMuxOut = getPassingTrigOut(iMu,iCh);  // TMuxOut primitive id in that station
    Int_t iPassingSeg     = getPassingProbe(iMu,iCh); // track segment id in t
    
    // ------  compute TMuxIn efficiency with WITH segment requirements 
    bool segm_ok = false;
    segm_ok = (iPassingSeg>=0 && ((dtsegm4D_hasZed->at(iPassingSeg)>0 && iCh!=4 && dtsegm4D_phinhits->at(iPassingSeg)>=4) ||
				  (iCh==4 && dtsegm4D_phinhits->at(iPassingSeg)>=4)));
   
    if (!segm_ok) continue;
    if (std::abs(probeVec.Eta()) > m_tnpConfig.probe_maxAbsEta[iCh - 1]) continue;    

    for (Int_t iWh = 1; iWh <= 5; ++iWh)    {
      std::stringstream wheel; 
      if (iWh<3 ) wheel << "_Whm" << iWh;	
      if (iWh==3) wheel << "_Wh0";
      if (iWh>3 ) wheel << "_Whp" << iWh-3;
      
      for (Int_t iSc = 1; iSc <= 14; ++iSc)    {
	if (iSc > 12 && iCh !=4) continue;
	std::stringstream sector; sector << "_Sec" << iSc;
	std::string chTag = wheel.str() + sector.str() + station.str();	  	

	int wheel    = dtsegm4D_wheel->at(iPassingSeg);
	int station  = dtsegm4D_station->at(iPassingSeg);
	int scsector = dtsegm4D_sector->at(iPassingSeg);

	if (iCh  != station ) continue;
	if (iWh-3!= wheel   ) continue; 
	if (iSc  != scsector) continue;
	float trackPosPhi = TMath::Tan(dtsegm4D_phi->at(iPassingSeg));
	float trackDirPhi = TMath::ATan(dtsegm4D_x_dir_loc->at(iPassingSeg)/ dtsegm4D_y_dir_loc->at(iPassingSeg))*TMath::RadToDeg(); 
//SEGMENT	float trackPosPhi = dtsegm4D_x_pos_loc->at(iPassingSeg);
//SEGMENT	float trackDirPhi = TMath::ATan(dtsegm4D_x_dir_loc->at(iPassingSeg)/ dtsegm4D_y_dir_loc->at(iPassingSeg))*TMath::RadToDeg(); 
//SEGMENT
//SEGMENT	Float_t trigPos = 0.; 
//SEGMENT	Float_t trigDir = 0.;
//SEGMENT	Float_t deltaPos = 0.; 
//SEGMENT	Float_t deltaDir = 0.; 
//SEGMENT	float phin = (ssector-1)*TMath::Pi()/6;
//SEGMENT	float phicenter =  dtsegm4D_phi;
//SEGMENT	float deltaphi = phicenter-phin;
//SEGMENT	float r = TMath::Sqrt(dtsegm4D_cosx->at(iPassingSeg)*dtsegm4D_cosx->at(iPassingSeg)+dtsegm4D_cosy->at(iPassingSeg)*dtsegm4D_cosy->at(iPassingSeg));
    
    Float_t trigPos = 0.; 
    Float_t trigDir = 0.;
    Float_t deltaPos = 0.;
    Float_t deltaDir = 0.;
    
    std::string hName = ""; 
	if (iPassingTMuxIn >=0) {
	  // In: 
	  trigPos = ltTwinMuxIn_phi->at(iPassingTMuxIn);
	  trigDir = ltTwinMuxIn_phiB->at(iPassingTMuxIn);
	  
	  
	  //	  float x = (tan(phi/4096.)-tan(deltaphi))*(r*cos(deltaphi) - zcn_[st-1]); //zcn is in local coordinates -> z invreases approching to vertex
	  
	  trigPos  = TMath::Tan(trigPos/4096. + ((TMath::Pi()*30.)/180.)*(scsector-1));
	  trigDir = (trigDir/512.+trigPos/4096.)*TMath::RadToDeg();
	  
	  deltaPos = trigPos-trackPosPhi;
	  deltaDir = trigDir-trackDirPhi;
	  
	  hName = "TM_PhiResidualIn" + chTag;
	  m_plots[hName.c_str()]->Fill(deltaPos);
	  
	  hName = "TM_PhibResidualIn" + chTag;
	  m_plots[hName.c_str()]->Fill(deltaDir);
	  
	  hName = "TM_PhitkvsPhitrig" + chTag;
	  m_plots[hName.c_str()]->Fill(trigPos,trackPosPhi);
	  
	  hName = "TM_PhibtkvsPhibtrig" + chTag;
	  m_plots[hName.c_str()]->Fill(trigDir,trackDirPhi);
	  
	  hName = "TM_PhibResidualvsTkPos" + chTag;
	  m_plots[hName.c_str()]->Fill(trackPosPhi,trigDir-trackDirPhi);
	  
	  hName = "TM_PhiResidualvsTkPos" + chTag;
	  m_plots[hName.c_str()]->Fill(trackPosPhi,trigPos-trackPosPhi);
	}
	// Out: 
	if (iPassingTMuxOut >= 0) {
	  trigPos  = ltTwinMuxOut_phi->at(iPassingTMuxOut);
	  trigDir = ltTwinMuxOut_phiB->at(iPassingTMuxOut);
//	  trigPos  = trigPos/4096 + ((TMath::Pi()*30)/180)*(scsector-1);
//	  trigDir = (trigDir/512.+trigPos/4096.)*TMath::RadToDeg();

	  trigPos  = TMath::Tan(trigPos/4096. + ((TMath::Pi()*30.)/180.)*(scsector-1));
	  trigDir = (trigDir/512.+trigPos/4096.)*TMath::RadToDeg();
	  
	  deltaPos = trigPos-trackPosPhi;
	  deltaDir = trigDir-trackDirPhi;
	  
	  hName = "TM_PhiResidualOut" + chTag;
	  m_plots[hName.c_str()]->Fill(deltaPos);
	  
	  hName = "TM_PhibResidualOut" + chTag;
	  m_plots[hName.c_str()]->Fill(deltaDir);
	}
      }
    }
  }
}

void DTTnPLocalTrigRes::endJob() 
{
}

Int_t DTTnPLocalTrigRes::getPassingProbe(const Int_t iMu,
					   const Int_t iCh) 
  {

    Int_t iBestSeg   = -1;
    Float_t bestSegDr = 999.;	  
	  
    for (Int_t iMatch = 0; iMatch < Mu_nMatches->at(iMu); ++ iMatch)
      {
	Int_t whMu  = getXY<Int_t>(Mu_matches_Wh,iMu,iMatch);
	Int_t secMu = getXY<Int_t>(Mu_matches_Sec,iMu,iMatch);
	Int_t stMu  = getXY<Int_t>(Mu_matches_St,iMu,iMatch);
	Float_t xMu = getXY<Float_t>(Mu_matches_x,iMu,iMatch); 
	Float_t yMu = getXY<Float_t>(Mu_matches_y,iMu,iMatch); 
      

	if (stMu == iCh)
	  {
	  
	    Int_t iSeg = getPassingProbeInCh(iMu,stMu,secMu,
					     whMu,xMu,yMu);

	    if (iSeg < 0)
	      continue;
	  
	    Float_t xSeg = dtsegm4D_x_pos_loc->at(iSeg);
	    Float_t ySeg = dtsegm4D_y_pos_loc->at(iSeg);
	  
	    Float_t dX = std::abs(xSeg-xMu);
	    Float_t dY = std::abs(ySeg-yMu);
	    Float_t dR = sqrt(dX*dX + dY*dY);
	  
	    if(dR < bestSegDr)
	      {
		iBestSeg = iSeg;
		bestSegDr = dR;
	      }
	  
	  }
      
      }

    return iBestSeg;

  }

Int_t DTTnPLocalTrigRes::getPassingTrigIn(const Int_t iMu,
					  const Int_t iCh) 
{
  
  Int_t ibestTMuxIn = -1;
  Float_t bestDphi = 999.;	  
  
  for (Int_t iMatch = 0; iMatch < Mu_nMatches->at(iMu); ++ iMatch)      {
    
    Int_t whMu  = getXY<Int_t>(Mu_matches_Wh,iMu,iMatch);
    Int_t secMu = getXY<Int_t>(Mu_matches_Sec,iMu,iMatch);
    Int_t stMu  = getXY<Int_t>(Mu_matches_St,iMu,iMatch);
    Float_t phiMu = getXY<Float_t>(Mu_matches_phi,iMu,iMatch); 
       
    if (stMu == iCh)
      {
//	if(stMu==4 && secMu==13) {secMu=4;}
//	else if(stMu==4 && secMu==14 ) {secMu=10;}
	
	Int_t iTMuxIn = getPassingTrigInCh(iMu,stMu,secMu,whMu,phiMu);
	if (iTMuxIn < 0)
	  continue;
	

	Float_t phiTrig = ltTwinMuxIn_phi->at(iTMuxIn);
	phiTrig = phiTrig/4096 + ((3.1415927*30)/180)*(secMu-1);
	
	Float_t Dphi = fabs(phiMu-phiTrig);
	if(Dphi < bestDphi)
	  {
	    ibestTMuxIn = iTMuxIn;
	    bestDphi = Dphi;
	  }
	
      }
    
  }
  
  return ibestTMuxIn;

}

Int_t DTTnPLocalTrigRes::getPassingTrigOut(const Int_t iMu,
					   const Int_t iCh) 
  {
    
    Int_t ibestTMuxOut = -1;
    Float_t bestDphi = 999.;	  
    
    for (Int_t iMatch = 0; iMatch < Mu_nMatches->at(iMu); ++ iMatch)
      {

	Int_t whMu  = getXY<Int_t>(Mu_matches_Wh,iMu,iMatch);
	Int_t secMu = getXY<Int_t>(Mu_matches_Sec,iMu,iMatch);
	Int_t stMu  = getXY<Int_t>(Mu_matches_St,iMu,iMatch);
	Float_t phiMu = getXY<Float_t>(Mu_matches_phi,iMu,iMatch); 
       
      	if (stMu == iCh)
	  {
	    
	  
	    Int_t iTMuxOut = getPassingTrigOutCh(iMu,stMu,secMu,
						 whMu,phiMu);

	    if (iTMuxOut < 0)
	      continue;
	  
	    Float_t phiTrig = ltTwinMuxOut_phi->at(iTMuxOut);
	    phiTrig = phiTrig/4096 + ((3.1415927*30)/180)*(secMu-1);
	  
	    Float_t Dphi = fabs(phiMu-phiTrig);
	    
	    if(Dphi < bestDphi)
	      {
		ibestTMuxOut = iTMuxOut;
		bestDphi = Dphi;
	      }
	    
	  }
      
      }
    return ibestTMuxOut;
  }


Int_t DTTnPLocalTrigRes::getPassingProbeInCh(const Int_t iMu,
					     const Int_t stMu,
					     const Int_t secMu,
					     const Int_t whMu,
					     const Int_t xMu,
					     const Int_t yMu)
{
  
    Int_t iBestSeg   = -1;
    Float_t bestSegDr = 999.;
  
    for (Int_t iSeg = 0; iSeg < Ndtsegments; ++ iSeg)
      {
      
	Int_t whSeg  = dtsegm4D_wheel->at(iSeg);
	Int_t secSeg = dtsegm4D_sector->at(iSeg);
	Int_t stSeg  = dtsegm4D_station->at(iSeg);
	Float_t xSeg = dtsegm4D_x_pos_loc->at(iSeg);
	Float_t ySeg = dtsegm4D_y_pos_loc->at(iSeg);
      
	Float_t dX = std::abs(xSeg-xMu);
	Float_t dY = std::abs(ySeg-yMu);
	Float_t dR = sqrt(dX*dX + dY*dY);

	// if(stSeg ==4 && secSeg==13)  {secSeg=4;}
	// else if(stSeg ==4 && secSeg==14)  {secSeg=10;}
      
	if(whMu  == whSeg  &&
	   secMu == secSeg &&
	   stMu  == stSeg  &&
	   dR < bestSegDr  &&
	   dR < m_tnpConfig.passing_probe_maxTkSegDr &&
	   dX < m_tnpConfig.passing_probe_maxTkSegDx &&
	   dY < m_tnpConfig.passing_probe_maxTkSegDy )
	  {
	    iBestSeg = iSeg;
	    bestSegDr = dR;
	  }
      }

    return iBestSeg;

  }


  Int_t DTTnPLocalTrigRes::getPassingTrigInCh(const Int_t iMu,
					      const Int_t stMu,
					      const Int_t secMu,
					      const Int_t whMu,
					      const Float_t phiMu
					      )
  {
  
    Int_t ibestTMuxIn   = -1;
    Float_t bestTMuxInQual = -1.;
    //       std::cout << " ******* another call to local trig eff " << std::endl;
    for (Int_t iTMuxIn = 0; iTMuxIn < NdtltTwinMuxIn; ++ iTMuxIn)
      {
      
	Int_t whTMuxIn  = ltTwinMuxIn_wheel->at(iTMuxIn);
	Int_t secTMuxIn = ltTwinMuxIn_sector->at(iTMuxIn);
	Int_t stTMuxIn  = ltTwinMuxIn_station->at(iTMuxIn);
	Int_t bxTMuxIn  = ltTwinMuxIn_bx->at(iTMuxIn);
	Int_t phiTMuxIn  = ltTwinMuxIn_phi->at(iTMuxIn);
	//	Int_t phiBTMuxIn  = ltTwinMuxIn_phiB->at(iTMuxIn);
	Int_t qualityTMuxIn  = ltTwinMuxIn_quality->at(iTMuxIn);

	if(stTMuxIn ==4 && secTMuxIn==13)  {secTMuxIn=4;}
	else if(stTMuxIn ==4 && secTMuxIn==14)  {secTMuxIn=10;}
      
	if(whMu  == whTMuxIn   &&
	   secMu == secTMuxIn  &&
	   stMu  == stTMuxIn   && 
	   bxTMuxIn == 0       &&
	   qualityTMuxIn > bestTMuxInQual
	   )
	
	  {
	    ibestTMuxIn = iTMuxIn;
	    bestTMuxInQual = qualityTMuxIn;	  
	  }
      }
  
    
    return ibestTMuxIn;
    
    
  }


  Int_t DTTnPLocalTrigRes::getPassingTrigOutCh(const Int_t iMu,
					      const Int_t stMu,
					      const Int_t secMu,
					      const Int_t whMu,
					      const Float_t phiMu
					      )
  {
  
    Int_t ibestTMuxOut   = -1;
    Float_t bestTMuxOutQual = -1.;
    //       std::cout << " ******* another call to local trig eff " << std::endl;
    for (Int_t iTMuxOut = 0; iTMuxOut < NdtltTwinMuxOut; ++ iTMuxOut)
      {
      
	Int_t whTMuxOut  = ltTwinMuxOut_wheel->at(iTMuxOut);
	Int_t secTMuxOut = ltTwinMuxOut_sector->at(iTMuxOut);
	Int_t stTMuxOut  = ltTwinMuxOut_station->at(iTMuxOut);
	Int_t bxTMuxOut  = ltTwinMuxOut_bx->at(iTMuxOut);
	Int_t phiTMuxOut  = ltTwinMuxOut_phi->at(iTMuxOut);
	//	Int_t phiBTMuxOut  = ltTwinMuxOut_phiB->at(iTMuxOut);
	Int_t qualityTMuxOut  = ltTwinMuxOut_quality->at(iTMuxOut);

	if(stTMuxOut ==4 && secTMuxOut==13)  {secTMuxOut=4;}
	else if(stTMuxOut ==4 && secTMuxOut==14)  {secTMuxOut=10;}
      
	if(whMu  == whTMuxOut   &&
	   secMu == secTMuxOut  &&
	   stMu  == stTMuxOut   && 
	   bxTMuxOut == 0       &&
	   qualityTMuxOut > bestTMuxOutQual
	   )
	
	  {
	    ibestTMuxOut = iTMuxOut;
	    bestTMuxOutQual = qualityTMuxOut;	  
	  }
      }
  
    
    return ibestTMuxOut;
    
    
  }

