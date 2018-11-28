import argparse, sys, os, json, copy
from array import *

"""
Function to define color plaette for 2D efficiency plots
"""

def efficiencyPalette() :
    
    from ROOT import TColor
    
    pcol = []
    
    for iBin in range(0,100) :

        rgb  = []

        if iBin < 70 :
            rgb = [0.70+0.007*iBin, 0.00+0.0069*iBin, 0.00]
    
        elif iBin < 90 :
            rgb = [0.70+0.007*iBin, 0.00+0.0069*iBin+0.10+0.01*(iBin-70), 0.00]
      
        else :
            rgb = [0.98-0.098*(iBin-90), 0.80, 0.00]

        pcol.append(TColor.GetColor(rgb[0], rgb[1], rgb[2]))
        
    return pcol

                    
"""
Setup argument parser
"""

parser = argparse.ArgumentParser(description="This program takes an input JSON config and extracts plots from ROOT files. " \
                                             + "The output consists of a plot with superimposed plots from multiple files." )
parser.add_argument("inputJsonConfig", help="Path to the input JSON config file")
parser.add_argument("-f", "--fast", default=0, action="count", help="Skip fetching and saving the fit canvases for each plot")
parser.add_argument("-v", "--verbosity", default=1, help="Increase or decrease output verbosity")
args = parser.parse_args()

"""
Parse JSON file
"""

with open(args.inputJsonConfig, 'r') as f:
    config = json.loads(f.read())

"""
Go through plots defined in config JSON
"""

from ROOT import * # import this here, otherwise it overwrites the argparse stuff
gROOT.SetBatch(True) # set ROOT to batch mode, this suppresses printing canvases
gROOT.ProcessLine("gErrorIgnoreLevel = 1001;") # suppress stdout pollution of canvas.Print(...)

TH1.AddDirectory(False)
gStyle.SetOptTitle(0)
gStyle.SetPaintTextFormat("1.3f");
gStyle.SetHistMinimumZero()

for keyPlot in config:
    if args.verbosity==1:
        print('Processing plot config: {}'.format(keyPlot))
        print('Config comment: {}'.format(config[keyPlot]['comment']))

    # Get result plots from fit canvases (in dir fit_eff_plots)

    # Get input parameters
    inputFilenames = []
    inputPlotNames = []
    inputFolders = []
    inputLegendEntries = []
    inputLabels = []
 
    for keyInputs in sorted(config[keyPlot]['inputs']):
        inputFilenames.append(config[keyPlot]['inputs'][keyInputs]['filename'])
        inputPlotNames.append(config[keyPlot]['inputs'][keyInputs]['plot'])
        inputFolders.append(config[keyPlot]['inputs'][keyInputs]['folder'])
        inputLegendEntries.append(config[keyPlot]['inputs'][keyInputs]['legendEntry'])
        inputLabels.append(config[keyPlot]['inputs'][keyInputs]['label'])

    # Get input graphs from files
    inputHistos = {}
    for iHisto in range(len(inputFilenames)):
        inputFile = TFile.Open(inputFilenames[iHisto])
        if not inputFile:
            print('[ERROR] File not found: {}'.format(inputFilenames[iHisto]))
            sys.exit()
        inputDir = inputFile.GetDirectory(inputFolders[iHisto])
        inputName = None
        for keys in inputDir.GetListOfKeys():
            if keys.GetName() == inputPlotNames[iHisto] or inputPlotNames[iHisto] == "all" :
                inputName = keys.GetName()

                if inputName.find("=") > 0 or \
                   inputDir.Get(inputName).ClassName() == "TDirectoryFile":
                    continue

                if inputPlotNames[iHisto] == "all" :
                    nameTag = inputName
                else :
                    nameTag = "histo"

                if not inputHistos.has_key(nameTag):
                    inputHistos[nameTag] = []
            
                print('Load plot \'{}\': {}'.format(inputFolders[iHisto], inputName))
                print inputDir.Get(inputName).ClassName()

                histo = copy.deepcopy(inputDir.Get(inputName).Clone(str(iHisto)))
                inputHistos[nameTag].append(histo)
        inputFile.Close()

    # Set line color and marker style for each graph using given maps from config
    colorMap    = config[keyPlot]['plot']['colorMap']
    markerMap   = config[keyPlot]['plot']['markerMap']
    legendRange = config[keyPlot]['plot']['legendRange']
    if args.verbosity==1:
        print('Using colormap: {}'.format(colorMap))
        print('Using markermap: {}'.format(markerMap))
    if len(colorMap)<len(inputFolders):
        print('[ERROR] The defined colormap has not enough entries for the number of defined input folders.')
        sys.exit()
    if len(markerMap)<len(inputFolders):
        print('[ERROR] The defined markermap has not enough entries for the number of defined input folders.')
        sys.exit()

    for histoName,histograms in inputHistos.iteritems():

        for iHisto in range(len(histograms)):

            histograms[iHisto].SetLineColor(colorMap[iHisto])
            histograms[iHisto].SetLineWidth(2)
            histograms[iHisto].SetMarkerStyle(markerMap[iHisto])
            histograms[iHisto].SetMarkerColor(colorMap[iHisto])

        # Setup canvas with all elements
        canvas = TCanvas('canvas', 'canvas', 800, 800)

        pad = TPad('pad', 'pad', 0.01, 0.00, 1.00, 1.00)
 
        pad.SetGrid()
        #pad.SetLogz()
        pad.Draw()
        pad.cd()

        option = config[keyPlot]['plot']['option']
 
        hasGridByWheel  = option.find("gridByWheel") > -1
        if hasGridByWheel :
            option = option.replace("gridByWheel","")
            
        hasGridBySector = option.find("gridBySector") > -1
        if hasGridBySector :
            option = option.replace("gridBySector","")
            
        hasGrid = option.find("grid") > -1
        if hasGrid :
            option = option.replace("grid","")

        hasChamberSummary  = option.find("chamberSummary") > -1
        if hasChamberSummary :
            option = option.replace("chamberSummary","")

        hasCleanEmptyBins = option.find("cleanEmptyBins") > -1
        if hasCleanEmptyBins :
            option = option.replace("cleanEmptyBins","")

            
        plotX  = config[keyPlot]['plot']['x']
        plotY  = config[keyPlot]['plot']['y']
        # Generate superimposed graph using TMultiHisto
 
        if hasChamberSummary :
            gStyle.SetOptStat("emu")
            gStyle.SetStatX(0.9)
            gStyle.SetStatY(0.9)
        else:
            gStyle.SetOptStat(0)
        
        if config[keyPlot]['plot'].has_key("z") :
            plotZ = config[keyPlot]['plot']['z']
    
        for iHisto in range(len(histograms)):

            histoDim   = histograms[iHisto].GetDimension()
            histoClass = histograms[iHisto].ClassName()

            histograms[iHisto].SetTitle(";"+plotX[2]+";"+plotY[2])

            if hasChamberSummary :
                histograms[iHisto].SetFillColor(kOrange-2)
                
            if iHisto == 0 :
                if histoDim == 2 :
                    histograms[iHisto].Draw(option)
                else :
                    if hasCleanEmptyBins :
                        histograms[iHisto].Paint(option + "AP")
                        histo = histograms[iHisto].GetPaintedGraph()
                        for iPoint in reversed(range(0,histo.GetN())) :
                            pointY = histo.GetY()[iPoint]
                            if pointY < 0.05 :
                                histo.RemovePoint(iPoint)
                            histo.Draw(option + "AP")
                    else :
                        histograms[iHisto].Draw(option)
            else :
                if hasCleanEmptyBins :
                    histograms[iHisto].Paint("same" + option)
                    histo = histograms[iHisto].GetPaintedGraph()
                    for iPoint in reversed(range(0,histo.GetN())) :
                        pointY = histo.GetY()[iPoint]
                        if pointY < 0.005 :
                            histo.RemovePoint(iPoint)
                        histo.Draw("same" + option + "P")
                else :
                    histograms[iHisto].Draw('same' + option)

            canvas.Update()

            if histoClass == "TEfficiency" and histoDim == 1:
                histo = copy.deepcopy(histograms[iHisto].GetPaintedGraph())
            elif histoClass == "TEfficiency" and histoDim == 2:
                histo = copy.deepcopy(histograms[iHisto].GetPaintedHistogram())
            else :
                histo = copy.deepcopy(histograms[iHisto])
                
            histo.GetXaxis().SetRangeUser(plotX[0], plotX[1])

            if histoClass == "TEfficiency" and histoDim == 1 :
                histo.GetYaxis().SetRangeUser(plotY[0], plotY[1])
            #    histograms[iHisto].SetTitle(";"+plotX[2]+";"+plotY[2])
            elif histoClass == "TEfficiency" and histoDim == 2 : 
                palette = efficiencyPalette()
                nBins = len(palette)
                gStyle.SetPalette(nBins,array('i',palette))
                histo.GetYaxis().SetRangeUser(plotY[0], plotY[1])
                histo.SetMinimum(plotZ[0])
                histo.SetMaximum(plotZ[1])
                histo.SetContour(nBins)
                histo.Draw(option)
                
                if hasGridBySector :
                    histo.GetXaxis().SetNdivisions(histo.GetNbinsX() / 2,True)
                    line = TLine();
                    for x in range(1,12) :
                        line.DrawLine(x+0.5, -2.5, x+0.5, 2.5);

                    histo.GetYaxis().SetNdivisions(histo.GetNbinsY() / 2,True)
                    for y in range(-2,2) :
                        line.DrawLine(0.5, y+0.5, 12.5, y+0.5);
                        
                if hasGridByWheel :
                    histo.GetXaxis().SetNdivisions(histo.GetNbinsX() / 4,True)
                    line = TLine();
                    for x in range(1,4) :
                        line.DrawLine((x*5)+1.0, 1.0, (x*5)+1.0, 13.0);
                    for x in range(0,20) :
                        histo.GetXaxis().SetBinLabel(x+1,"YB"+str(x%5 - 2))
                    histo.GetXaxis().LabelsOption("v")
                    latex = TLatex()
                    latex.SetNDC()
                    latex.SetTextFont(62)
                    latex.SetTextSize(0.028)
                    latex.SetTextAlign(11);
                    latex.DrawLatex(0.19, 0.01, "MB1")
                    latex.DrawLatex(0.39, 0.01, "MB2")
                    latex.DrawLatex(0.59, 0.01, "MB3")
                    latex.DrawLatex(0.79, 0.01, "MB4")
                    histo.GetYaxis().SetNdivisions(histo.GetNbinsY(),True)
                    for x in range(1,13) :
                        histo.GetYaxis().SetBinLabel(x,str(x))
                        
                if hasGrid or hasGridBySector or hasGridByWheel :
                    histoClone = histo.Clone()
                    histoClone.GetXaxis().SetNdivisions(histo.GetNbinsX(),True)
                    histoClone.GetYaxis().SetNdivisions(histo.GetNbinsY(),True)
                    histoClone.GetXaxis().SetTickLength(0.01)
                    histoClone.GetYaxis().SetTickLength(0.01)
                    histoClone.Draw("same axig")
            elif histoClass == "TH2F" :
                gStyle.SetPalette(1)
                histo.GetYaxis().SetRangeUser(plotY[0], plotY[1])
                histo.Draw(option)
            else:
                histo.SetLineWidth(2)
                histo.GetYaxis().SetRangeUser(0.0, histo.GetMaximum() * 1.5)
                histo.GetXaxis().SetTitle(plotX[2])
                histo.GetYaxis().SetTitle(plotY[2])
                
            canvas.Update()

            histo.GetXaxis().SetLabelSize(22)
            histo.GetXaxis().SetTitleFont(63)
            histo.GetXaxis().SetLabelFont(43)
            histo.GetXaxis().SetTitleSize(22)
            histo.GetXaxis().SetLabelSize(20)
            histo.GetXaxis().SetTitleOffset(1.2)
            if plotX[2] == "sector" and histo.GetXaxis().GetNbins() <= 14 :
                for iBinX in range(1,histo.GetXaxis().GetNbins() +1) :
                    histo.GetXaxis().SetBinLabel(iBinX,str(iBinX))
            
            histo.GetYaxis().SetLabelSize(22)
            histo.GetYaxis().SetTitleFont(63)
            histo.GetYaxis().SetLabelFont(43)
            histo.GetYaxis().SetTitleSize(22)
            histo.GetYaxis().SetLabelSize(20)
            histo.GetYaxis().SetTitleOffset(1.5)
            if plotY[2] == "wheel" and histo.GetYaxis().GetNbins() == 5 :
                for iBinY in range(1,histo.GetYaxis().GetNbins() +1) :
                    histo.GetYaxis().SetBinLabel(iBinY,str(iBinY - 3))
                        
            canvas.Update()

        canvas.Update()

        canvas.cd()

        if len(histograms) > 1 :
        
            leg = TLegend(legendRange[0], legendRange[1], legendRange[2], legendRange[3])

            for iHisto in range(len(histograms)):
                leg.AddEntry(histograms[iHisto], inputLegendEntries[iHisto], 'LP')

            leg.SetBorderSize(1)
            leg.SetTextFont(43)
            leg.SetTextSize(20)
            leg.Draw()

        canvas.cd()
        latex = TLatex()
        latex.SetNDC()
        latex.SetTextFont(61)
        latex.SetTextSize(0.030)
        latex.SetTextAlign(11);
        latex.DrawLatex(0.115, 0.91, config[keyPlot]['plot']['logo'][0])
        latex.SetTextFont(52)
        latex.SetTextSize(0.027)
        latex.SetTextAlign(11);
        latex.DrawLatex(0.179, 0.91, config[keyPlot]['plot']['logo'][1])
        latex.SetTextFont(42)
        latex.SetTextSize(0.030)
        latex.SetTextAlign(31);
        latex.DrawLatex(0.90, 0.91, config[keyPlot]['plot']['caption'])
        latex.SetTextAlign(11);
        latex.SetTextColor(1)
        latex.SetTextFont(61)
        latex.SetTextSize(0.032)
        latex.DrawLatex(legendRange[0], legendRange[3] + 0.02, config[keyPlot]['plot']['legendTitle'])
        canvas.Update()

        # Save plot
        outputDirectory = config[keyPlot]['output']['directory']
        if args.verbosity==1:
            print('Output directory: {}'.format(outputDirectory))
        if not os.path.exists(outputDirectory):
            os.makedirs(outputDirectory)
        for fileType in config[keyPlot]['output']['fileType']:

            if config[keyPlot]['output']['filenamePlot'] == "all":
                 fileNamePlot = histoName
            else:
                 fileNamePlot = config[keyPlot]['output']['filenamePlot']
            
            canvas.SaveAs(os.path.join(outputDirectory,fileNamePlot+'.'+fileType))

    if args.verbosity==1:
        print('')
