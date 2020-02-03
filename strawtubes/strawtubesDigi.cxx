#include "strawtubesDigi.h"
#include "FairRunSim.h"

//last updated

strawtubesDigi::strawtubesDigi() {
    mpvTime = 0;
    LandauSigma = 0;
    newDist2Wire = 0;
    f2 = 0;
    minimalDriftTime = 5.285;
    timeDependence = new TF1("timeCoordinate_dependence", "[0]*x*x + [1]");
    timeDependence->SetParameter(0, 622.8);
    timeDependence->SetParameter(1, minimalDriftTime);
    leftChain = new TF1("leftChain", "[0]*x*x + [1]");
    leftChain->SetParameter(1, minimalDriftTime);
    rightChain = new TF1("rightChain", "[0]*x*x + [1]");
    rightChain->SetParameter(1, minimalDriftTime);
    rand = new TRandom3();
}

strawtubesDigi::~strawtubesDigi() {
   delete timeDependence;
   delete leftChain;
   delete rightChain;
}

void strawtubesDigi::driftTimeCalculation(Double_t dist2Wire, bool inSmallerArea) {
    if (inSmallerArea) {
       mpvTime = rightChain->Eval(dist2Wire);
    } else {
       mpvTime = leftChain->Eval(dist2Wire);
    }
    f2 = p1 * TMath::Exp(-p2 * dist2Wire) + p3 * TMath::Exp(-p4 * dist2Wire) + p5;
    LandauSigma = mpvTime * f2 / 100;
    driftTime = rand->Landau(mpvTime, LandauSigma);
}

void strawtubesDigi::NewDist2WireCalculation(Double_t driftTime, Double_t wireOffset) {
    parabolaChainsEstimation(wireOffset);
    Double_t checkTime = rightChain->Eval(1.0 - wireOffset);
    if (driftTime < checkTime) {
       newDist2Wire = rightChain->GetX(driftTime, 0., 4.);
    } else {
       newDist2Wire = leftChain->GetX(driftTime, 0., 4.);
    }
}

void strawtubesDigi::default_NewDist2WireCalculation(Double_t driftTime)
{
   newDist2Wire = sqrt(abs(driftTime - 5.285) / 622.8);
}

void strawtubesDigi::SetLandauParams(Double_t p1, Double_t p2, Double_t p3, Double_t p4, Double_t p5) {
    this->p1 = p1;
    this->p2 = p2;
    this->p3 = p3;
    this->p4 = p4;
    this->p5 = p5;
}

Double_t strawtubesDigi::DriftTimeFromDist2Wire(Double_t dist2Wire, Double_t wireOffset, bool inSmallerArea)
{
   parabolaChainsEstimation(wireOffset);
   driftTimeCalculation(dist2Wire, inSmallerArea);
   return driftTime;
}

Double_t strawtubesDigi::NewDist2WireFromDriftTime(Double_t driftTime)
{
//   NewDist2WireCalculation(driftTime);
   default_NewDist2WireCalculation(driftTime);
   return newDist2Wire;
}

Double_t strawtubesDigi::DriftTimeFromTDC(Double_t TDC, Double_t t0, Double_t signalPropagationTime) {
  return TDC - t0 - signalPropagationTime;

}

void strawtubesDigi::parabolaChainsEstimation(Double_t wireOffset)
{
   Double_t aLeftChain = 83.11 * wireOffset + 622.8;
   Double_t aRightChain = -83.11 * wireOffset + 622.8;
   leftChain->SetParameter(0, aLeftChain);
   rightChain->SetParameter(0, aRightChain);
}

void strawtubesDigi::d2w_dtRelation(const TH1D* TDC, TGraph* graph)
{
   TH1D* TDChist = (TH1D*) TDC->Clone();
   Double_t tubeRadius = 1.0;
   Double_t wireRadius = 0.01;
   Double_t sum = 0;
   Double_t coordinate = 0;
   TF1 *tdcFunc = new TF1("tdcFunc", "[0] + ( [1] * (1 + [2] * exp( ([4] - x) / [3]) ) ) / ( (1 + exp( ([4] - x) / [6]) ) * (1 + exp( (x - [5]) / [7]) ) )", 0, 1500);
   tdcFunc->SetParameter(0,2);
   tdcFunc->SetParLimits(0,0,5);
   tdcFunc->SetParameter(1,7500);
   tdcFunc->SetParLimits(1,0,30000);
   tdcFunc->SetParameter(2,8);
   tdcFunc->SetParLimits(2,0,100);
   tdcFunc->SetParameter(3,175);
   tdcFunc->SetParLimits(3,0,1100);
   tdcFunc->SetParameter(4,0);
   tdcFunc->SetParLimits(4,0,100);
   tdcFunc->SetParameter(5,600);
   tdcFunc->SetParLimits(5,500, 900);
   tdcFunc->SetParameter(6,0);
   tdcFunc->SetParLimits(6,0,2);
   tdcFunc->SetParameter(7,15);
   tdcFunc->SetParLimits(7,0,100);
   TDChist->Fit("tdcFunc");
   Double_t maxT = tdcFunc->GetParameter(5);
   TAxis *xaxis = TDChist->GetXaxis();
   Int_t maxBinT = xaxis->FindBin(maxT);
   for (int i = 0; i < maxBinT; ++i) {
      for (int j = 0; j < i; ++j) {
         sum += TDChist->GetBinContent(j);
      }
      coordinate = (sum / TDChist->Integral()) * (tubeRadius - wireRadius) + wireRadius;
      graph->SetPoint(i, coordinate, TDChist->GetBinCenter(i));
      sum = 0;
   }
   delete tdcFunc;
}

// For the Misalignment part
void strawtubesDigi::SetSameSagging(Double_t inMaxTubeSagging, Double_t inMaxWireSagging)
{
    maxTubeSagging = inMaxTubeSagging;
    maxWireSagging = inMaxWireSagging;
    randType = None;
    misalign = true;
    beingInit = true;
}

void strawtubesDigi::SetGausSagging(Double_t inMaxTubeSagging, Double_t inTubeGausSigma, Double_t inMaxWireSagging, Double_t inWireGausSigma)
{
    maxTubeSagging = inMaxTubeSagging;
    maxWireSagging = inMaxWireSagging;
    tubeGausSigma = inTubeGausSigma;
    wireGausSigma = inWireGausSigma;
    randType = Gaus;
    misalign = true;
    beingInit = true;
}

void strawtubesDigi::SetUnifSagging(Double_t inMaxTubeSagging, Double_t inTubeUnifDelta, Double_t inMaxWireSagging, Double_t inWireUnifDelta)
{
    maxTubeSagging = inMaxTubeSagging;
    maxWireSagging = inMaxWireSagging;
    tubeUnifDelta = inTubeUnifDelta;
    wireUnifDelta = inWireUnifDelta;
    randType = Unif;
    misalign = true;
    beingInit = true;
}

void strawtubesDigi::InitializeMisalign(Double_t tubeSag, Double_t wireSag, Double_t r, bool inDebug)
{
    if (not(beingInit))
    {
       maxTubeSagging = tubeSag;
       maxWireSagging = wireSag;
       tubeRadius = r;
       debug = inDebug;
       randType = None;
       misalign = true;
       beingInit = true;
    }
}

void strawtubesDigi::InitializeMisalign(Double_t tubeMean, Double_t tubeSigma, Double_t wireSigma, Double_t wireMean, Double_t r, bool inDebug)
{
    if (not(beingInit))
    {
       maxTubeSagging = tubeMean;
       tubeGausSigma = tubeSigma;
       maxWireSagging = wireMean;
       wireGausSigma = wireSigma;
       tubeRadius = r;
       debug = inDebug;
       randType = Gaus;
       misalign = true;
       beingInit = true;
    }
}

Double_t strawtubesDigi::GetMaxTubeSagging(Float_t ID)
{
    if (randType == None)
    {
       return maxTubeSagging;
    }
    else
    {
       if (tubeSaggingMap.count(ID) == 0)
       {
          if (randType == Gaus) //not yet finished, need a condition check to prevent non physics result
          {
              tubeSaggingMap[ID] = rand->Gaus(maxTubeSagging, tubeGausSigma);
          }
          else if (randType == Unif)
          {
              tubeSaggingMap[ID] = rand->Uniform(maxTubeSagging - tubeUnifDelta, maxTubeSagging + tubeUnifDelta);
          }
          if (tubeSaggingMap[ID] < 0){ tubeSaggingMap[ID] = 0;}
       }
       return tubeSaggingMap[ID];
    }
}

Double_t strawtubesDigi::GetMaxWireSagging(Float_t ID)
{
    if (randType == None)
    {
       return maxWireSagging;
    }
    else
    {
       if (wireSaggingMap.count(ID) == 0)
       {
          if (randType == Gaus) // not yet finished
          {
              wireSaggingMap[ID] = rand->Gaus(maxWireSagging, wireGausSigma);
          }
          else if (randType == Unif)
          {
              wireSaggingMap[ID] = rand->Uniform(maxWireSagging - wireUnifDelta, maxWireSagging + wireUnifDelta);
          }
          if (wireSaggingMap[ID] < 0){ wireSaggingMap[ID] = 0;}
       }
       return wireSaggingMap[ID];
    }
}

Double_t strawtubesDigi::FindTubeShift(Double_t x, Double_t startx, Double_t stopx, Float_t ID)
{
   Double_t delta = GetMaxTubeSagging(ID);
   Double_t a = -4. * delta / TMath::Sq(startx - stopx);
   Double_t b = 0.5 * (startx + stopx);
   Double_t c = delta;

   return a * TMath::Sq(x - b) + c;
}

Double_t strawtubesDigi::FindWireShift(Double_t x, Double_t startx, Double_t stopx, Float_t ID)
{
   Double_t delta = GetMaxWireSagging(ID);
   Double_t a = -4. * delta / TMath::Sq(startx - stopx);
   Double_t b = 0.5 * (startx + stopx);
   Double_t c = delta;

   return a * TMath::Sq(x - b) + c;
}

bool strawtubesDigi::CheckInTube(TVector3 pPos, TVector3 start, TVector3 stop, Float_t ID)
{
   Double_t tubeShift = FindTubeShift(pPos.x(), start.x(), stop.x(), ID);
   TVector3 wPos = ((start.x() - pPos.x()) * stop + (pPos.x() - stop.x()) * start) * (1. / (start.x() - stop.x()));

   Double_t r = tubeRadius;
   Double_t theta = TMath::ATan((start.y() - stop.y()) / (start.x() - stop.x()));
   Double_t dz = pPos.z() - wPos.z();
   Double_t h = TMath::Sqrt(r * r - dz * dz) / TMath::Cos(theta);

   if ((h - (pPos.y() - wPos.y())) < tubeShift) {
      //       if (debug){ std::cout<<"OutOfTube"<<std::endl; }
      return false;
   }
   return true;
}

Double_t strawtubesDigi::FindWireSlope(Double_t x, TVector3 start, TVector3 stop, Float_t ID)
{
    Double_t startx = start.x();
    Double_t stopx = stop.x();
    Double_t delta = GetMaxWireSagging(ID);
    Double_t a = -4. * delta / TMath::Sq(startx - stopx);
    Double_t b = 0.5 * (startx + stopx);

    Double_t slope = 2 * a * (x - b);
    slope = slope + (start.y() - stop.y())/(startx - stopx);
    return slope;
}

Double_t strawtubesDigi::FindMisalignDist2Wire(TVector3 pPos, TVector3 start, TVector3 stop, Float_t ID)
{
    // Approimate version, linearlized locally for the wire,
    TVector3 wPos = ((start.x() - pPos.x()) * stop + (pPos.x() - stop.x()) * start) * (1./(start.x() - stop.x()));
    Double_t wireShift = FindWireShift(pPos.x(), start.x(), stop.x(), ID);
    TVector3 dr = pPos - (wPos - TVector3(0,wireShift,0));
    //return dr.Mag();

    Double_t slope = FindWireSlope(pPos.x(), start, stop, ID);
    Double_t theta = TMath::ATan(slope);
    Double_t ds = TMath::Sqrt(dr.x()*dr.x() + dr.y() * dr.y()) * cos(theta);
    Double_t dz = dr.z();
    Double_t result = TMath::Sqrt(ds*ds+dz*dz);

    TVector3 pFromEnd = pPos - start;
    result = TMath::Min(result, pFromEnd.Mag());
    pFromEnd = pPos - stop;
    result = TMath::Min(result, pFromEnd.Mag());
    return result;

    // Another method, by using TF1 to find inverse function and minimize
}

Double_t strawtubesDigi::FindMisalignDist2Wire(strawtubesPoint* p)
{
    TVector3 pPos = TVector3(p->GetX(), p->GetY(), p->GetZ());
    Float_t ID = p->GetDetectorID();
    TVector3 start = TVector3();
    TVector3 stop = TVector3();
    strawtubes* module = dynamic_cast<strawtubes*> (FairRunSim::Instance()->GetListOfModules()->FindObject("Strawtubes") );
    module->StrawEndPoints(ID,start,stop);

    // Approimate version
    TVector3 wPos = ((start.x() - pPos.x()) * stop + (pPos.x() - stop.x()) * start) * (1./(start.x() - stop.x()));
    Double_t wireShift = FindWireShift(pPos.x(), start.x(), stop.x(), ID);
    TVector3 dr = pPos - (wPos - TVector3(0,wireShift,0));
    //return dr.Mag();

    Double_t slope = FindWireSlope(pPos.x(), start, stop, ID);
    Double_t theta = TMath::ATan(slope);
    Double_t ds = TMath::Sqrt(dr.x()*dr.x() + dr.y() * dr.y()) * cos(theta);
    Double_t dz = dr.z();
    Double_t result = TMath::Sqrt(ds*ds+dz*dz);

    TVector3 pFromEnd = pPos - start;
    result = TMath::Min(result, pFromEnd.Mag());
    pFromEnd = pPos - stop;
    result = TMath::Min(result, pFromEnd.Mag());
    return result;

   // Another method, by using TF1 to find inverse function and minimize
}

bool strawtubesDigi::InSmallerSection(TVector3 pPos, TVector3 start, TVector3 stop, Float_t ID)
{
   TVector3 wPos = ((start.x() - pPos.x()) * stop + (pPos.x() - stop.x()) * start) * (1. / (start.x() - stop.x()));
   Double_t wireShift = FindWireShift(pPos.x(), start.x(), stop.x(), ID);
   Double_t tubeShift = FindTubeShift(pPos.x(), start.x(), stop.x(), ID);
   residualsInStraw->Fill(wPos.z() - pPos.z());
   if (wireShift <= tubeShift) // the wire is above the tube center, upper part is smaller part
   {
      if (pPos.y() > wPos.y() - wireShift) {
         return true;
      } else
         return false;
   } else // the wire is under the tube center, lower part is smaller part
   {
      if (pPos.y() > wPos.y() - wireShift) {
         return false;
      } else
         return true;
   }
}

Double_t strawtubesDigi::GetWireOffset(Float_t ID) {
   return GetMaxTubeSagging(ID) - GetMaxWireSagging(ID);
}

